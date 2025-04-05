/*
 * ushortobject.c
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

#include "ushortobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "intobject.h"
#include "uint64object.h"
#include "strobject.h"

#define DUMP_BUF_SIZE 18

/* Object ops. */
static object_t *ushortobject_op_lnot (object_t *obj);
static void ushortobject_op_print (object_t *obj);
static object_t *ushortobject_op_dump (object_t *obj);
static object_t *ushortobject_op_neg (object_t *obj);
static object_t *ushortobject_op_add (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_sub (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_mul (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_mod (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_div (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_not (object_t *obj);
static object_t *ushortobject_op_and (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_or (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_xor (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_land (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_lshift (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_rshift (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *ushortobject_op_hash (object_t *obj);
static object_t *ushortobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	ushortobject_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	ushortobject_op_print, /* Print. */
	ushortobject_op_dump, /* Dump. */
	ushortobject_op_neg, /* Negative. */
	NULL, /* Call. */
	ushortobject_op_add, /* Addition. */
	ushortobject_op_sub, /* Substraction. */
	ushortobject_op_mul, /* Multiplication. */
	ushortobject_op_div, /* Division. */
	ushortobject_op_mod, /* Mod. */
	ushortobject_op_not, /* Bitwise not. */
	ushortobject_op_and, /* Bitwise and. */
	ushortobject_op_or, /* Bitwise or. */
	ushortobject_op_xor, /* Bitwise xor. */
	ushortobject_op_land, /* Logic and. */
	ushortobject_op_lor, /* Logic or. */
	ushortobject_op_lshift, /* Left shift. */
	ushortobject_op_rshift, /* Right shift. */
	ushortobject_op_eq, /* Equality. */
	ushortobject_op_cmp, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	ushortobject_op_hash, /* Hash. */
	ushortobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Logic Not. */
static object_t *
ushortobject_op_lnot (object_t *obj)
{
	return boolobject_new (!ushortobject_get_value (obj), NULL);
}

/* Print. */
static void
ushortobject_op_print (object_t *obj)
{
	printf ("%hu", ushortobject_get_value (obj));
}

/* Dump. */
static object_t *
ushortobject_op_dump (object_t *obj)
{
	char buf[DUMP_BUF_SIZE];

	snprintf (buf, DUMP_BUF_SIZE, "<ushort %hu>", ushortobject_get_value (obj));

	return strobject_new (buf, strlen (buf), 1, NULL);
}

/* Negative. */
static object_t *
ushortobject_op_neg (object_t *obj)
{
	unsigned short int val;

	val = ushortobject_get_value (obj);

	return ushortobject_new (-val, NULL);
}

/* Addition. */
static object_t *
ushortobject_op_add (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	unsigned short int val2;

	val1 = ushortobject_get_value (obj1);
	val2 = ushortobject_get_value (obj2);

	return ushortobject_new (val1 + val2, NULL);
}

/* Substraction. */
static object_t *
ushortobject_op_sub (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	unsigned short int val2;

	val1 = ushortobject_get_value (obj1);
	val2 = ushortobject_get_value (obj2);

	return ushortobject_new (val1 - val2, NULL);
}

/* Multiplication. */
static object_t *
ushortobject_op_mul (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	unsigned short int val2;

	val1 = ushortobject_get_value (obj1);
	val2 = ushortobject_get_value (obj2);

	return ushortobject_new (val1 * val2, NULL);
}


/* Division. */
static object_t *
ushortobject_op_div (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	unsigned short int val2;

	val1 = ushortobject_get_value (obj1);
	val2 = ushortobject_get_value (obj2);

	return ushortobject_new (val1 / val2, NULL);
}

/* Mod. */
static object_t *
ushortobject_op_mod (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	unsigned short int val2;

	val1 = ushortobject_get_value (obj1);
	val2 = ushortobject_get_value (obj2);

	return ushortobject_new (val1 % val2, NULL);
}

/* Bitwise not. */
static object_t *
ushortobject_op_not (object_t *obj)
{
	unsigned short int val;

	val = ushortobject_get_value (obj);

	return ushortobject_new (~val, NULL);
}

/* Bitwise and. */
static object_t *
ushortobject_op_and (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	unsigned short int val2;

	val1 = ushortobject_get_value (obj1);
	val2 = ushortobject_get_value (obj2);

	return ushortobject_new (val1 & val2, NULL);
}

/* Bitwise or. */
static object_t *
ushortobject_op_or (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	unsigned short int val2;

	val1 = ushortobject_get_value (obj1);
	val2 = ushortobject_get_value (obj2);

	return ushortobject_new (val1 | val2, NULL);
}

/* Bitwise xor. */
static object_t *
ushortobject_op_xor (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	unsigned short int val2;

	val1 = ushortobject_get_value (obj1);
	val2 = ushortobject_get_value (obj2);

	return ushortobject_new (val1 ^ val2, NULL);
}

/* Logic and. */
static object_t *
ushortobject_op_land (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;

	val1 = ushortobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
ushortobject_op_lor (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;

	val1 = ushortobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Left shift. */
static object_t *
ushortobject_op_lshift (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	integer_value_t val2;

	val1 = ushortobject_get_value (obj1);
	val2 = object_get_integer (obj2);

	return ushortobject_new (val1 << val2, NULL);
}

/* Right shift. */
static object_t *
ushortobject_op_rshift (object_t *obj1, object_t *obj2)
{
	unsigned short int val1;
	integer_value_t val2;

	val1 = ushortobject_get_value (obj1);
	val2 = object_get_integer (obj2);

	return ushortobject_new (val1 >> val2, NULL);
}

/* Equality. */
static object_t *
ushortobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		unsigned short int val1;

		val1 = ushortobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}

	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
ushortobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
ushortobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_integer_hash (object_get_integer (obj));
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
ushortobject_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((ushortobject_t *) obj)->val),
						  sizeof (unsigned short int), 1, NULL);
}

object_t *
ushortobject_load_binary (FILE *f)
{
	unsigned short int val;

	if (fread (&val, sizeof (unsigned short int), 1, f) != 1) {
		error ("failed to load int binary.");

		return NULL;
	}

	return ushortobject_new (val, NULL);
}

object_t *
ushortobject_load_buf (const char **buf, size_t *len)
{
	unsigned short int val;

	if (*len < sizeof (unsigned short int)) {
		error ("failed to load ushort buffer.");

		return NULL;
	}

	val = *(unsigned short int *) *buf;
	*buf += sizeof (unsigned short int);
	*len -= sizeof (unsigned short int);

	return ushortobject_new (val, NULL);
}

static uint64_t
ushortobject_digest_fun (void *obj)
{
	return object_integer_hash (object_get_integer ((object_t *) obj));
}

object_t *
ushortobject_new (unsigned short int val, void *udata)
{
	ushortobject_t *obj;

	obj = (ushortobject_t *) pool_alloc (sizeof (ushortobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_USHORT, udata);
	OBJECT_DIGEST_FUN (obj) = ushortobject_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

unsigned short int
ushortobject_get_value (object_t *obj)
{
	ushortobject_t *ob;

	ob = (ushortobject_t *) obj;

	return ob->val;
}
