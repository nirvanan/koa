/*
 * object.c
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

#include "object.h"
#include "pool.h"
#include "error.h"

#define BYTE_CACHE_SIZE ((size_t)1<<sizeof(byte_t))

#define INT_CACHE_MIN -1000
#define INT_CACHE_MAX 10000
#define INT_CACHE_SIZE ((size_t)INT_CACHE_MAX-INT_CACHE_MIN+1)

#define INT_2_CACHE_IDX(x) ((size_t)x-INT_CACHE_MIN)

#define DUMP_BUFF_SIZE (2*1024*1024)

/* All byte objects should be cached. */
static object_t *g_byte_cache[BYTE_CACHE_SIZE];

/* Small ints should be cached. */
static object_t *g_int_cache[INT_CACHE_SIZE];

/* For big objects more than DUMP_BUFF_SIZE, write your own dumping code. */
static char g_dump_buffer[DUMP_BUFF_SIZE + 1];

static object_t *
object_new (object_type_t t, void *udata)
{
	object_t *ob;

	ob = (object_t *) pool_alloc (sizeof (object_t));
	ob->head.ref = 0;
	ob->head.type = t;
	ob->udata = udata;

	return ob;
}

object_t *
object_byte_new (byte_t b, void *udata)
{
	object_t *ob;

	if (g_byte_cache[b] != NULL) {
		return g_byte_cache[b];
	}
	ob = object_new (OBJECT_TYPE_BYTE, udata);
	ob->value.b = b;

	return ob;
}

object_t *
object_int_new (const int i, void *udata)
{
	object_t *ob;

	if (i >= INT_CACHE_MIN && i <= INT_CACHE_MAX
		&& g_int_cache[INT_2_CACHE_IDX (i)] != NULL) {
		return g_int_cache[INT_2_CACHE_IDX (i)];
	}
	ob = object_new (OBJECT_TYPE_INT, udata);
	ob->value.i = i;

	return ob;
}

object_t *
object_float_new (const float f, void *udata)
{
	object_t *ob;

	ob = object_new (OBJECT_TYPE_FLOAT, udata);
	ob->value.f = f;

	return ob;
}

object_t *
object_double_new (const double d, void *udata)
{
	object_t *ob;

	ob = object_new (OBJECT_TYPE_DOUBLE, udata);
	ob->value.d = d;

	return ob;
}

object_t *
object_str_new (const char *s, void *udata)
{
	object_t *ob;

	ob = object_new (OBJECT_TYPE_STR, udata);
	ob->value.s = str_new (s);

	return ob;
}

void
object_ref (object_t *ob)
{
	ob->head.ref++;
}

void
object_unref (object_t *ob)
{
	ob->head.ref--;
	if (ob->head.ref <= 0) {
		object_free (ob);
	}
}

void
object_free (object_t *ob)
{
	/* Refed objects should not be freed. */
	if (ob->head.ref > 0) {
		return;
	}
	switch (ob->head.type) {
		case OBJECT_TYPE_BYTE:
		case OBJECT_TYPE_INT:
		case OBJECT_TYPE_FLOAT:
		case OBJECT_TYPE_DOUBLE: 
		{
			pool_free (ob);
			break;
		}
		case OBJECT_TYPE_STR:
		{
			str_free (ob->value.s);
			pool_free (ob);
			break;
		}
		case OBJECT_TYPE_VEC:
		{
			pool_free (ob);
			break;
		}
		case OBJECT_TYPE_DICT:
		{
			pool_free (ob);
			break;
		}
		case OBJECT_TYPE_RAW:
		{
			pool_free (ob);
			break;
		}
	}
}

void
object_init ()
{
	/* Init byte cache. */
	for (int i = 0; i < BYTE_CACHE_SIZE; i++) {
		g_byte_cache[i] = object_byte_new ((byte_t) i, NULL);
		/* Never recycle it. */
		g_byte_cache[i]->head.ref = 1;
	}
	/* Init int cache. */
	for (int i = INT_CACHE_MIN; i <= INT_CACHE_MAX; i++) {
		g_int_cache[INT_2_CACHE_IDX (i)] = object_int_new (i, NULL);
		/* Never recycle it. */
		g_int_cache[INT_2_CACHE_IDX (i)]->head.ref = 1;
	}
	snprintf (g_dump_buffer, DUMP_BUFF_SIZE, "null");
}
