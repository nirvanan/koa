/*
 * doubleobject.c
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

#include "doubleobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "intobject.h"
#include "uint64object.h"
#include "strobject.h"

#define DUMP_BUF_SIZE 1400

/* Object ops. */
static object_t *doubleobject_op_lnot (object_t *obj);
static void doubleobject_op_print (object_t *obj);
static object_t *doubleobject_op_dump (object_t *obj);
static object_t *doubleobject_op_neg (object_t *obj);
static object_t *doubleobject_op_add (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_sub (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_mul (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_div (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_land (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_hash (object_t *obj);
static object_t *doubleobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	doubleobject_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	doubleobject_op_print, /* Print. */
	doubleobject_op_dump, /* Dump. */
	doubleobject_op_neg, /* Negative. */
	NULL, /* Call. */
	doubleobject_op_add, /* Addition. */
	doubleobject_op_sub, /* Substraction. */
	doubleobject_op_mul, /* Multiplication. */
	doubleobject_op_div, /* Division. */
	NULL, /* Mod. */
	NULL, /* Bitwise not. */
	NULL, /* Bitwise and. */
	NULL, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	doubleobject_op_land, /* Logic and. */
	doubleobject_op_lor, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	doubleobject_op_eq, /* Equality. */
	doubleobject_op_cmp, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	doubleobject_op_hash, /* Hash. */
	doubleobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Logic Not. */
static object_t *
doubleobject_op_lnot (object_t *obj)
{
	return boolobject_new (!doubleobject_get_value (obj), NULL);
}

/* Print. */
static void
doubleobject_op_print (object_t *obj)
{
	printf ("%f", doubleobject_get_value (obj));
}

/* Dump. */
static object_t *
doubleobject_op_dump (object_t *obj)
{
	char buf[DUMP_BUF_SIZE];

	snprintf (buf, DUMP_BUF_SIZE, "<double %lf>", doubleobject_get_value (obj));

	return strobject_new (buf, strlen (buf), 1, NULL);
}

/* Negative. */
static object_t *
doubleobject_op_neg (object_t *obj)
{
	double val;

	val = doubleobject_get_value (obj);

	return doubleobject_new (-val, NULL);
}

/* Addition. */
static object_t *
doubleobject_op_add (object_t *obj1, object_t *obj2)
{
	double val1;
	double val2;

	val1 = doubleobject_get_value (obj1);
	val2 = doubleobject_get_value (obj2);

	return doubleobject_new (val1 + val2, NULL);
}

/* Substraction. */
static object_t *
doubleobject_op_sub (object_t *obj1, object_t *obj2)
{
	double val1;
	double val2;

	val1 = doubleobject_get_value (obj1);
	val2 = doubleobject_get_value (obj2);

	return doubleobject_new (val1 - val2, NULL);
}

/* Multiplication. */
static object_t *
doubleobject_op_mul (object_t *obj1, object_t *obj2)
{
	double val1;
	double val2;

	val1 = doubleobject_get_value (obj1);
	val2 = doubleobject_get_value (obj2);

	return doubleobject_new (val1 * val2, NULL);
}


/* Division. */
static object_t *
doubleobject_op_div (object_t *obj1, object_t *obj2)
{
	double val1;
	double val2;

	val1 = doubleobject_get_value (obj1);
	val2 = doubleobject_get_value (obj2);

	return doubleobject_new (val1 / val2, NULL);
}

/* Logic and. */
static object_t *
doubleobject_op_land (object_t *obj1, object_t *obj2)
{
	double val1;

	val1 = doubleobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
doubleobject_op_lor (object_t *obj1, object_t *obj2)
{
	double val1;

	val1 = doubleobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Equality. */
static object_t *
doubleobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		double val1;

		val1 = doubleobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}

	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
doubleobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
doubleobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_floating_hash (object_get_floating (obj));
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
doubleobject_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((doubleobject_t *) obj)->val),
						  sizeof (double), 1, NULL);
}

object_t *
doubleobject_load_binary (FILE *f)
{
	double val;

	if (fread (&val, sizeof (double), 1, f) != 1) {
		error ("failed to load double binary.");

		return NULL;
	}

	return doubleobject_new (val, NULL);
}

object_t *
doubleobject_load_buf (const char **buf, size_t *len)
{
	double val;

	if (*len < sizeof (double)) {
		error ("failed to load double buffer.");

		return NULL;
	}

	val = *(double *) *buf;
	*buf += sizeof (double);
	*len -= sizeof (double);

	return doubleobject_new (val, NULL);
}

static uint64_t
doubleobject_digest_fun (void *obj)
{
	return object_floating_hash (object_get_floating ((object_t *) obj));
}

object_t *
doubleobject_new (double val, void *udata)
{
	doubleobject_t *obj;

	obj = (doubleobject_t *) pool_alloc (sizeof (doubleobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_DOUBLE, udata);
	OBJECT_DIGEST_FUN (obj) = doubleobject_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

double
doubleobject_get_value (object_t *obj)
{
	doubleobject_t *ob;

	ob = (doubleobject_t *) obj;

	return ob->val;
}

