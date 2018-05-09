/*
 * hash.c
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

#include <string.h>

#include "hash.h"
#include "pool.h"
#include "error.h"

typedef struct hash_find_foreach_s
{
	void *data;
	void *res;
} hash_find_foreach_t;

typedef struct hash_test_foreach_s
{
	hash_t *ha;
	void *data; /* Heterogeneous data. */
} hash_test_foreach_t;

typedef struct hash_collect_foreach_s
{
	vec_t *vec;
	int *pos;
} hash_collect_foreach_t;

typedef struct hash_remove_cleanup_s
{
	hash_t *ha;
	void *data;
} hash_remove_cleanup_t;

hash_t *
hash_new (size_t bu, hash_f hf, hash_test_f tf)
{
	hash_t *ha;
	size_t bucket_size;

	ha = pool_alloc (sizeof (hash_t));
	if (ha == NULL) {
		error ("out of memory.");

		return NULL;
	}

	bucket_size = bu * sizeof (list_t *);
	/* We can still take use of pool allocation when bu is small. */
	ha->h = (list_t **) pool_calloc (bucket_size, sizeof (list_t *));
	if (ha->h == NULL) {
		pool_free ((void *) ha);
		error ("out of memory.");

		return NULL;
	}
	ha->hf = hf;
	ha->tf = tf;
	ha->bu = bu;
	ha->size = 0;

	return ha;
}

/* Used to delete whole hash bucket list. */
static int
hash_clear_fun (list_t *list, void *data)
{
	hash_t *ha;

	UNUSED (list);
	ha = (hash_t *) data;

	ha->size--;

	return 1;
}

void
hash_free (hash_t *ha)
{
	for (size_t i = 0; i < ha->bu; i++) {
		ha->h[i] = list_cleanup (ha->h[i], hash_clear_fun, 1, (void *) ha);
	}

	pool_free ((void *) ha->h);
	pool_free ((void *) ha);
}

void *
hash_add (hash_t *ha, void *data)
{
	size_t idx;
	hash_node_t *node;

	idx = ha->hf (data) % ha->bu;
	/* Check whether there is already a node presenting the same value. */
	if (hash_find (ha, data) > 0) {
		return NULL;
	}

	node = pool_alloc (sizeof (hash_node_t));
	if (node == NULL) {
		return NULL;
	}

	node->value = data;
	node->idx = idx;

	ha->h[idx] = list_append (ha->h[idx], LIST (node));
	ha->size++;

	return (void *) node;
}

void
hash_fast_add (hash_t *ha, void *hn)
{
	size_t idx;
	hash_node_t *node;

	node = (hash_node_t *) hn;
	idx = node->idx;

	ha->h[idx] = list_append (ha->h[idx], LIST (node));
}

/* Cleanup function for list cleanup routine. */
static int
hash_remove_fun (list_t *list, void *data)
{
	hash_node_t *node;
	hash_remove_cleanup_t *remove;

	node = (hash_node_t *) list;
	remove = (hash_remove_cleanup_t *) data;

	remove->ha->size--;
	
	return (int) (node->value == remove->data);
}

void
hash_remove (hash_t *ha, void *data)
{
	size_t idx;
	hash_remove_cleanup_t remove;

	idx = ha->hf (data) % ha->bu;
	remove.ha = ha;
	remove.data = data;
	ha->h[idx] = list_cleanup (ha->h[idx], hash_remove_fun, 1, (void *) &remove);
}

void
hash_fast_remove (hash_t *ha, void *hn)
{
	size_t idx;
	hash_node_t *node;

	node = (hash_node_t *) hn;
	idx = node->idx;

	ha->h[idx] = list_remove (ha->h[idx], LIST (node));

	pool_free ((void *) node);
}

/* Test function for list finding routine. */
static int
hash_test_fun (list_t *list, void *data)
{
	hash_node_t *node;
	hash_find_foreach_t *test;
	hash_test_foreach_t *test_info;

	node = (hash_node_t *) list;
	test = (hash_find_foreach_t *) data;
	test_info = (hash_test_foreach_t *) test->data;
	if (test_info->ha->tf (node->value, test_info->data) != 0) {
		test->res = node->value;

		return 1;
	}

	return 0;
}

/* This function is used for finding heterogeneous data. */
void *
hash_test (hash_t *ha, void *hd, uint64_t hash)
{
	size_t idx;
	hash_find_foreach_t test;
	hash_test_foreach_t test_info;

	idx = (size_t) hash % ha->bu;
	test_info.ha = ha;
	test_info.data = hd;
	test.data = (void *) &test_info;
	test.res = NULL;

	list_foreach (ha->h[idx], hash_test_fun, (void *) &test);

	return test.res;
}

/* Find function for list finding routine. */
static int
hash_find_fun (list_t *list, void *data)
{
	hash_node_t *node;
	hash_find_foreach_t *find;

	node = (hash_node_t *) list;
	find = (hash_find_foreach_t *) data;
	if (node->value == find->data) {
		*((int *) find->res) = 1;

		return 1;
	}

	return 0;
}

int
hash_find (hash_t *ha, void *data)
{
	size_t idx;
	hash_find_foreach_t find;
	int res;

	idx = (size_t) ha->hf (data) % ha->bu;
	find.data = data;
	res = 0;
	find.res = (void *) &res;

	list_foreach (ha->h[idx], hash_find_fun, (void *) &find);

	return res;
}

int
hash_occupied (hash_t *ha, uint64_t hash)
{
	size_t idx;

	idx = (size_t) hash % ha->bu;

	return ha->h[idx] != NULL;
}

static int
hash_collect_foreach (list_t *list, void *udata)
{
	hash_node_t *node;
	hash_collect_foreach_t *collect;

	node = (hash_node_t *) list;
	collect = (hash_collect_foreach_t *) udata;

	UNUSED (vec_set (collect->vec, (*collect->pos)++, node->value));

	return 0;
}

vec_t *
hash_get_all_values (hash_t *ha)
{
	vec_t *vec;
	int pos;
	hash_collect_foreach_t collect;

	vec = vec_new (ha->size);
	if (vec == NULL) {
		return NULL;
	}

	pos = 0;
	collect.vec = vec;
	collect.pos = &pos;
	for (size_t i = 0; i < ha->bu; i++) {
		list_foreach (ha->h[i], hash_collect_foreach, (void *) &collect);
	}

	return vec;
}

void *
hash_handle_copy (void *hn)
{
	hash_node_t *node;

	node = (hash_node_t *) pool_alloc (sizeof (hash_node_t));
	if (node == NULL) {
		error ("out of memery.");

		return NULL;
	}

	memcpy ((void *) node, hn, sizeof (hash_node_t));

	return (void *) node;
}
