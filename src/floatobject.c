/*
 * floatobject.c
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

#include "floatobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "intobject.h"
#include "uint64object.h"
#include "strobject.h"

#define DUMP_BUF_SIZE 1400

/* Object ops. */
static object_t *floatobject_op_lnot (object_t *obj);
static void floatobject_op_print (object_t *obj);
static object_t *floatobject_op_dump (object_t *obj);
static object_t *floatobject_op_neg (object_t *obj);
static object_t *floatobject_op_add (object_t *obj1, object_t *obj2);
static object_t *floatobject_op_sub (object_t *obj1, object_t *obj2);
static object_t *floatobject_op_mul (object_t *obj1, object_t *obj2);
static object_t *floatobject_op_div (object_t *obj1, object_t *obj2);
static object_t *floatobject_op_land (object_t *obj1, object_t *obj2);
static object_t *floatobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *floatobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *floatobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *floatobject_op_hash (object_t *obj);
static object_t *floatobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	floatobject_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	floatobject_op_print, /* Print. */
	floatobject_op_dump, /* Dump. */
	floatobject_op_neg, /* Negative. */
	NULL, /* Call. */
	floatobject_op_add, /* Addition. */
	floatobject_op_sub, /* Substraction. */
	floatobject_op_mul, /* Multiplication. */
	floatobject_op_div, /* Division. */
	NULL, /* Mod. */
	NULL, /* Bitwise not. */
	NULL, /* Bitwise and. */
	NULL, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	floatobject_op_land, /* Logic and. */
	floatobject_op_lor, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	floatobject_op_eq, /* Equality. */
	floatobject_op_cmp, /* Comparation. */
	NULL,  /* Index. */
	NULL,  /* Inplace index. */
	floatobject_op_hash, /* Hash. */
	floatobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Logic Not. */
static object_t *
floatobject_op_lnot (object_t *obj)
{
	return boolobject_new (!floatobject_get_value (obj), NULL);
}

/* Print. */
static void
floatobject_op_print (object_t *obj)
{
	printf ("%f", floatobject_get_value (obj));
}

/* Dump. */
static object_t *
floatobject_op_dump (object_t *obj)
{
	char buf[DUMP_BUF_SIZE];

	snprintf (buf, DUMP_BUF_SIZE, "<float %f>", floatobject_get_value (obj));

	return strobject_new (buf, strlen (buf), 1, NULL);
}

/* Negative. */
static object_t *
floatobject_op_neg (object_t *obj)
{
	float val;

	val = floatobject_get_value (obj);

	return floatobject_new (-val, NULL);
}

/* Addition. */
static object_t *
floatobject_op_add (object_t *obj1, object_t *obj2)
{
	float val1;
	float val2;

	val1 = floatobject_get_value (obj1);
	val2 = floatobject_get_value (obj2);

	return floatobject_new (val1 + val2, NULL);
}

/* Substraction. */
static object_t *
floatobject_op_sub (object_t *obj1, object_t *obj2)
{
	float val1;
	float val2;

	val1 = floatobject_get_value (obj1);
	val2 = floatobject_get_value (obj2);

	return floatobject_new (val1 - val2, NULL);
}

/* Multiplication. */
static object_t *
floatobject_op_mul (object_t *obj1, object_t *obj2)
{
	float val1;
	float val2;

	val1 = floatobject_get_value (obj1);
	val2 = floatobject_get_value (obj2);

	return floatobject_new (val1 * val2, NULL);
}


/* Division. */
static object_t *
floatobject_op_div (object_t *obj1, object_t *obj2)
{
	float val1;
	float val2;

	val1 = floatobject_get_value (obj1);
	val2 = floatobject_get_value (obj2);

	return floatobject_new (val1 / val2, NULL);
}

/* Logic and. */
static object_t *
floatobject_op_land (object_t *obj1, object_t *obj2)
{
	float val1;

	val1 = floatobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
floatobject_op_lor (object_t *obj1, object_t *obj2)
{
	float val1;

	val1 = floatobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Equality. */
static object_t *
floatobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		float val1;

		val1 = floatobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}

	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
floatobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
floatobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_floating_hash (object_get_floating (obj));
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
floatobject_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((floatobject_t *) obj)->val),
						  sizeof (float), 1, NULL);
}

object_t *
floatobject_load_binary (FILE *f)
{
	float val;

	if (fread (&val, sizeof (float), 1, f) != 1) {
		error ("failed to load float binary.");

		return NULL;
	}

	return floatobject_new (val, NULL);
}

object_t *
floatobject_load_buf (const char **buf, size_t *len)
{
	float val;

	if (*len < sizeof (float)) {
		error ("failed to load float buffer.");

		return NULL;
	}

	val = *(float *) *buf;
	*buf += sizeof (float);
	*len -= sizeof (float);

	return floatobject_new (val, NULL);
}

static uint64_t
floatobject_digest_fun (void *obj)
{
	return object_floating_hash (object_get_floating ((object_t *) obj));
}

object_t *
floatobject_new (float val, void *udata)
{
	floatobject_t *obj;

	obj = (floatobject_t *) pool_alloc (sizeof (floatobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_FLOAT, udata);
	OBJECT_DIGEST_FUN (obj) = floatobject_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

float
floatobject_get_value (object_t *obj)
{
	floatobject_t *ob;

	ob = (floatobject_t *) obj;

	return ob->val;
}

