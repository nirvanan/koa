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

#define DICT_REQ_BUCKET 8 /* Initial allocation size. */

#define MAX_DICT_BUCKET INT_MAX

static uint64_t
dict_hash_fun (void *data)
{
	dict_node_t *node;

	node = (dict_node_t *) data;
	
	return node->d->hf (node->first);
}

static int
dict_hash_test_fun (void *value, void *hd)
{
	dict_node_t *node;

	node = (dict_node_t *) value;

	return node->d->tf (node->first, hd);
}

static void
dict_hash_cleanup_fun (void *value)
{
	pool_free (value);
}

dict_t *
dict_new (hash_f hf, hash_test_f tf)
{
	size_t req;
	dict_t *dict;

	req = DICT_REQ_BUCKET;

	dict = pool_alloc (sizeof (dict_t));
	if (dict == NULL) {
		fatal_error ("out of memory.");
	}

	dict->used = 0;
	dict->bu_size = req;
	dict->size = 0;
	dict->hf = hf;
	dict->tf = tf;
	dict->h = hash_new (req,
						dict_hash_fun,
						dict_hash_test_fun,
						dict_hash_cleanup_fun);
	if (dict->h == NULL) {
		pool_free ((void *) dict);
		fatal_error ("out of memory.");
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
dict_rehash (dict_t *dict, size_t req)
{
	if (req > MAX_DICT_BUCKET) {
		error ("hash bucket too big.");

		return 0;
	}

	if (req > dict->bu_size) {
		req = dict->bu_size * 2;
	}
	else if (req < dict->bu_size / 4) {
		req = dict->bu_size / 2;
		if (req < DICT_REQ_BUCKET) {
			req = DICT_REQ_BUCKET;
		}
	}
	else {
		req = dict->bu_size;
	}

	if (req != dict->bu_size) {
		hash_t *new_hash;
		vec_t *vec;
		size_t size;
		dict_node_t *dn;

		new_hash = hash_new (req,
							 dict_hash_fun,
							 dict_hash_test_fun,
							 dict_hash_cleanup_fun);
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
			dn = (dict_node_t *) vec_pos (vec, i);
			dn->hn = hash_add (new_hash, (void *) dn);
			if (dn->hn == NULL) {
				hash_free (new_hash);
				error ("failed to rehash, can not add hash node.");

				return 0;
			}
		}

		/* Clear old hash table. */
		hash_free (dict->h);

		dict->h = new_hash;
		dict->bu_size = req;
		dict->used = 0;
		for (uint64_t i = 0; i < (uint64_t) req; i++) {
			if (hash_occupied (dict->h, i)) {
				dict->used++;
			}
		}
	}

	return 1;
}

void *
dict_set (dict_t *dict, void *key, void *value)
{
	uint64_t hash;
	dict_node_t *node;
	void *prev_value;
	int occupied;

	hash = dict->hf (key);
	
	/* Already a node with this key? */
	node = (dict_node_t *) hash_test (dict->h, key, hash);
	if (node != NULL) {
		prev_value = node->second;
		node->second = value;

		return prev_value;
	}
	
	/* Need rehash? */
	occupied = hash_occupied (dict->h, hash);
	if (occupied == 0) {
		if (dict_rehash (dict, dict->used + 1) == 0) {
			return NULL;
		}
	}

	node = (dict_node_t *) pool_alloc (sizeof (dict_node_t));
	if (node == NULL) {
		fatal_error ("out of memory.");
	}

	node->d = dict;
	node->first = key;
	node->second = value;
	node->hn = hash_add (dict->h, (void *) node);
	if (node->hn == NULL) {
		pool_free ((void *) node);
		error ("failed to add hash node.");

		return NULL;
	}
	
	dict->size++;
	dict->used += occupied? 0: 1;

	return value;
}

void *
dict_get (dict_t *dict, void *key)
{
	uint64_t hash;
	dict_node_t *node;

	hash = dict->hf (key);

	node = (dict_node_t *) hash_test (dict->h, key, hash);
	if (node == NULL) {
		return NULL;
	}

	return node->second;
}

/* Return the original key and set *value to the
 * original value. */
void *
dict_remove (dict_t *dict, void *key, void **value)
{
	uint64_t hash;
	dict_node_t *node;
	int occupied;
	void *origin_key;

	hash = dict->hf (key);
	
	/* Key is not presented? */
	node = (dict_node_t *) hash_test (dict->h, key, hash);
	if (node == NULL) {
		*value = NULL;
		error ("key doesn't exist.");

		return NULL;
	}
	
	origin_key = node->first;
	*value = node->second;

	hash_fast_remove (dict->h, node->hn);
	dict->size--;
	/* Need rehash? */
	occupied = hash_occupied (dict->h, hash);
	if (occupied == 0) {
		dict->used--;
		if (dict_rehash (dict, dict->used) == 0) {
			pool_free ((void *) node);
			
			return NULL;
		}
	}

	pool_free ((void *) node);

	return origin_key;
}

size_t
dict_size (dict_t *dict)
{
	return dict->size;
}

vec_t *
dict_pairs (dict_t *dict)
{
	return hash_get_all_values (dict->h);
}

