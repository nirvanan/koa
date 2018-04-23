/*
 * dict.h
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

#ifndef DICT_H
#define DICT_H

#include "koa.h"
#include "hash.h"

typedef struct dict_s {
	hash_t *h;
	hash_f hf;
	hash_test_f tf;
	size_t used; /* Hash buckets used. */
	size_t bu_size; /* Number of hash buckets. */
	size_t size;
} dict_t;

typedef struct dict_node_s
{
	dict_t *ha;
	void *first;
	void *second;
} dict_node_t;

dict_t *
dict_new (hash_f hf, hash_test_f tf);

void
dict_free (dict_t *dict);

void *
dict_set (dict_t *dict, void *key, void *value);

void *
dict_get (dict_t *dict, void *key);

void *
dict_remove (dict_t *dict, void *key, void **value);

size_t
dict_size (dict_t *dict);

#endif /* DICT_H */
