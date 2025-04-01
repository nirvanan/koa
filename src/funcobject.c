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
#include "uint64object.h"
#include "strobject.h"

#define DUMP_BUF_EXTRA 9

/* Object ops. */
static void funcobject_op_free (object_t *obj);
static void funcobject_op_print (object_t *obj);
static object_t *funcobject_op_dump (object_t *obj);
static object_t *funcobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *funcobject_op_hash (object_t *obj);
static object_t *funcobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	funcobject_op_free, /* Free. */
	funcobject_op_print, /* Print. */
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
	funcobject_op_eq, /* Equality. */
	NULL, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	funcobject_op_hash, /* Hash. */
	funcobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Free. */
void
funcobject_op_free (object_t *obj)
{
	/* Do nothing. */
}

/* Print. */
static void
funcobject_op_print (object_t *obj)
{
	funcobject_t *func;
	const char *filename;
	const char *name;

	filename = "unknown";
	name = "unknown";
	func = (funcobject_t *) obj;
	if (func->is_builtin) {
		filename = "builtin";
	}
	else if (func->val != NULL) {
		filename = code_get_filename (func->val);
	}
	if (func->is_builtin) {
		name = builtin_get_name (func->builtin);
	}
	else if (func->val != NULL) {
		name = code_get_name (func->val);
	}
	printf ("(%s:%s)", filename, name);
}

/* Dump. */
static object_t *
funcobject_op_dump (object_t *obj)
{
	funcobject_t *func;
	const char *filename;
	const char *name;
	char *buf;
	size_t size;
	object_t *res;

	filename = "unknown";
	name = "unknown";
	func = (funcobject_t *) obj;
	if (func->is_builtin) {
		filename = "builtin";
	}
	else if (func->val != NULL) {
		filename = code_get_filename (func->val);
	}
	if (func->is_builtin) {
		name = builtin_get_name (func->builtin);
	}
	else if (func->val != NULL) {
		name = code_get_name (func->val);
	}
	size = strlen (filename) + strlen (name) + DUMP_BUF_EXTRA;
	buf = (char *) pool_calloc (size, sizeof (char));
	if (buf == NULL) {
		fatal_error ("out of memory.");
	}

	snprintf (buf, size, "<func %s:%s>", filename, name);
	res = strobject_new (buf, strlen (buf), 1, NULL);
	pool_free ((void *) buf);

	return res;
}

/* Equality. */
static object_t *
funcobject_op_eq (object_t *obj1, object_t *obj2)
{
	return boolobject_new (obj1 == obj2, NULL);
}

/* Hash. */
static object_t *
funcobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_address_hash ((void *) obj);
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
funcobject_op_binary (object_t *obj)
{
	funcobject_t *func;
	object_t *type;
	object_t *bin;
	object_t *ret;

	func = (funcobject_t *) obj;
	type = strobject_new (BINARY (func->is_builtin), sizeof (int), 1, NULL);
	if (type == NULL) {
		return NULL;
	}
	if (func->is_builtin) {
		bin = builtin_binary (func->builtin);
	}
	else {
		bin = code_binary (funcobject_get_value (obj));
	}
	if (bin == NULL) {
		return NULL;
	}

	ret = object_add (type, bin);
	object_free (type);
	object_free (bin);

	return ret;
}

object_t *
funcobject_load_binary (FILE *f)
{
	code_t *code;
	int is_builtin;

	if (fread (&is_builtin, sizeof (int), 1, f) != 1) {
		error ("failed to load func binary.");

		return NULL;
	}

	if (is_builtin) {
		builtin_t *builtin;

		builtin = builtin_load_binary (f);
		if (builtin == NULL) {
			return NULL;
		}

		return funcobject_builtin_new (builtin, NULL);
	}

	code = code_load_binary (NULL, f);
	if (code == NULL) {
		return NULL;
	}

	return funcobject_code_new (code, NULL);
}

object_t *
funcobject_load_buf (const char **buf, size_t *len)
{
	code_t *code;
	int is_builtin;

	if (*len < sizeof (int)) {
		error ("failed to load func buf.");

		return NULL;
	}

	is_builtin = *(int *) *buf;
	*buf += sizeof (int);
	*len -= sizeof (int);

	if (is_builtin) {
		builtin_t *builtin;

		builtin = builtin_load_buf (buf, len);
		if (builtin == NULL) {
			return NULL;
		}

		return funcobject_builtin_new (builtin, NULL);
	}

	code = code_load_buf (buf, len);
	if (code == NULL) {
		return NULL;
	}

	return funcobject_code_new (code, NULL);
}

static uint64_t
funcobject_digest_fun (void *obj)
{
	return object_address_hash (obj);
}

/* This is a null func object, which means it is not callable. */
object_t *
funcobject_new (void *udata)
{
	funcobject_t *obj;

	obj = (funcobject_t *) pool_alloc (sizeof (funcobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_FUNC, udata);
	OBJECT_DIGEST_FUN (obj) = funcobject_digest_fun;

	obj->is_builtin = 0;
	obj->builtin = NULL;
	obj->val = NULL;

	return (object_t *) obj;
}

object_t *
funcobject_code_new (code_t *val, void *udata)
{
	funcobject_t *obj;

	obj = (funcobject_t *) pool_alloc (sizeof (funcobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_FUNC, udata);
	OBJECT_DIGEST_FUN (obj) = funcobject_digest_fun;

	obj->is_builtin = 0;
	obj->builtin = NULL;
	obj->val = val;

	return (object_t *) obj;
}

object_t *
funcobject_builtin_new (builtin_t *builtin, void *udata)
{
	funcobject_t *obj;

	obj = (funcobject_t *) pool_alloc (sizeof (funcobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_FUNC, udata);
	OBJECT_DIGEST_FUN (obj) = funcobject_digest_fun;

	obj->is_builtin = 1;
	obj->builtin = builtin;
	obj->val = NULL;

	return (object_t *) obj;
}

code_t *
funcobject_get_value (object_t *obj)
{
	funcobject_t *ob;

	ob = (funcobject_t *) obj;

	return ob->val;
}

builtin_t *
funcobject_get_builtin (object_t *obj)
{
	funcobject_t *ob;

	ob = (funcobject_t *) obj;

	return ob->builtin;
}

int
funcobject_is_builtin (object_t *obj)
{
	funcobject_t *ob;

	ob = (funcobject_t *) obj;

	return ob->is_builtin;
}

object_t *
funcobject_copy (object_t *obj)
{
	/* Just memcpy it. */
	funcobject_t *new_obj;

	new_obj = (funcobject_t *) pool_alloc (sizeof (funcobject_t));
	if (new_obj == NULL) {
		fatal_error ("out of memory.");
	}

	memcpy (new_obj, obj, sizeof (funcobject_t));

	return (object_t *) new_obj;
}