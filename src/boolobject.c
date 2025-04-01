/*
 * boolobject.c
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

#include "boolobject.h"
#include "pool.h"
#include "error.h"
#include "thread.h"
#include "str.h"
#include "intobject.h"
#include "uint64object.h"
#include "strobject.h"

/* Note that the 'true' and 'false' objects are shared everywhere. */
static object_t *g_true_object;
static object_t *g_false_object;

/* Object ops. */
static object_t *boolobject_op_lnot (object_t *obj);
static void boolobject_op_print (object_t *obj);
static object_t *boolobject_op_dump (object_t *obj);
static object_t *boolobject_op_land (object_t *obj1, object_t *obj2);
static object_t *boolobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *boolobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *boolobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *boolobject_op_hash (object_t *obj);
static object_t *boolobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	boolobject_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	boolobject_op_print, /* Print. */
	boolobject_op_dump, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	NULL, /* Addition. */
	NULL, /* Substraction. */
	NULL, /* Multiplication. */
	NULL, /* Division. */
	NULL, /* Mod. */
	NULL, /* Bitwise not. */
	NULL, /* Bitwise and. */
	NULL, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	boolobject_op_land, /* Logic and. */
	boolobject_op_lor, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	boolobject_op_eq, /* Equality. */
	boolobject_op_cmp, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	boolobject_op_hash, /* Hash. */
	boolobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Print. */
static void
boolobject_op_print (object_t *obj)
{
	printf ("%s", boolobject_get_value (obj)? "true": "false");
}

/* Dump. */
static object_t *
boolobject_op_dump (object_t *obj)
{
	if (boolobject_get_value (obj)) {
		return strobject_new ("<bool true>", strlen ("<bool true>"), 1, NULL);
	}

	return strobject_new ("<bool false>", strlen ("<bool true>"), 1, NULL);
}

/* Logic Not. */
static object_t *
boolobject_op_lnot (object_t *obj)
{
	return boolobject_new (!boolobject_get_value (obj), NULL);
}

/* Logic and. */
static object_t *
boolobject_op_land (object_t *obj1, object_t *obj2)
{
	bool val1;

	val1 = boolobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
boolobject_op_lor (object_t *obj1, object_t *obj2)
{
	bool val1;

	val1 = boolobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Equality. */
static object_t *
boolobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		bool val1;

		val1 = boolobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}

	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
boolobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
boolobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_integer_hash (object_get_integer (obj));
	}

	/* The digest is already computed. */
	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
boolobject_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((boolobject_t *) obj)->val),
						  sizeof (bool), 1, NULL);
}

object_t *
boolobject_load_binary (FILE *f)
{
	bool val;

	if (fread (&val, sizeof (bool), 1, f) != 1) {
		error ("failed to load bool binary.");

		return NULL;
	}

	return boolobject_new (val, NULL);
}

object_t *
boolobject_load_buf (const char **buf, size_t *len)
{
	bool val;

	if (*len < sizeof (bool)) {
		error ("failed to load bool buffer.");

		return NULL;
	}

	val = *(bool *) *buf;
	*buf += sizeof (bool);
	*len -= sizeof (bool);

	return boolobject_new (val, NULL);
}

static uint64_t
boolobject_digest_fun (void *obj)
{
	return object_integer_hash (object_get_integer ((object_t *) obj));
}

object_t *
boolobject_new (bool val, void *udata)
{
	boolobject_t *obj;

	if (val && g_true_object != NULL) {
		return g_true_object;
	}
	else if (!val && g_false_object != NULL) {
		return g_false_object;
	}

	obj = (boolobject_t *) pool_alloc (sizeof (boolobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_BOOL, udata);
	OBJECT_DIGEST_FUN (obj) = boolobject_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

bool
boolobject_get_value (object_t *obj)
{
	boolobject_t *ob;

	ob = (boolobject_t *) obj;

	return ob->val;
}

void
boolobject_init ()
{
	if (!thread_is_main_thread ()) {
		return;
	}

	/* This two objects should never be freed. */
	g_true_object = boolobject_new (true, NULL);
	if (g_true_object == NULL) {
		fatal_error ("failed to init object system.");

		return;
	}

	object_set_const (g_true_object);

	g_false_object = boolobject_new (false, NULL);
	if (g_false_object == NULL) {
		fatal_error ("failed to init object system.");

		return;
	}

	object_set_const (g_false_object);
}
