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

/* The general structure of koa memory allocation is Pool=>Page=>Cell.
 * There are several kinds of cells: from 8 bytes to 256 bytes, and all
 * allocations will be done by choosing the most proper size of cell.
 * We use a linked list to trace all pools and use a hash table to trace
 * pages for quick access. Pages appear in three states, that is,
 * full, empty and used. Each page contains exactly one type of cells.
 */

#define PAGE_SIZE 4096
#define POOL_REQUEST_SIZE (1024*PAGE_SIZE)
#define CHILD_INIT_REQUEST_SIZE (16*PAGE_SIZE) /* For child thread when starting. */
#define INIT_POOL_NUM 1
#define RECYCLE_CYCLE 100

#define BLOCK_START(x, s) ((void *)(((intptr_t)(x))&(~((intptr_t)((s)-1)))))

#define BLOCK_ALIGNED(x, s) ((intptr_t)BLOCK_START((x),(s))==(intptr_t)(x))

#define CELL_SIZE(x) ((size_t)(x)*8)

#define REQ_2_CELL_TYPE(x) ((int)((x)-1)/8+1)

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
	page_t *p;
} page_hash_t;

static __thread allocator_t *g_allocator;
static __thread allocator_t *g_second_allocator;

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
		*((void **) c) = c + 2 * CELL_SIZE (t) <= page_end? c + CELL_SIZE (t): NULL;
	}
}

static int
pool_page_hash_add (allocator_t *allocator, page_t *page)
{
	int b;
	page_hash_t *ph;

	b = (intptr_t) page % PAGE_HASH_BUCKET;
	/* Hash data can be allocated from pool! */
	ph = (page_hash_t *) pool_alloc_allocator (allocator, sizeof (page_hash_t));
	if (ph == NULL) {
		return 0;
	}

	ph->p = page;
	allocator->page_hash[b] = list_append (allocator->page_hash[b], LIST (ph));
	page->hash = ph;

	return 1;
}


static void
pool_page_hash_remove (allocator_t *allocator, page_t *page)
{
	int b;
	page_hash_t *ph;

	b = (intptr_t) page % PAGE_HASH_BUCKET;
	ph = (page_hash_t *) page->hash;
	allocator->page_hash[b] = list_remove (allocator->page_hash[b], LIST (ph));

	pool_free_allocator (allocator, (void *) ph);
}

static pool_t *
pool_new (allocator_t *allocator, size_t request_size, void *extra)
{
	pool_t *new_pool;
	void *pool_end;
	void *pool_t_end;

	/* In some system, calloc is faster than malloc+memset.
	 * For example, freebsd. */
	new_pool = (pool_t *) calloc (1, request_size);
	if (new_pool == NULL) {
		return NULL;
	}

	/* Align pages. */
	pool_t_end = (void *) new_pool + sizeof (pool_t);
	new_pool->po = BLOCK_START (pool_t_end, PAGE_SIZE);
	if (!BLOCK_ALIGNED (pool_t_end, PAGE_SIZE)) {
		new_pool->po += PAGE_SIZE;
	}

	/* Insert all pages into free list and hash. */
	pool_end = (void *) new_pool + request_size;
	for (void *p = new_pool->po; p + PAGE_SIZE <= pool_end; p += PAGE_SIZE) {
		page_t *page;
		
		page = (page_t *) p;
		new_pool->free = list_append (new_pool->free, LIST (page));
		page->pool = new_pool;
	}

	new_pool->used = 0;
	new_pool->extra = extra;

	/* Insert current pool to list. */
	allocator->pool_list = list_append (allocator->pool_list, LIST (new_pool));

	return new_pool;
}

static void
pool_empty_page_out (allocator_t *allocator, pool_t *pool, cell_type_t t)
{
	page_t *page;

	page = (page_t *) pool->free;
	pool->free = list_remove (pool->free, LIST (page));
	pool->used++;
	allocator->page_table[t] = list_append (allocator->page_table[t], LIST (page));
}

