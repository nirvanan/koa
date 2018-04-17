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
#include <limits.h>

#include "vec.h"
#include "pool.h"
#include "error.h"

#define VEC_REQ_SIZE 2 /* Initial allocation size. */

#define MAX_VEC_SIZE INT_MAX

vec_t *
vec_new (size_t size)
{
	size_t req;
	vec_t *vec;

	if (size > MAX_VEC_SIZE) {
		error ("vec too big.");

		return NULL;
	}

	req = size;
	if (req < VEC_REQ_SIZE) {
		req = VEC_REQ_SIZE;
	}
	
	vec = pool_alloc (sizeof (vec_t));
	if (vec == NULL) {
		error ("out of memery.");

		return NULL;
	}

	vec->size = size;
	vec->allocated = req;
	vec->v = (void **) pool_calloc (req, sizeof (void *));
	if (vec->v == NULL) {
		pool_free ((void *) vec);
		error ("out of memery.");

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
vec_capacity (vec_t *vec)
{
	return vec->allocated;
}

vec_t *
vec_concat (vec_t *vec1, vec_t *vec2)
{
	vec_t *new_vec;
	size_t new_size;

	new_size = vec1->size + vec2->size;
	if (new_size >= MAX_VEC_SIZE) {
		error ("vec too big.");

		return NULL;
	}

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
vec_check_and_resize (vec_t *vec, size_t req)
{
	size_t new_req;
	void **new_v;

	if (req > MAX_VEC_SIZE) {
		error ("vec too big.");

		return 0;
	}

	new_req = vec->allocated;
	if (req > vec->allocated && vec->allocated < MAX_VEC_SIZE) {
		new_req = vec->allocated * 2;
		if (new_req > MAX_VEC_SIZE) {
			new_req = MAX_VEC_SIZE;
		}
	}
	else if (req < vec->allocated / 4) {
		new_req = vec->allocated / 2;
		if (new_req < VEC_REQ_SIZE) {
			new_req = VEC_REQ_SIZE;
		}
	}

	if (new_req != vec->allocated) {
		new_v = pool_alloc (new_req * sizeof (void *));
		if (new_v == NULL) {
			error ("out of memory.");

			return 0;
		}
		
		memcpy (new_v, vec->v, vec->size * sizeof (void *));
		pool_free ((void *) vec->v);
		vec->v = new_v;
		vec->allocated = new_req;

		return 1;
	}

	return 1;
}

int
vec_push_back (vec_t *vec, void *data)
{
	return vec_insert (vec, vec->size, data);
}

int
vec_pop_back (vec_t *vec)
{
	return vec_remove (vec, vec->size - 1);
}

int
vec_push_front (vec_t *vec, void *data)
{
	return vec_insert (vec, 0, data);
}

int
vec_pop_front (vec_t *vec)
{
	return vec_remove (vec, 0);
}

void *
vec_first (vec_t *vec)
{
	/* It is valid to get the first item of an empty vec. */
	if (vec->size <= 0) {
		return NULL;
	}

	return vec->v[0];
}

void *
vec_last (vec_t *vec)
{
	/* It is valid to get the last item of an empty vec. */
	if (vec->size <= 0) {
		return NULL;
	}

	return vec->v[vec->size - 1];
}

/* Int is enough. */
int
vec_find (vec_t *vec, void *data)
{
	for (size_t i = 0; i < vec->size; i++) {
		if (vec->v[i] == data) {
			return i;
		}
	}

	return -1;
}

int
vec_insert (vec_t *vec, int pos, void *data)
{
	if (pos < 0 || pos > vec->size) {
		error ("invalid vec pos for inserting.");

		return 0;
	}

	/* Need resizing? */
	if (vec_check_and_resize (vec, vec->size + 1) == 0) {
		return 0;
	}

	for (size_t i = vec->size; i > pos; i++) {
		vec->v[i] = vec->v[i - 1];
	}
	vec->v[pos] = data;
	vec->size++;

	return 1;
}

int
vec_remove (vec_t *vec, int pos)
{
	void *to_del;

	if (pos < 0 || pos >= vec->size) {
		error ("invalid vec pos for removing.");

		return 0;
	}

	to_del = vec->v[pos];
	/* Do not use memcpy even for moving forward. */
	for (size_t i = pos; i < vec->size; i++) {
		vec->v[i] = vec->v[i + 1];
	}

	if (vec_check_and_resize (vec, vec->size - 1) == 0) {
		/* Rollback. */
		for (size_t i = vec->size - 1; i > pos; i++) {
			vec->v[i] = vec->v[i - 1];
		}
		vec->v[pos] = to_del;

		return 0;
	}

	vec->size--;

	return 1;
}
