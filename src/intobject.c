/*
 * intobject.c
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

#include "intobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "longobject.h"
#include "strobject.h"

#define DUMP_BUF_SIZE 18

#define INT_CACHE_MIN -1000
#define INT_CACHE_MAX 10000
#define INT_CACHE_SIZE ((size_t)(INT_CACHE_MAX-INT_CACHE_MIN+1))

#define INT_HAS_CACHE(x) ((x)>=INT_CACHE_MIN&&(x)<=INT_CACHE_MAX)

#define INT_CACHE_INDEX(x) ((size_t)((x)-INT_CACHE_MIN))

/* Small ints should be cached. */
static object_t *g_int_cache[INT_CACHE_SIZE];

/* Object ops. */
static object_t *intobject_op_lnot (object_t *obj);
static object_t *intobject_op_dump (object_t *obj);
static object_t *intobject_op_neg (object_t *obj);
static object_t *intobject_op_add (object_t *obj1, object_t *obj2);
static object_t *intobject_op_sub (object_t *obj1, object_t *obj2);
static object_t *intobject_op_mul (object_t *obj1, object_t *obj2);
static object_t *intobject_op_mod (object_t *obj1, object_t *obj2);
static object_t *intobject_op_div (object_t *obj1, object_t *obj2);
static object_t *intobject_op_not (object_t *obj);
static object_t *intobject_op_and (object_t *obj1, object_t *obj2);
static object_t *intobject_op_or (object_t *obj1, object_t *obj2);
static object_t *intobject_op_xor (object_t *obj1, object_t *obj2);
static object_t *intobject_op_land (object_t *obj1, object_t *obj2);
static object_t *intobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *intobject_op_lshift (object_t *obj1, object_t *obj2);
static object_t *intobject_op_rshift (object_t *obj1, object_t *obj2);
static object_t *intobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *intobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *intobject_op_hash (object_t *obj);
static object_t *intobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	intobject_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	intobject_op_dump, /* Dump. */
	intobject_op_neg, /* Negative. */
	NULL, /* Call. */
	intobject_op_add, /* Addition. */
	intobject_op_sub, /* Substraction. */
	intobject_op_mul, /* Multiplication. */
	intobject_op_div, /* Division. */
	intobject_op_mod, /* Mod. */
	intobject_op_not, /* Bitwise not. */
	intobject_op_and, /* Bitwise and. */
	intobject_op_or, /* Bitwise or. */
	intobject_op_xor, /* Bitwise xor. */
	intobject_op_land, /* Logic and. */
	intobject_op_lor, /* Logic or. */
	intobject_op_lshift, /* Left shift. */
	intobject_op_rshift, /* Right shift. */
	intobject_op_eq, /* Equality. */
	intobject_op_cmp, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	intobject_op_hash, /* Hash. */
	intobject_op_binary /* Binary. */
};

/* Logic Not. */
static object_t *
intobject_op_lnot (object_t *obj)
{
	return boolobject_new (!intobject_get_value (obj), NULL);
}

/* Dump. */
static object_t *
intobject_op_dump (object_t *obj)
{
	char buf[DUMP_BUF_SIZE];

	snprintf (buf, DUMP_BUF_SIZE, "<int %d>", intobject_get_value (obj));

	return strobject_new (buf, strlen (buf), NULL);
}

/* Negative. */
static object_t *
intobject_op_neg (object_t *obj)
{
	int val;

	val = intobject_get_value (obj);

	return intobject_new (-val, NULL);
}

/* Addition. */
static object_t *
intobject_op_add (object_t *obj1, object_t *obj2)
{
	int val1;
	int val2;

	val1 = intobject_get_value (obj1);
	val2 = intobject_get_value (obj2);

	return intobject_new (val1 + val2, NULL);
}

/* Substraction. */
static object_t *
intobject_op_sub (object_t *obj1, object_t *obj2)
{
	int val1;
	int val2;

	val1 = intobject_get_value (obj1);
	val2 = intobject_get_value (obj2);

	return intobject_new (val1 - val2, NULL);
}

