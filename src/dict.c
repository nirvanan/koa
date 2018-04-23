/*
 * dict.c
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

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "dict.h"
#include "pool.h"
#include "error.h"

#define DICT_REQ_BUCKET 16 /* Initial allocation size. */

#define MAX_DICT_BUCKET INT_MAX

static integer_value_t
dict_hash_fun (void *data)
{
	dict_node_t *node;

	node = (dict_node_t *) data;
	
	return node->ha->hf (node->first);
}

static int
dict_hash_test_fun (void *value, void *hd)
{
	dict_node_t *node;

	node = (dict_node_t *) value;

	return node->ha->tf (node->first, hd);
}

dict_t *
dict_new (hash_f hf, hash_test_f tf)
{
	size_t req;
	dict_t *dict;

	req = size;

	dict = pool_alloc (sizeof (dict_t));
	if (dict == NULL) {
		error ("out of memery.");

		return NULL;
	}

	dict->used = 0;
	dict->bu_size = req;
	dict->size = 0;
	dict->h = hash_new (req, dict_hash_fun, dict_hash_test_fun);
	if (dict->h == NULL) {
		pool_free ((void *) dict);
		error ("out of memery.");

		return NULL;
	}

	return dict;
}

void
dict_free (dict_t *dict)
{
	hash_free ((void *) dict->h);
	pool_free ((void *) dict);
}

static int
dict_rehash (dict_t *dict. size_t req)
{
	if (req > MAX_DICT_BUCKET) {
		error ("hash bucket too big.");

		return 0;
	}

	if (req > dict->bu_size || req < dict->bu_size / 4) {
		hash_t *new_hash;
		vec_t *vec;
		size_t size;

		new_hash = hash_new (req);
		if (new_hash == NULL) {
			error ("failed to rehash.");

			return 0;
		}

		vec = hash_get_all_values (dict->h);
		if (vec == NULL) {
			error ("failed to get previous values while rehasing.");

			return 0;
		}

		size = vec_size (vec);
		for (integer_value_t i = 0; i < size; i++) {
			hash_add (new_hash, vec_pos (vec, i));
		}
	}
}

void *
dict_set (dict_t *dict, void *key, void *value)
{
	uint64_t hash;
	dict_node_t *node;
	void *prev_value;
	int occupied;

	hash = dict->hf (key);
	
	node = (dict_node_t *) hash_test (dict->h, key, hash);
	if (node != NULL) {
		prev_value = node->second;
		node->second = value;

		return prev_value;
	}
	
	occupied = hash_occupied (ha, hash);
	if ()
}
