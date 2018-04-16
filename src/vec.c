/*
 * vec.c
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

#include "vec.h"
#include "pool.h"

#define VEC_REQ_SIZE 2 /* Initial allocation size. */

vec_t *
vec_new (size_t size)
{
	size_t req;
	vec_t *vec;

	req = size;
	if (req < VEC_REQ_SIZE) {
		req = VEC_REQ_SIZE;
	}
	
	vec = pool_alloc (sizeof (vec_t));
	if (vec == NULL) {
		return NULL;
	}
	vec->size = size;
	vec->allocated = req;
	vec->v = (void **) pool_calloc (req, sizeof (void *));
	if (vec->v == NULL) {
		pool_free (vec);

		return NULL;
	}

	return vec;
}

void
vec_free (vec_t *vec)
{
	pool_free ((void *) vec->v);
	pool_free ((void *) vec);
}

size_t
vec_size (vec_t *vec)
{
	return vec->size;
}

size_t
vec_capacity (vec_t *str)
{
	return vec->allocated;
}

vec_t *
vec_concat (vec_t *vec1, vec_t *vec2)
{
	vec_t *new_vec;
	size_t new_size;

	new_size = vec1->size + vec2->size;
	new_vec = vec_new (new_size);
	memcpy ((void *) new_vec->v, (void *) vec1->v,
			vec1->size * sizeof (void *));
	memcpy ((void *) new_vec->v + vec1->size, (void *) vec2->v,
			vec2->size * sizeof (void *));

	return new_vec;
}

void *
vec_pos (vec_t *vec, integer_value_t pos)
{
	if (pos >= 0 && pos < vec->size) {
		return vec->v[pos];
	}

	return NULL;
}

static int
vec_resize (vec_t *vec, size_t req)
{
	size_t new_req;
	void **new_v;

	if (req > vec->allocated) {
		new_req = vec->allocated * 2;
	}
	else if (req < vec->allocated / 2) {
		new_req = vec->allocated / 2;
		if (new_req < VEC_REQ_SIZE) {
			new_req = VEC_REQ_SIZE;
		}
	}

	if (new_req != vec->allocated) {
		new_v = pool_alloc (new_req * sizeof (void *));
		if (new_v == NULL) {
			return 0;
		}
		
		memcpy (new_v, vec->v, vec->size * sizeof (void *));
		pool_free (vec->v);
		vec->v = new_v;
		vec->allocated = new_req;

		return 1;
	}

	return 1;
}

int
vec_push_back (vec_t *vec, void *data)
{
	/* Need resizing? */
	if (vec->size >= vec->allocated) {
		if (vec_resize (vec, vec->size + 1) == 0) {
			return 0;
		}
	}

	vec->v[vec->size++] = data;

	return 1;
}

int
vec_pop_back (vec_t *vec)
{
	if (vec->size <= 0) {
		error ("vec is empty.");

		return 0;
	}

	if (vec->size < vec->allocated / 4) {
		if (vec_resize (vec, vec->allocated / 2) == 0) {
			return 0;
		}
	}

	vec->v[vec->size-- - 1] = NULL;
	
	return 0;
}
