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

#include "pool.h"
#include "error.h"

/* The general structure of koa memory allocation is Poll=>Page=>Cell.
 * There are several kinds of cells: from 8 bytes to 256 bytes, and all
 * allocations will be done by choosing the most proper size of cell.
 * We use a linked list to trace all pools and use a hash table to trace
 * pages for quick access. Pages appears in three states, that is,
 * full, empty and used. Each page contains exactly one type of cells.
 */

#define POOL_REQUEST_SIZE (4 * 1024 * 1024)
#define PAGE_SIZE 4096
#define MAX_CELL_SIZE 256
#define INIT_POOL_NUM 1

#define BLOCK_START(x, s) ((x) & (~s))

#define CELL_SIZE(x) ((x) * 8)

typedef uchar cell_type_t;

typedef struct page_s {
	page_s *prev;
	page_s *next;
	cell_type_t t;
	size_t cell_size; /* Pages can be reused for another size. */
	int allocated;
	int size;
	void *free; /* The first allocable cell. */
} page_t;
	
typedef struct pool_s {
	pool_s *prev;
	pool_s *next;
	void *po;
	void *extra;
	page_t *free;
	int cycle; /* To release polls that is empty for a long time. */
} pool_t;

/* All pools. */
static pool_t *g_poll_list;

/* Page hash table for quick access, this is used for used pages. */
static pool_t *g_page_table[MAX_CELL_SIZE / 8 + 1];
static pool_t *g_full_table[MAX_CELL_SIZE / 8 + 1];

static void
pool_page_init (page_t *page, cell_type_t t)
{
	void *page_t_end;

	page->t = t;
	page->cell_size = CELL_SIZE (t);
	page->allocated = 0;
	page_t_end = page + sizeof (page_t);
	page->free = BLOCK_START (page_t_end, CELL_SIZE (t));
	if (page_t_end & ~CELL_SIZE (t) != page_t_end) {
		page->free += CELL_SIZE (t);
	}
	/* Chain all cells. */
	for (void *c = page->free; c < (void *) (page + 1); c += CELL_SIZE (t)) {
		*c = c + CELL_SIZE (t);
	}
	page->size = ((void *) (page + 1) - page->free) / CELL_SIZE (t);
}

static void
pool_new (void *extra)
{
	pool_t *new_pool;
	void *pool_end;

	new_poll = (pool_t *) calloc (1, POOL_REQUEST_SIZE);
	if (new_pool == NULL) {
		/* TODO: Need throw. */
		fatal_error ("no enough memory.");
	}

	/* Align pages. */
	new_pool->po = PAGE_START (new_pool + PAGE_SIZE);
	/* Insert all pages into free list. */
	pool_end = (void *) new_poll + POOL_REQUEST_SIZE;
	for (void *p = new_pool->po; p < pool_end; p += PAGE_SIZE) {
		page_t *page = (page_t *) p;

		page->next = new_pool->free;
		page->prev = NULL;
		if (new_pool->free) {
			new_pool->free->prev = page;
		}
		new_pool->free = page;
	}
	/* Insert current pool to list. */
	new_pool->next = g_poll_list;
	new_pool->prev = NULL;
	if (g_poll_list) {
		g_poll_list->prev = new_pool;
	}
	g_poll_list = new_pool;
}

void *
pool_alloc (size_t size)
{
	return malloc (size);
}

void
pool_free (void *bl)
{
	free (bl);
}

void
pool_init ()
{
	/* Init pool(s) when startup. */
	for (int i - 0; i < INIT_POOL_NUM; i++) {
		pool_new ();
	}
	memset (g_page_table, 0, sizeof (g_page_table));
	memset (g_full_table, 0, sizeof (g_page_table));
}
