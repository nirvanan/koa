/*
 * vec.h
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

#ifndef VEC_H
#define VEC_H

#include <stddef.h>

#include "koa.h"

typedef int (*vec_find_f) (void *a, void *b);
typedef int (*vec_foreach_f) (void *data);

typedef struct vec_s {
	size_t size;
	size_t allocated;
	void **v;
} vec_t;

vec_t *
vec_new (size_t size);

void
vec_free (vec_t *vec);

size_t
vec_size (vec_t *vec);

size_t
vec_capacity (vec_t *vec);

vec_t *
vec_concat (vec_t *vec1, vec_t *vec2);

void *
vec_pos (vec_t *vec, integer_value_t pos);

void *
vec_set (vec_t *vec, integer_value_t pos, void *data);

int
vec_push_back (vec_t *vec, void *data);

int
vec_pop_back (vec_t *vec);

int
vec_push_front (vec_t *vec, void *data);

int
vec_pop_front (vec_t *vec);

void *
vec_first (vec_t *vec);

void *
vec_last (vec_t *vec);

size_t
vec_find (vec_t *vec, void *data, vec_find_f ff);

int
vec_insert (vec_t *vec, integer_value_t pos, void *data);

int
vec_remove (vec_t *vec, integer_value_t pos);

void
vec_foreach (vec_t *vec, vec_foreach_f ff);

#endif /* VEC_H */
