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

hash_t *
hash_new (size_t bu, hash_f hf, hash_eq_f ef, hash_test_f tf)
{
	hash_t *ha;
	size_t bucket_size;

	ha = pool_alloc (sizeof (hash_t));
	if (ha == NULL) {
		error ("out of memory.");

		return NULL;
	}

	bucket_size = bu * sizeof (list_t *);
	/* We can still use pool allocation when bu is small. */
	ha->h = (list_t **) pool_alloc (bucket_size);
	if (ha->h == NULL) {
		pool_free ((void *) ha);
		error ("out of memory.");

		return NULL;
	}
	memset (ha->h, 0, bucket_size);
	ha->hf = hf;
	ha->ef = ef;
	ha->tf = tf;
	ha->bu = bu;

	return ha;
}

void
hash_add (hash_t *ha, void *data)
{
	int idx;
	list_t *l;
	hash_node_t *node;

	idx = ha->hf (data) % ha->bu;
	/* Check whether there is already a node presenting the same value. */
	for (l = ha->h[idx]; l; l = LIST_NEXT (l)) {
		node = (hash_node_t *) l;
		if (ha->ef (node->value, data)) {
			return;
		}
	}
	node = pool_alloc (sizeof (hash_node_t));
	ha->h[idx] = list_append (ha->h[idx], LIST (node));
}

void
hash_remove (hash_t *ha, void *data)
{
	int idx;
	list_t *l;
	hash_node_t *node;

	idx = ha->hf (data) % ha->bu;
	l = ha->h[idx];
	while (l != NULL) {
		node = (hash_node_t *) l;
		if (ha->ef (node->value, data)) {
			ha->h[idx] = list_remove (ha->h[idx], l);
			break;
		}

		l = LIST_NEXT (l);
	}
}

/* This function is used for heterogeneous data. */
int
hash_test (hash_t *ha, void *hd, int hash)
{
	int idx;
	list_t *l;
	hash_node_t *node;

	idx = hash % ha->bu;
	l = ha->h[idx];
	while (l != NULL) {
		node = (hash_node_t *) l;
		if (ha->tf (node->value, hd)) {
			return 1;
		}

		l = LIST_NEXT (l);
	}

	return 0;
}

int
hash_find (hash_t *ha, void *data)
{
	int idx;
	list_t *l;
	hash_node_t *node;

	idx = ha->hf (data) % ha->bu;
	l = ha->h[idx];
	while (l != NULL) {
		node = (hash_node_t *) l;
		if (ha->ef (node->value, data)) {
			return 1;
		}

		l = LIST_NEXT (l);
	}

	return 0;
}
