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

#include "koa.h"

typedef int (*hash_f) (void *data);
typedef int (*hash_eq_f) (void *value, void *data);
typedef int (*hash_test_f) (void *value, void *hd);

typedef struct hash_node_s
{
	struct hash_node_s *prev;
	struct hash_node_s *next;
	void *value;
} hash_node_t;

typedef struct hash_s
{
	hash_node_t **h;
	hash_f hf;
	hash_eq_f ef;
	hash_test_f tf;
	size_t bu;
} hash_t;

hash_t *
hash_new (size_t bu, hash_f hf, hash_eq_f ef, hash_test_f tf);

void
hash_add (hash_t *ha, void *data);

void
hash_remove (hash_t *ha, void *data);

int
hash_test (hash_t *ha, void *hd, int hash);

int
hash_find (hash_t *ha, void *data);

#endif /* HASH_H */
