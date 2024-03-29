/*
 * modobject.c
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

#include "modobject.h"
#include "pool.h"
#include "error.h"
#include "nullobject.h"
#include "boolobject.h"
#include "uint64object.h"
#include "strobject.h"

#define DUMP_BUF_EXTRA 8

/* Object ops. */
static void modobject_op_free (object_t *obj);
static void modobject_op_print (object_t *obj);
static object_t *modobject_op_dump (object_t *obj);
static object_t *modobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *modobject_op_hash (object_t *obj);
static object_t *modobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	modobject_op_free, /* Free. */
	modobject_op_print, /* Print. */
	modobject_op_dump, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	NULL, /* Addition. */
	NULL, /* Submodaction. */
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
	modobject_op_eq, /* Equality. */
	NULL, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	modobject_op_hash, /* Hash. */
	modobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Free. */
void
modobject_op_free (object_t *obj)
{
	code_free (modobject_get_value (obj));
}

/* Print. */
static void
modobject_op_print (object_t *obj)
{
	const char *filename;

	filename = code_get_filename (modobject_get_value (obj));
	printf ("<%s>", filename);
}

/* Dump. */
static object_t *
modobject_op_dump (object_t *obj)
{
	const char *filename;
	char *buf;
	size_t size;
	object_t *res;

	filename = code_get_filename (modobject_get_value (obj));
	size = strlen (filename) + DUMP_BUF_EXTRA;
	buf = (char *) pool_calloc (size, sizeof (char));
	if (buf == NULL) {
		fatal_error ("out of memory.");
	}

	snprintf (buf, size, "<mod %s>", filename);
	res = strobject_new (buf, strlen (buf), 1, NULL);
	pool_free ((void *) buf);

	return res;
}

/* Equality. */
static object_t *
modobject_op_eq (object_t *obj1, object_t *obj2)
{
	return boolobject_new (obj1 == obj2, NULL);
}

/* Hash. */
static object_t *
modobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_address_hash ((void *) obj);
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
modobject_op_binary (object_t *obj)
{
	return code_binary (modobject_get_value (obj));
}

object_t *
modobject_load_binary (FILE *f)
{
	code_t *code;

	code = code_load_binary (NULL, f);
	if (code == NULL) {
		return NULL;
	}

	return modobject_code_new (code, NULL);
}

object_t *
modobject_load_buf (const char **buf, size_t *len)
{
	code_t *code;

	code = code_load_buf (buf, len);
	if (code == NULL) {
		return NULL;
	}

	return modobject_code_new (code, NULL);
}

static uint64_t
modobject_digest_fun (void *obj)
{
	return object_address_hash (obj);
}

/* This is a null mod object, which means it has nothing. */
object_t *
modobject_new (void *udata)
{
	modobject_t *obj;

	obj = (modobject_t *) pool_alloc (sizeof (modobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_MOD, udata);
	OBJECT_DIGEST_FUN (obj) = modobject_digest_fun;

	obj->val = NULL;

	return (object_t *) obj;
}

object_t *
modobject_code_new (code_t *val, void *udata)
{
	modobject_t *obj;

	obj = (modobject_t *) pool_alloc (sizeof (modobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_MOD, udata);
	OBJECT_DIGEST_FUN (obj) = modobject_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

code_t *
modobject_get_value (object_t *obj)
{
	modobject_t *ob;

	ob = (modobject_t *) obj;

	return ob->val;
}

