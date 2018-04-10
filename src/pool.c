/*
 * pool.c
 * This file is part of koa
 *
 * Copyright (C) 2018 - Gordon Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pool.h"
#include "list.h"
#include "error.h"

/* The general structure of koa memory allocation is pool=>Page=>Cell.
 * There are several kinds of cells: from 8 bytes to 256 bytes, and all
 * allocations will be done by choosing the most proper size of cell.
 * We use a linked list to trace all pools and use a hash table to trace
 * pages for quick access. Pages appears in three states, that is,
 * full, empty and used. Each page contains exactly one type of cells.
 */

#define POOL_REQUEST_SIZE (1024*1024)
#define PAGE_SIZE 4096
#define MAX_CELL_SIZE 256
#define INIT_POOL_NUM 1
#define RECYCLE_CYCLE 100

#define BLOCK_START(x, s) ((void *)(((intptr_t)(x))&(~((intptr_t)(s-1)))))

#define BLOCK_ALIGNED(x, s) ((intptr_t)BLOCK_START((x),(s))==(intptr_t)(x))

#define CELL_SIZE(x) ((size_t)(x)*8)

#define REQ_2_CELL_TYPE(x) ((int)(x-1)/8+1)

#define PAGE_HASH_BUCKET 196613

typedef unsigned char cell_type_t;

typedef struct page_s
{
	list_t link;
	void *pool;
	void *hash;
	cell_type_t t;
	int allocated;
	size_t cell_size; /* Pages can be reused for another size. */
	void *free; /* The first allocable cell. */
} page_t;
	
typedef struct pool_s
{
	list_t link;
	void *po;
	void *extra;
	int used;
	list_t *free;
	int cycle; /* To release pools that is empty for a long time. */
} pool_t;

typedef struct page_hash_s
{
	list_t link;
	void *p;
} page_hash_t;

/* All pools. */
static list_t *g_pool_list;

/* Page table for quick access. */
static list_t *g_page_table[MAX_CELL_SIZE / 8 + 1];
static list_t *g_full_table[MAX_CELL_SIZE / 8 + 1];

/* Altough we have hash utility using pool, but at this moment
 * we can not use it, so we just make another one. */
static list_t *g_page_hash[PAGE_HASH_BUCKET];

static void
pool_page_init (page_t *page, cell_type_t t)
{
	void *page_end;
	void *page_t_end;

	page->t = t;
	page->cell_size = CELL_SIZE (t);
	page->allocated = 0;
	page_t_end = (void *) page + sizeof (page_t);
	page->free = BLOCK_START (page_t_end, CELL_SIZE (t));
	if (!BLOCK_ALIGNED (page_t_end, CELL_SIZE (t))) {
		page->free += CELL_SIZE (t);
	}
	/* Chain all cells. */
	page_end = (void *) page + PAGE_SIZE;
	for (void *c = page->free; c < page_end; c += CELL_SIZE (t)) {
		if (c + CELL_SIZE (t) < page_end) {
			*((void **) c) = c + CELL_SIZE (t);
		}
	}
}

static void
pool_page_hash_add (page_t *page)
{
	int b;
	page_hash_t *ph;

	b = (intptr_t) page % PAGE_HASH_BUCKET;
	/* Hash data can be allocated from pool! */
	ph = (page_hash_t *) pool_alloc (sizeof (page_hash_t));
	if (ph == NULL) {
		/* TODO: Need throw. */
		fatal_error ("no enough memory.");
	}
	ph->p = page;
	g_page_hash[b] = list_append (g_page_hash[b], (list_t *) ph);
	page->hash = ph;
}


static void
pool_page_hash_remove (page_t *page)
{
	int b;
	page_hash_t *ph;

	b = (intptr_t) page % PAGE_HASH_BUCKET;
	ph = (page_hash_t *) page->hash;
	g_page_hash[b] = list_remove (g_page_hash[b], (list_t *) ph);

	pool_free (ph);
}

static void
pool_new (void *extra)
{
	pool_t *new_pool;
	void *pool_end;
	void *pool_t_end;

	/* In some system, calloc is faster than malloc+memset.
	 * For example, freebsd. */
	new_pool = (pool_t *) calloc (1, POOL_REQUEST_SIZE);
	if (new_pool == NULL) {
		/* TODO: Need throw. */
		fatal_error ("no enough memory.");
	}

	/* Align pages. */
	pool_t_end = (void *) new_pool + sizeof (page_t);
	new_pool->po = BLOCK_START (pool_t_end, PAGE_SIZE);
	if (!BLOCK_ALIGNED (pool_t_end, PAGE_SIZE)) {
		new_pool->po += PAGE_SIZE;
	}

	/* Insert all pages into free list and hash. */
	pool_end = (void *) new_pool + POOL_REQUEST_SIZE;
	for (void *p = new_pool->po; p < pool_end; p += PAGE_SIZE) {
		page_t *page;
		
		page = (page_t *) p;
		new_pool->free = list_append (new_pool->free, (list_t *) page);
		page->pool = new_pool;
	}

	new_pool->used = 0;
	new_pool->extra = extra;

	/* Insert current pool to list. */
	g_pool_list = list_append (g_pool_list, (list_t *) new_pool);
}