static void
pool_empty_page_in (allocator_t *allocator, pool_t *pool, page_t *page)
{
	cell_type_t t;
	
	t = page->t;
	allocator->page_table[t] = list_remove (allocator->page_table[t], LIST (page));
	pool->free = list_append (pool->free, LIST (page));
	pool->used--;

	/* Set empty flag for recycling empty pools. */
	if (pool->used <= 0) {
		pool->cycle = 1;
	}
}

static page_t *
pool_get_page (allocator_t *allocator, size_t size, int *need_hash)
{
	cell_type_t cell_idx;
	list_t *l;
	page_t *page;
	pool_t *first_pool;

	cell_idx = REQ_2_CELL_TYPE (size);

	/* First we lookup used page table. */
	if (allocator->page_table[cell_idx] != NULL) {
		return (page_t *) allocator->page_table[cell_idx];
	}

	/* Then, we pick up an empty page in an avaliable pool. */
	for (l = allocator->pool_list; l; l = LIST_NEXT (l)) {
		pool_t *pool;

		pool = (pool_t *) l;
		if (pool->free != NULL) {
			page = (page_t *) pool->free;
			pool_empty_page_out (allocator, pool, cell_idx);
			pool_page_init (page, cell_idx);
			*need_hash = 1;

			return page;
		}
	}

	/* No empty page? Allocte a new pool!
	 * Note that if current thread is not the main thread and
	 * this is the first pool to allocate, make a small one. */
	size_t init_size = allocator->pool_list == NULL? CHILD_INIT_REQUEST_SIZE: POOL_REQUEST_SIZE;
	first_pool = (pool_t *) pool_new (allocator, init_size, NULL);
	if (first_pool == NULL) {
		return NULL;
	}
	page = (page_t *) first_pool->free;
	pool_empty_page_out (allocator, first_pool, cell_idx);
	pool_page_init (page, cell_idx);
	*need_hash = 1;

	return page;
}

static void *
pool_get_cell (allocator_t *allocator, page_t *page)
{
	void *cell;
	cell_type_t t;

	/* It should have a valid cell. */
	if (page->free == NULL) {
		return NULL;
	}
	
	cell = page->free;
	page->allocated++;
	page->free = *((void **) cell);
	/* Full? */
	if (page->free == NULL) {
		t = page->t;
		allocator->page_table[t] = list_remove (allocator->page_table[t], LIST (page));
		allocator->full_table[t] = list_append (allocator->full_table[t], LIST (page));
	}

	return cell;
}

void *
pool_alloc (size_t size)
{
	if (g_second_allocator != NULL) {
		return pool_alloc_allocator (g_second_allocator, size);
	}

	return pool_alloc_allocator (g_allocator, size);
}

void *
pool_alloc_allocator (allocator_t *allocator, size_t size)
{
	page_t *page;
	void *cell;
	int need_hash;

	/* If size is larger than max cell size, use system malloc then. */
	if (size > MAX_CELL_SIZE) {
		void *ret;

		ret = malloc (size);
		if (ret == NULL) {
			return NULL;
		}

		return ret;
	}

	need_hash = 0;
	page = pool_get_page (allocator, size, &need_hash);
	if (page == NULL) {
		return NULL;
	}

	cell = pool_get_cell (allocator, page);
	if (cell == NULL) {
		return NULL;
	}

	if (need_hash && pool_page_hash_add (allocator, page) == 0) {
		pool_free_allocator (allocator, cell);
		error ("failed to hash page table.");

		return NULL;
	}

	return cell;
}

void *
pool_calloc (size_t member, size_t size)
{
	if (g_second_allocator != NULL) {
		return pool_calloc_allocator (g_second_allocator, member, size);
	}

	return pool_calloc_allocator (g_allocator, member, size);
}

void *
pool_calloc_allocator (allocator_t *allocator, size_t member, size_t size)
{
	size_t total;
	void *ret;

	total = member * size;
	if (total > MAX_CELL_SIZE) {
		ret = calloc (member, size);
		if (ret == NULL) {
			return NULL;
		}

		return ret;
	}

	ret = pool_alloc_allocator (allocator, total);
	if (ret == NULL) {
		return NULL;
	}

	memset (ret, 0, total);

	return ret;
}

