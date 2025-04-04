/*
 * uint8object.c
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

#include "uint8object.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "intobject.h"
#include "uint64object.h"
#include "strobject.h"

#define DUMP_BUF_SIZE 18

/* Object ops. */
static object_t *uint8object_op_lnot (object_t *obj);
static void uint8object_op_print (object_t *obj);
static object_t *uint8object_op_dump (object_t *obj);
static object_t *uint8object_op_neg (object_t *obj);
static object_t *uint8object_op_add (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_sub (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_mul (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_mod (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_div (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_not (object_t *obj);
static object_t *uint8object_op_and (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_or (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_xor (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_land (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_lor (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_lshift (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_rshift (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_eq (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_cmp (object_t *obj1, object_t *obj2);
static object_t *uint8object_op_hash (object_t *obj);
static object_t *uint8object_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	uint8object_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	uint8object_op_print, /* Print. */
	uint8object_op_dump, /* Dump. */
	uint8object_op_neg, /* Negative. */
	NULL, /* Call. */
	uint8object_op_add, /* Addition. */
	uint8object_op_sub, /* Substraction. */
	uint8object_op_mul, /* Multiplication. */
	uint8object_op_div, /* Division. */
	uint8object_op_mod, /* Mod. */
	uint8object_op_not, /* Bitwise not. */
	uint8object_op_and, /* Bitwise and. */
	uint8object_op_or, /* Bitwise or. */
	uint8object_op_xor, /* Bitwise xor. */
	uint8object_op_land, /* Logic and. */
	uint8object_op_lor, /* Logic or. */
	uint8object_op_lshift, /* Left shift. */
	uint8object_op_rshift, /* Right shift. */
	uint8object_op_eq, /* Equality. */
	uint8object_op_cmp, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	uint8object_op_hash, /* Hash. */
	uint8object_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Logic Not. */
static object_t *
uint8object_op_lnot (object_t *obj)
{
	return boolobject_new (!uint8object_get_value (obj), NULL);
}

/* Print. */
static void
uint8object_op_print (object_t *obj)
{
	printf ("%" PRIu8, uint8object_get_value (obj));
}

/* Dump. */
static object_t *
uint8object_op_dump (object_t *obj)
{
	char buf[DUMP_BUF_SIZE];

	snprintf (buf, DUMP_BUF_SIZE, "<uint8 %" PRIu8 ">", uint8object_get_value (obj));

	return strobject_new (buf, strlen (buf), 1, NULL);
}

/* Negative. */
static object_t *
uint8object_op_neg (object_t *obj)
{
	uint8_t val;

	val = uint8object_get_value (obj);

	return uint8object_new (-val, NULL);
}

/* Addition. */
static object_t *
uint8object_op_add (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	uint8_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = uint8object_get_value (obj2);

	return uint8object_new (val1 + val2, NULL);
}

/* Substraction. */
static object_t *
uint8object_op_sub (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	uint8_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = uint8object_get_value (obj2);

	return uint8object_new (val1 - val2, NULL);
}

/* Multiplication. */
static object_t *
uint8object_op_mul (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	uint8_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = uint8object_get_value (obj2);

	return uint8object_new (val1 * val2, NULL);
}


/* Division. */
static object_t *
uint8object_op_div (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	uint8_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = uint8object_get_value (obj2);

	return uint8object_new (val1 / val2, NULL);
}

/* Mod. */
static object_t *
uint8object_op_mod (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	uint8_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = uint8object_get_value (obj2);

	return uint8object_new (val1 % val2, NULL);
}

/* Bitwise not. */
static object_t *
uint8object_op_not (object_t *obj)
{
	uint8_t val;

	val = uint8object_get_value (obj);

	return uint8object_new (~val, NULL);
}

/* Bitwise and. */
static object_t *
uint8object_op_and (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	uint8_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = uint8object_get_value (obj2);

	return uint8object_new (val1 & val2, NULL);
}

/* Bitwise or. */
static object_t *
uint8object_op_or (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	uint8_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = uint8object_get_value (obj2);

	return uint8object_new (val1 | val2, NULL);
}

/* Bitwise xor. */
static object_t *
uint8object_op_xor (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	uint8_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = uint8object_get_value (obj2);

	return uint8object_new (val1 ^ val2, NULL);
}

/* Logic and. */
static object_t *
uint8object_op_land (object_t *obj1, object_t *obj2)
{
	uint8_t val1;

	val1 = uint8object_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
uint8object_op_lor (object_t *obj1, object_t *obj2)
{
	uint8_t val1;

	val1 = uint8object_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Left shift. */
static object_t *
uint8object_op_lshift (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	integer_value_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = object_get_integer (obj2);

	return uint8object_new (val1 << val2, NULL);
}

/* Right shift. */
static object_t *
uint8object_op_rshift (object_t *obj1, object_t *obj2)
{
	uint8_t val1;
	integer_value_t val2;

	val1 = uint8object_get_value (obj1);
	val2 = object_get_integer (obj2);

	return uint8object_new (val1 >> val2, NULL);
}

/* Equality. */
static object_t *
uint8object_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		uint8_t val1;

		val1 = uint8object_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}

	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
uint8object_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
uint8object_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_integer_hash (object_get_integer (obj));
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
uint8object_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((uint8object_t *) obj)->val),
						  sizeof (uint8_t), 1, NULL);
}

object_t *
uint8object_load_binary (FILE *f)
{
	uint8_t val;

	if (fread (&val, sizeof (uint8_t), 1, f) != 1) {
		error ("failed to load int binary.");

		return NULL;
	}

	return uint8object_new (val, NULL);
}

object_t *
uint8object_load_buf (const char **buf, size_t *len)
{
	uint8_t val;

	if (*len < sizeof (uint8_t)) {
		error ("failed to load uint8 buffer.");

		return NULL;
	}

	val = *(uint8_t *) *buf;
	*buf += sizeof (uint8_t);
	*len -= sizeof (uint8_t);

	return uint8object_new (val, NULL);
}

static uint64_t
uint8object_digest_fun (void *obj)
{
	return object_integer_hash (object_get_integer ((object_t *) obj));
}

object_t *
uint8object_new (uint8_t val, void *udata)
{
	uint8object_t *obj;

	obj = (uint8object_t *) pool_alloc (sizeof (uint8object_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_UINT8, udata);
	OBJECT_DIGEST_FUN (obj) = uint8object_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

uint8_t
uint8object_get_value (object_t *obj)
{
	uint8object_t *ob;

	ob = (uint8object_t *) obj;

	return ob->val;
}