static void
pool_empty_page_out (pool_t *pool, cell_type_t t)
{
	page_t *page;

	page = (page_t *) pool->free;
	pool->free = list_remove (pool->free, (list_t *) page);
	pool->used++;
	g_page_table[t] = list_append (g_page_table[t], (list_t *) page);

	pool_page_hash_add (page);
}

static void
pool_empty_page_in (pool_t *pool, page_t *page)
{
	cell_type_t t;
	
	t = page->t;
	g_page_table[t] = list_remove (g_page_table[t], (list_t *) page);
	pool->free = list_append (pool->free, (list_t *) page);
	pool->used--;

	pool_page_hash_remove (page);

	/* Set empty flag for recycling empty pools. */
	if (pool->used <= 0) {
		pool->cycle = 1;
	}
}

static page_t *
pool_get_page (size_t size)
{
	cell_type_t cell_idx;
	list_t *l;
	page_t *page;

	cell_idx = REQ_2_CELL_TYPE (size);

	/* First we lookup used page table. */
	if (g_page_table[cell_idx] != NULL) {
		return (page_t *) g_page_table[cell_idx];
	}

	/* Then, we pick up an empty page in an avaliable pool. */
	for (l = g_pool_list; l; l = list_next (l)) {
		pool_t *pool;

		pool = (pool_t *) l;
		if (pool->free != NULL) {
			page = (page_t *) pool->free;
			pool_page_init (page, (cell_type_t) cell_idx);
			pool_empty_page_out (pool, (cell_type_t) cell_idx);
			pool->used++;

			return page;
		}
	}

	/* No empty page? Allocte a new pool! */
	pool_new (NULL);
	page = (page_t *) ((pool_t *) g_pool_list)->free;
	pool_page_init (page, (cell_type_t) cell_idx);
	pool_empty_page_out ((pool_t *) g_pool_list, (cell_type_t) cell_idx);
	((pool_t *) g_pool_list)->used++;

	return page;
}

static void *
pool_get_cell (page_t *page)
{
	void *cell;
	cell_type_t t;

	/* It should have a valid cell. */
	if (page->free == NULL) {
		/* TODO: Need throw. */
		fatal_error ("no free cell in an empty or used page?");
	}
	
	cell = page->free;
	page->allocated++;
	page->free = *((void **) cell);
	/* Full? */
	if (page->free == NULL) {
		t = page->t;
		g_page_table[t] = list_remove (g_page_table[t], (list_t *) page);
		g_full_table[t] = list_append (g_full_table[t], (list_t *) page);
	}

	return cell;
}

void *
pool_alloc (size_t size)
{
	page_t *page;

	/* If size is larger than max cell size, use system malloc then. */
	if (size > MAX_CELL_SIZE) {
		void *ret;

		ret = malloc (size);
		if (ret == NULL) {
			/* TODO: Need throw. */
			fatal_error ("no enough memory.");
		}

		return ret;
	}

	page = pool_get_page (size);
	if (page == NULL) {
		/* TODO: Need throw. */
		fatal_error ("can not find an avaliable page.");
	}

	return pool_get_cell (page);
}

static int
pool_is_pool_cell (void *bl)
{
	page_t *page;
	int h;

	page = (page_t *) BLOCK_START (bl, PAGE_SIZE);
	h = (intptr_t) page % PAGE_HASH_BUCKET;

	return list_find (g_page_hash[h], (list_t *) page);
}

static void
pool_page_full_2_used (page_t *page)
{
	cell_type_t t;

	t = page->t;
	g_full_table[t] = list_remove (g_full_table[t], (list_t *) page);
	g_page_table[t] = list_append (g_page_table[t], (list_t *) page);
}

void
pool_free (void *bl)
{
	page_t *page;

	if (!pool_is_pool_cell (bl)) {
		/* Use system free to release this block. */
		free (bl);

		return;
	}
	
	page = (page_t *) BLOCK_START (bl, PAGE_SIZE);

	/* Page from full to used? */
	if (page->free == NULL) {
		pool_page_full_2_used (page);
	}

	page = (page_t *) BLOCK_START (bl, PAGE_SIZE);
	*((void **) bl) = page->free;
	page->free = bl;
	page->allocated--;

	/* Page from used to free? */
	if (page->allocated <= 0) {
		pool_empty_page_in ((pool_t *) page->pool, page);
	}
}

static void
pool_add_cycle (void *data, void *udata)
{
	pool_t *p;

	UNUSED (udata);
	p = (pool_t *) data;
	
	p->cycle++;
}

static int
pool_need_recycle (void *data, void *udata)
{
	pool_t *p;

	UNUSED (udata);
	p = (pool_t *) data;
	
	return p->cycle > RECYCLE_CYCLE;
}

void
pool_recycle ()
{
	/* This is called by interperter, when a pool is being empty for
	 * more than RECYCLE_CYCLE cycles, we recycle it. */
	list_foreach (g_pool_list, pool_add_cycle, NULL);

	g_pool_list = list_cleanup (g_pool_list, pool_need_recycle, NULL);
}

void
pool_init ()
{
	/* Init pool(s) when startup. */
	for (int i = 0; i < INIT_POOL_NUM; i++) {
		pool_new (NULL);
	}
	memset (g_page_table, 0, sizeof (g_page_table));
	memset (g_full_table, 0, sizeof (g_page_table));
	memset (g_page_hash, 0, sizeof (g_page_hash));
}
