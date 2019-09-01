/*
 * longobject.c
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

#include "longobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "intobject.h"
#include "strobject.h"

#define DUMP_BUF_SIZE 28

#define LONG_CACHE_MIN -1000
#define LONG_CACHE_MAX 10000
#define LONG_CACHE_SIZE ((size_t)(LONG_CACHE_MAX-LONG_CACHE_MIN+1))

#define LONG_HAS_CACHE(x) ((x)>=LONG_CACHE_MIN&&(x)<=LONG_CACHE_MAX)

#define LONG_CACHE_INDEX(x) ((size_t)((x)-LONG_CACHE_MIN))

/* Small long should be cached. */
static object_t *g_long_cache[LONG_CACHE_SIZE];

/* Object ops. */
static object_t *longobject_op_lnot (object_t *obj);
static void longobject_op_print (object_t *obj);
static object_t *longobject_op_dump (object_t *obj);
static object_t *longobject_op_neg (object_t *obj);
static object_t *longobject_op_add (object_t *obj1, object_t *obj2);
static object_t *longobject_op_sub (object_t *obj1, object_t *obj2);
static object_t *longobject_op_mul (object_t *obj1, object_t *obj2);
static object_t *longobject_op_mod (object_t *obj1, object_t *obj2);
static object_t *longobject_op_div (object_t *obj1, object_t *obj2);
static object_t *longobject_op_not (object_t *obj);
static object_t *longobject_op_and (object_t *obj1, object_t *obj2);
static object_t *longobject_op_or (object_t *obj1, object_t *obj2);
static object_t *longobject_op_xor (object_t *obj1, object_t *obj2);
static object_t *longobject_op_land (object_t *obj1, object_t *obj2);
static object_t *longobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *longobject_op_lshift (object_t *obj1, object_t *obj2);
static object_t *longobject_op_rshift (object_t *obj1, object_t *obj2);
static object_t *longobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *longobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *longobject_op_hash (object_t *obj);
static object_t *longobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	longobject_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	longobject_op_print, /* Print. */
	longobject_op_dump, /* Dump. */
	longobject_op_neg, /* Negative. */
	NULL, /* Call. */
	longobject_op_add, /* Addition. */
	longobject_op_sub, /* Substraction. */
	longobject_op_mul, /* Multiplication. */
	longobject_op_div, /* Division. */
	longobject_op_mod, /* Mod. */
	longobject_op_not, /* Bitwise not. */
	longobject_op_and, /* Bitwise and. */
	longobject_op_or, /* Bitwise or. */
	longobject_op_xor, /* Bitwise xor. */
	longobject_op_land, /* Logic and. */
	longobject_op_lor, /* Logic or. */
	longobject_op_lshift, /* Left shift. */
	longobject_op_rshift, /* Right shift. */
	longobject_op_eq, /* Equality. */
	longobject_op_cmp, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	longobject_op_hash, /* Hash. */
	longobject_op_binary /* Binary. */
};

/* Logic Not. */
static object_t *
longobject_op_lnot (object_t *obj)
{
	return boolobject_new (!longobject_get_value (obj), NULL);
}

/* Print. */
static void
longobject_op_print (object_t *obj)
{
	printf ("%ld", longobject_get_value (obj));
}

/* Dump. */
static object_t *
longobject_op_dump (object_t *obj)
{
	char buf[DUMP_BUF_SIZE];

	snprintf (buf, DUMP_BUF_SIZE, "<int %ld>", longobject_get_value (obj));

	return strobject_new (buf, strlen (buf), 1, NULL);
}

/* Negative. */
static object_t *
longobject_op_neg (object_t *obj)
{
	long val;

	val = longobject_get_value (obj);

	return longobject_new (-val, NULL);
}

/* Addition. */
static object_t *
longobject_op_add (object_t *obj1, object_t *obj2)
{
	long val1;
	long val2;

	val1 = longobject_get_value (obj1);
	val2 = longobject_get_value (obj2);

	return longobject_new (val1 + val2, NULL);
}

/* Substraction. */
static object_t *
longobject_op_sub (object_t *obj1, object_t *obj2)
{
	long val1;
	long val2;

	val1 = longobject_get_value (obj1);
	val2 = longobject_get_value (obj2);

	return intobject_new (val1 - val2, NULL);
}

