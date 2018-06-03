/*
 * funcobject.c
 * This file is part of koa
 *
 * Copyright (C) 2018 - Gordon Li
 *
 * This program is free software: you can redifuncibute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is difuncibuted in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "funcobject.h"
#include "pool.h"
#include "error.h"
#include "nullobject.h"
#include "boolobject.h"
#include "longobject.h"
#include "strobject.h"

#define DUMP_BUF_EXTRA 8

/* Object ops. */
static void funcobject_op_free (object_t *obj);
static object_t *funcobject_op_dump (object_t *obj);
static object_t *funcobject_op_hash (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	funcobject_op_free, /* Free. */
	funcobject_op_dump, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	NULL, /* Addition. */
	NULL, /* Subfuncaction. */
	NULL, /* Multiplication. */
	NULL, /* Division. */
	NULL, /* Mod. */
	NULL, /* Bitwise not. */
	NULL, /* Bitwise and. */
	NULL, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	NULL, /* Logic and. */
	NULL, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	NULL, /* Equality. */
	NULL, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	funcobject_op_hash /* Hash. */
};

/* Free. */
void
funcobject_op_free (object_t *obj)
{
	code_free (funcobject_get_value (obj));
}

/* Dump. */
static object_t *
funcobject_op_dump (object_t *obj)
{
	const char *name;
	char *buf;
	size_t size;
	object_t *res;

	name = code_get_name (funcobject_get_value (obj));
	size = strlen (name) + DUMP_BUF_EXTRA;
	buf = (char *) pool_calloc (size, sizeof (char));
	if (buf == NULL) {
		error ("out of memory.");

		return NULL;
	}

	snprintf (buf, size, "<func %s>", name);
	res = strobject_new (buf, NULL);
	pool_free ((void *) buf);

	return res;
}

/* Hash. */
static object_t *
funcobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_address_hash ((void *) obj);
	}

	return longobject_new ((long) OBJECT_DIGEST (obj), NULL);
}

/* This is a null func object, which means it is not callable. */
object_t *
funcobject_new (void *udata)
{
	funcobject_t *obj;

	obj = (funcobject_t *) pool_alloc (sizeof (funcobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_FUNC);

	obj->val = NULL;

	return (object_t *) obj;
}

object_t *
funcobject_code_new (code_t *val, void *udata)
{
	funcobject_t *obj;

	obj = (funcobject_t *) pool_alloc (sizeof (funcobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_VEC);

	obj->val = val;

	return (object_t *) obj;
}

code_t *
funcobject_get_value (object_t *obj)
{
	funcobject_t *ob;

	ob = (funcobject_t *) obj;

	return ob->val;
}