/* Multiplication. */
static object_t *
intobject_op_mul (object_t *obj1, object_t *obj2)
{
	int val1;
	int val2;

	val1 = intobject_get_value (obj1);
	val2 = intobject_get_value (obj2);

	return intobject_new (val1 * val2, NULL);
}


/* Division. */
static object_t *
intobject_op_div (object_t *obj1, object_t *obj2)
{
	int val1;
	int val2;

	val1 = intobject_get_value (obj1);
	val2 = intobject_get_value (obj2);

	return intobject_new (val1 / val2, NULL);
}

/* Mod. */
static object_t *
intobject_op_mod (object_t *obj1, object_t *obj2)
{
	int val1;
	int val2;

	val1 = intobject_get_value (obj1);
	val2 = intobject_get_value (obj2);

	return intobject_new (val1 % val2, NULL);
}

/* Bitwise not. */
static object_t *
intobject_op_not (object_t *obj)
{
	int val;

	val = intobject_get_value (obj);

	return intobject_new (~val, NULL);
}

/* Bitwise and. */
static object_t *
intobject_op_and (object_t *obj1, object_t *obj2)
{
	int val1;
	int val2;

	val1 = intobject_get_value (obj1);
	val2 = intobject_get_value (obj2);

	return intobject_new (val1 & val2, NULL);
}

/* Bitwise or. */
static object_t *
intobject_op_or (object_t *obj1, object_t *obj2)
{
	int val1;
	int val2;

	val1 = intobject_get_value (obj1);
	val2 = intobject_get_value (obj2);

	return intobject_new (val1 | val2, NULL);
}

/* Bitwise xor. */
static object_t *
intobject_op_xor (object_t *obj1, object_t *obj2)
{
	int val1;
	int val2;

	val1 = intobject_get_value (obj1);
	val2 = intobject_get_value (obj2);

	return intobject_new (val1 ^ val2, NULL);
}

/* Logic and. */
static object_t *
intobject_op_land (object_t *obj1, object_t *obj2)
{
	int val1;

	val1 = intobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
intobject_op_lor (object_t *obj1, object_t *obj2)
{
	int val1;

	val1 = intobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Left shift. */
static object_t *
intobject_op_lshift (object_t *obj1, object_t *obj2)
{
	int val1;
	integer_value_t val2;

	val1 = intobject_get_value (obj1);
	val2 = object_get_integer (obj2);

	return intobject_new (val1 << val2, NULL);
}

/* Right shift. */
static object_t *
intobject_op_rshift (object_t *obj1, object_t *obj2)
{
	int val1;
	integer_value_t val2;

	val1 = intobject_get_value (obj1);
	val2 = object_get_integer (obj2);

	return intobject_new (val1 >> val2, NULL);
}

/* Equality. */
static object_t *
intobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		int val1;
	
		val1 = intobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}
	
	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
intobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
intobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_integer_hash (object_get_integer (obj));
	}

	return longobject_new ((long) OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
intobject_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((intobject_t *) obj)->val),
						  sizeof (int), NULL);
}

object_t *
intobject_new (int val, void *udata)
{
	intobject_t *obj;

	/* Return cached object. */
	if (INT_HAS_CACHE (val) && g_int_cache[INT_CACHE_INDEX (val)] != NULL) {
		return g_int_cache[INT_CACHE_INDEX (val)];
	}

	obj = (intobject_t *) pool_alloc (sizeof (intobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_INT);

	obj->val = val;

	return (object_t *) obj;
}

int
intobject_get_value (object_t *obj)
{
	intobject_t *ob;

	ob = (intobject_t *) obj;

	return ob->val;
}

void
intobject_init ()
{
	/* Make small int cache. */
	for (int i = INT_CACHE_MIN; i <= INT_CACHE_MAX; i++) {
		g_int_cache[INT_CACHE_INDEX (i)] = intobject_new (i, NULL);
		if (g_int_cache[INT_CACHE_INDEX (i)] == NULL) {
			fatal_error ("failed to init object system.");
	
			return;
		}

		/* Should never be freed. */
		object_ref (g_int_cache[INT_CACHE_INDEX (i)]);
	}
}