/* Multiplication. */
static object_t *
longobject_op_mul (object_t *obj1, object_t *obj2)
{
	long val1;
	long val2;

	val1 = longobject_get_value (obj1);
	val2 = longobject_get_value (obj2);

	return longobject_new (val1 * val2, NULL);
}


/* Division. */
static object_t *
longobject_op_div (object_t *obj1, object_t *obj2)
{
	long val1;
	long val2;

	val1 = longobject_get_value (obj1);
	val2 = longobject_get_value (obj2);

	return longobject_new (val1 / val2, NULL);
}

/* Mod. */
static object_t *
longobject_op_mod (object_t *obj1, object_t *obj2)
{
	long val1;
	long val2;

	val1 = longobject_get_value (obj1);
	val2 = longobject_get_value (obj2);

	return longobject_new (val1 % val2, NULL);
}

/* Bitwise not. */
static object_t *
longobject_op_not (object_t *obj)
{
	long val;

	val = longobject_get_value (obj);

	return longobject_new (~val, NULL);
}

/* Bitwise and. */
static object_t *
longobject_op_and (object_t *obj1, object_t *obj2)
{
	long val1;
	long val2;

	val1 = longobject_get_value (obj1);
	val2 = longobject_get_value (obj2);

	return longobject_new (val1 & val2, NULL);
}

/* Bitwise or. */
static object_t *
longobject_op_or (object_t *obj1, object_t *obj2)
{
	long val1;
	long val2;

	val1 = longobject_get_value (obj1);
	val2 = longobject_get_value (obj2);

	return longobject_new (val1 | val2, NULL);
}

/* Bitwise xor. */
static object_t *
longobject_op_xor (object_t *obj1, object_t *obj2)
{
	long val1;
	long val2;

	val1 = longobject_get_value (obj1);
	val2 = longobject_get_value (obj2);

	return longobject_new (val1 ^ val2, NULL);
}

/* Logic and. */
static object_t *
longobject_op_land (object_t *obj1, object_t *obj2)
{
	long val1;

	val1 = longobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
longobject_op_lor (object_t *obj1, object_t *obj2)
{
	long val1;

	val1 = longobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Left shift. */
static object_t *
longobject_op_lshift (object_t *obj1, object_t *obj2)
{
	long val1;
	integer_value_t val2;

	val1 = longobject_get_value (obj1);
	val2 = object_get_integer (obj2);

	return longobject_new (val1 << val2, NULL);
}

/* Right shift. */
static object_t *
longobject_op_rshift (object_t *obj1, object_t *obj2)
{
	int val1;
	integer_value_t val2;

	val1 = intobject_get_value (obj1);
	val2 = object_get_integer (obj2);

	return longobject_new (val1 >> val2, NULL);
}

/* Equality. */
static object_t *
longobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		long val1;
	
		val1 = longobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}
	
	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
longobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
longobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_integer_hash (object_get_integer (obj));
	}

	return longobject_new ((long) OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
longobject_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((longobject_t *) obj)->val),
						  sizeof (long), 1, NULL);
}

object_t *
longobject_load_binary (FILE *f)
{
	long val;

	if (fread (&val, sizeof (long), 1, f) != 1) {
		error ("failed to load long binary.");

		return NULL;
	}

	return longobject_new (val, NULL);
}

object_t *
longobject_new (long val, void *udata)
{
	longobject_t *obj;

	/* Return cached object. */
	if (LONG_HAS_CACHE (val) && g_long_cache[LONG_CACHE_INDEX (val)] != NULL) {
		return g_long_cache[LONG_CACHE_INDEX (val)];
	}

	obj = (longobject_t *) pool_alloc (sizeof (longobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_LONG);

	obj->val = val;

	return (object_t *) obj;
}

long
longobject_get_value (object_t *obj)
{
	longobject_t *ob;

	ob = (longobject_t *) obj;

	return ob->val;
}

void
longobject_init ()
{
	/* Make small int cache. */
	for (long i = LONG_CACHE_MIN; i <= LONG_CACHE_MAX; i++) {
		g_long_cache[LONG_CACHE_INDEX (i)] = longobject_new (i, NULL);
		if (g_long_cache[LONG_CACHE_INDEX (i)] == NULL) {
			fatal_error ("failed to init object system.");

			return;
		}

		/* Should never be freed. */
		object_ref (g_long_cache[LONG_CACHE_INDEX (i)]);
	}
}
