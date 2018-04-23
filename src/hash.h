/*
 * hash.h
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

#ifndef HASH_H
#define HASH_H

#include <stddef.h>
#include <inttypes.h>

#include "koa.h"
#include "list.h"
#include "vec.h"

#define HASH_SEED(x) ((x)->seed)

typedef uint64_t (*hash_f) (void *data);
typedef int (*hash_test_f) (void *value, void *hd);

typedef struct hash_node_s
{
	list_t link;
	void *value;
	size_t idx;
} hash_node_t;

typedef struct hash_s
{
	list_t **h;
	hash_f hf;
	hash_test_f tf;
	size_t bu;
	size_t size;
} hash_t;

hash_t *
hash_new (size_t bu, hash_f hf, hash_test_f tf);

void
hash_free (hash_t *ha);

void *
hash_add (hash_t *ha, void *data);

void
hash_fast_add (hash_t *ha, void *hn);

void
hash_remove (hash_t *ha, void *data);

void
hash_fast_remove (hash_t *ha, void *hn);

void *
hash_test (hash_t *ha, void *hd, uint64_t hash);

int
hash_find (hash_t *ha, void *data);

int
hash_occupied (hash_t *ha, uint64_t hash);

vec_t *
hash_get_all_values (hash_t *ha);

void *
hash_handle_copy (void *hn);

#endif /* HASH_H */
