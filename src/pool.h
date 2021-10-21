/*
 * pool.h
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

#ifndef POOL_H
#define POOL_H

#include <stddef.h>

#include "koa.h"
#include "list.h"

#define MAX_CELL_SIZE 512
#define PAGE_HASH_BUCKET 196613

typedef struct allocator_s
{
    list_t *pool_list; /* All pools. */
    list_t *page_table[MAX_CELL_SIZE / 8 + 1]; /* Page table for quick access. */
    list_t *full_table[MAX_CELL_SIZE / 8 + 1]; /* All full pages. */
    list_t *page_hash[PAGE_HASH_BUCKET];
} allocator_t;

void *
pool_alloc (size_t size);

void *
pool_alloc_allocator (allocator_t *allocator, size_t size);

void *
pool_calloc (size_t member, size_t size);

void *
pool_calloc_allocator (allocator_t *allocator, size_t member, size_t size);

void
pool_free (void *bl);

void
pool_free_allocator (allocator_t *allocator, void *bl);

void
pool_recycle ();

void
pool_free_all ();

void
pool_set_second_allocator (allocator_t *allocator);

void
pool_set_allocator (allocator_t *allocator);

allocator_t *
pool_make_new_allocator ();

void
pool_init ();

#endif /* POOL_H */