static int
pool_find_page_fun (list_t *list, void *data)
{
	page_hash_t *ph;
	page_t **page;

	ph = (page_hash_t *) list;
	page = (page_t **) data;
	if (ph->p == *page) {
		*page = NULL;

		return 1;
	}

	return 0;
}

static int
pool_is_pool_cell (allocator_t *allocator, void *bl)
{
	page_t *page;
	int h;

	page = (page_t *) BLOCK_START (bl, PAGE_SIZE);
	h = (intptr_t) page % PAGE_HASH_BUCKET;

	list_foreach (allocator->page_hash[h], pool_find_page_fun, (void *) &page);

	return page == NULL;
}

static void
pool_page_full_2_used (allocator_t *allocator, page_t *page)
{
	cell_type_t t;

	t = page->t;
	allocator->full_table[t] = list_remove (allocator->full_table[t], LIST (page));
	allocator->page_table[t] = list_append (allocator->page_table[t], LIST (page));
}

void
pool_free (void *bl)
{
	if (g_second_allocator != NULL) {
		pool_free_allocator (g_second_allocator, bl);
	}
	pool_free_allocator (g_allocator, bl);
}

void
pool_free_allocator (allocator_t *allocator, void *bl)
{
	page_t *page;

	if (!pool_is_pool_cell (allocator, bl)) {
		/* Use system free to release this block. */
		free (bl);

		return;
	}
	
	page = (page_t *) BLOCK_START (bl, PAGE_SIZE);

	/* Page from full to used? */
	if (page->free == NULL) {
		pool_page_full_2_used (allocator, page);
	}

	page = (page_t *) BLOCK_START (bl, PAGE_SIZE);
	*((void **) bl) = page->free;
	page->free = bl;
	page->allocated--;

	/* Page from used to free? */
	if (page->allocated <= 0) {
		pool_page_hash_remove (allocator, page);
		pool_empty_page_in (allocator, (pool_t *) page->pool, page);
	}
}

static int
pool_add_cycle (list_t *list, void *data)
{
	pool_t *p;

	UNUSED (data);
	p = (pool_t *) list;
	
	if (p->used <= 0) {
		p->cycle++;
	}

	return 0;
}

static int
pool_need_recycle (list_t *list, void *data)
{
	pool_t *p;

	UNUSED (data);
	p = (pool_t *) list;
	
	return p->cycle > RECYCLE_CYCLE;
}

void
pool_recycle ()
{
	/* This is called by interperter, when a pool is being empty for
	 * more than RECYCLE_CYCLE cycles, we recycle it. */
	list_foreach (g_allocator->pool_list, pool_add_cycle, NULL);

	g_allocator->pool_list = list_cleanup (g_allocator->pool_list, pool_need_recycle, 1, NULL);
}

static int
pool_free_list (list_t *list, void *data)
{
	free ((void *) list);

	return 0;
}

void
pool_free_all ()
{
	list_foreach (g_allocator->pool_list, pool_free_list, NULL);
}

void
pool_set_second_allocator (allocator_t *allocator)
{
	g_second_allocator = allocator;
}

void
pool_set_allocator (allocator_t *allocator)
{
	if (g_allocator == NULL) {
		g_allocator = allocator;
	}
}

allocator_t *
pool_make_new_allocator ()
{
	allocator_t *allocator;

	allocator = calloc (1, sizeof (allocator_t));
	if (allocator == NULL) {
		fatal_error ("out of memory.");
	}

	return allocator;
}

void
pool_allocator_free (allocator_t *allocator)
{
	free ((void *) allocator);
}

void
pool_init ()
{
	g_allocator = calloc (1, sizeof (allocator_t));
	if (g_allocator == NULL) {
		fatal_error ("out of memory.");
	}

	/* Init pool(s) for main thread when startup. */
	for (int i = 0; i < INIT_POOL_NUM; i++) {
		pool_t *pool;
		
		pool = pool_new (g_allocator, POOL_REQUEST_SIZE, NULL);
		if (pool == NULL) {
			fatal_error ("failed to allocate pool on startup.");
		}
	}
}
