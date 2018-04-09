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

hash_t *
hash_new (size_t bu, hash_f hf, hash_eq_f ef, hash_test_f tf)
{
	hash_t *ha;
	size_t bucket_size;

	ha = pool_alloc (sizeof (hash_t));
	bucket_size = bu * sizeof (hash_node_t *);
	/* We can still use pool allocation when bu is small. */
	ha->h = (hash_node_t **) pool_alloc (bucket_size);
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
	hash_node_t *node;

	idx = ha->hf (data) % ha->bu;
	/* Check whether there is already a node presenting the same value. */
	for (node = ha->h[idx]; node; node = node->next) {
		if (ha->ef (node, data)) {
			return;
		}
	}
	node = pool_alloc (sizeof (hash_node_t));
	node->next = ha->h[idx];
	node->prev = NULL;
	if (ha->h[idx] != NULL) {
		ha->h[idx]->prev = node;
	}
	ha->h[idx] = node;
}

void
hash_remove (hash_t *ha, void *data)
{
	int idx;
	hash_node_t *node;

	idx = ha->hf (data) % ha->bu;
	node = ha->h[idx];
	while (node != NULL) {
		if (ha->ef (node, data)) {
			if (node->next != NULL) {
				node->next->prev = node->prev;
			}
			if (node->prev != NULL) {
				node->prev->next = node->next;
			}
			else {
				ha->h[idx] = node->next;
			}
			break;
		}

		node = node->next;
	}
}

/* This function is used for heterogeneous data. */
int
hash_test (hash_t *ha, void *hd, int hash)
{
	int idx;
	hash_node_t *node;

	idx = hash % ha->bu;
	node = ha->h[idx];
	while (node != NULL) {
		if (ha->tf (node->value, hd)) {
			return 1;
		}

		node = node->next;
	}

	return 0;
}

int
hash_find (hash_t *ha, void *data)
{
	int idx;
	hash_node_t *node;

	idx = ha->hf (data) % ha->bu;
	node = ha->h[idx];
	while (node != NULL) {
		if (ha->ef (node, data)) {
			return 1;
		}

		node = node->next;
	}

	return 0;
}
