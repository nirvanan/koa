/*
 * ulongobject.c
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

#include "ulongobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "intobject.h"
#include "uint64object.h"
#include "strobject.h"

#define DUMP_BUF_SIZE 18

/* Object ops. */
static object_t *ulongobject_op_lnot (object_t *obj);
static void ulongobject_op_print (object_t *obj);
static object_t *ulongobject_op_dump (object_t *obj);
static object_t *ulongobject_op_neg (object_t *obj);
static object_t *ulongobject_op_add (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_sub (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_mul (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_mod (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_div (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_not (object_t *obj);
static object_t *ulongobject_op_and (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_or (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_xor (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_land (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_lshift (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_rshift (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *ulongobject_op_hash (object_t *obj);
static object_t *ulongobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	ulongobject_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	ulongobject_op_print, /* Print. */
	ulongobject_op_dump, /* Dump. */
	ulongobject_op_neg, /* Negative. */
	NULL, /* Call. */
	ulongobject_op_add, /* Addition. */
	ulongobject_op_sub, /* Substraction. */
	ulongobject_op_mul, /* Multiplication. */
	ulongobject_op_div, /* Division. */
	ulongobject_op_mod, /* Mod. */
	ulongobject_op_not, /* Bitwise not. */
	ulongobject_op_and, /* Bitwise and. */
	ulongobject_op_or, /* Bitwise or. */
	ulongobject_op_xor, /* Bitwise xor. */
	ulongobject_op_land, /* Logic and. */
	ulongobject_op_lor, /* Logic or. */
	ulongobject_op_lshift, /* Left shift. */
	ulongobject_op_rshift, /* Right shift. */
	ulongobject_op_eq, /* Equality. */
	ulongobject_op_cmp, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	ulongobject_op_hash, /* Hash. */
	ulongobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Logic Not. */
static object_t *
ulongobject_op_lnot (object_t *obj)
{
	return boolobject_new (!ulongobject_get_value (obj), NULL);
}

/* Print. */
static void
ulongobject_op_print (object_t *obj)
{
	printf ("%lu", ulongobject_get_value (obj));
}

/* Dump. */
static object_t *
ulongobject_op_dump (object_t *obj)
{
	char buf[DUMP_BUF_SIZE];

	snprintf (buf, DUMP_BUF_SIZE, "<ulong %lu>", ulongobject_get_value (obj));

	return strobject_new (buf, strlen (buf), 1, NULL);
}

/* Negative. */
static object_t *
ulongobject_op_neg (object_t *obj)
{
	unsigned long int val;

	val = ulongobject_get_value (obj);

	return ulongobject_new (-val, NULL);
}

/* Addition. */
static object_t *
ulongobject_op_add (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	unsigned long int val2;

	val1 = ulongobject_get_value (obj1);
	val2 = ulongobject_get_value (obj2);

	return ulongobject_new (val1 + val2, NULL);
}

/* Substraction. */
static object_t *
ulongobject_op_sub (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	unsigned long int val2;

	val1 = ulongobject_get_value (obj1);
	val2 = ulongobject_get_value (obj2);

	return ulongobject_new (val1 - val2, NULL);
}

/* Multiplication. */
static object_t *
ulongobject_op_mul (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	unsigned long int val2;

	val1 = ulongobject_get_value (obj1);
	val2 = ulongobject_get_value (obj2);

	return ulongobject_new (val1 * val2, NULL);
}


/* Division. */
static object_t *
ulongobject_op_div (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	unsigned long int val2;

	val1 = ulongobject_get_value (obj1);
	val2 = ulongobject_get_value (obj2);

	return ulongobject_new (val1 / val2, NULL);
}

/* Mod. */
static object_t *
ulongobject_op_mod (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	unsigned long int val2;

	val1 = ulongobject_get_value (obj1);
	val2 = ulongobject_get_value (obj2);

	return ulongobject_new (val1 % val2, NULL);
}

/* Bitwise not. */
static object_t *
ulongobject_op_not (object_t *obj)
{
	unsigned long int val;

	val = ulongobject_get_value (obj);

	return ulongobject_new (~val, NULL);
}

/* Bitwise and. */
static object_t *
ulongobject_op_and (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	unsigned long int val2;

	val1 = ulongobject_get_value (obj1);
	val2 = ulongobject_get_value (obj2);

	return ulongobject_new (val1 & val2, NULL);
}

/* Bitwise or. */
static object_t *
ulongobject_op_or (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	unsigned long int val2;

	val1 = ulongobject_get_value (obj1);
	val2 = ulongobject_get_value (obj2);

	return ulongobject_new (val1 | val2, NULL);
}

/* Bitwise xor. */
static object_t *
ulongobject_op_xor (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	unsigned long int val2;

	val1 = ulongobject_get_value (obj1);
	val2 = ulongobject_get_value (obj2);

	return ulongobject_new (val1 ^ val2, NULL);
}

/* Logic and. */
static object_t *
ulongobject_op_land (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;

	val1 = ulongobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
ulongobject_op_lor (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;

	val1 = ulongobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Left shift. */
static object_t *
ulongobject_op_lshift (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	integer_value_t val2;

	val1 = ulongobject_get_value (obj1);
	val2 = object_get_integer (obj2);

	return ulongobject_new (val1 << val2, NULL);
}

/* Right shift. */
static object_t *
ulongobject_op_rshift (object_t *obj1, object_t *obj2)
{
	unsigned long int val1;
	integer_value_t val2;

	val1 = ulongobject_get_value (obj1);
	val2 = object_get_integer (obj2);

	return ulongobject_new (val1 >> val2, NULL);
}

/* Equality. */
static object_t *
ulongobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		unsigned long int val1;

		val1 = ulongobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}

	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
ulongobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
ulongobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_integer_hash (object_get_integer (obj));
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
ulongobject_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((ulongobject_t *) obj)->val),
						  sizeof (unsigned long int), 1, NULL);
}

object_t *
ulongobject_load_binary (FILE *f)
{
	unsigned long int val;

	if (fread (&val, sizeof (unsigned long int), 1, f) != 1) {
		error ("failed to load int binary.");

		return NULL;
	}

	return ulongobject_new (val, NULL);
}

object_t *
ulongobject_load_buf (const char **buf, size_t *len)
{
	unsigned long int val;

	if (*len < sizeof (unsigned long int)) {
		error ("failed to load ulong buffer.");

		return NULL;
	}

	val = *(unsigned long int *) *buf;
	*buf += sizeof (unsigned long int);
	*len -= sizeof (unsigned long int);

	return ulongobject_new (val, NULL);
}

static uint64_t
ulongobject_digest_fun (void *obj)
{
	return object_integer_hash (object_get_integer ((object_t *) obj));
}

object_t *
ulongobject_new (unsigned long int val, void *udata)
{
	ulongobject_t *obj;

	obj = (ulongobject_t *) pool_alloc (sizeof (ulongobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_ULONG, udata);
	OBJECT_DIGEST_FUN (obj) = ulongobject_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

unsigned long int
ulongobject_get_value (object_t *obj)
{
	ulongobject_t *ob;

	ob = (ulongobject_t *) obj;

	return ob->val;
}
