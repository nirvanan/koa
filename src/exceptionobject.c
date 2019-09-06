/*
 * exceptionobject.c
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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "exceptionobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "charobject.h"
#include "intobject.h"
#include "uint64object.h"

#define DUMP_HEAD_LENGTH 12
#define DUMP_TAIL_LENGTH 2

static str_t *g_dump_head;
static str_t *g_dump_tail;

/* Object ops. */
static void exceptionobject_op_free (object_t *obj);
static void exceptionobject_op_print (object_t *obj);
static object_t *exceptionobject_op_dump (object_t *obj);
static object_t *exceptionobject_op_hash (object_t *obj);
static object_t *exceptionobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	exceptionobject_op_free, /* Free. */
	exceptionobject_op_print, /* Print. */
	exceptionobject_op_dump, /* Dump. */
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
	NULL, /* Logic and. */
	NULL, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	NULL, /* Equality. */
	NULL, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */ /* Note that str objects are read-only! */
	exceptionobject_op_hash, /* Hash. */
	exceptionobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Free. */
void
exceptionobject_op_free (object_t *obj)
{
	str_free (exceptionobject_get_value (obj));
}

/* Print. */
static void
exceptionobject_op_print (object_t *obj)
{
	str_t *str;
	size_t len;

	str = exceptionobject_get_value (obj);
	len = str_len (str);
	for (size_t i = 0; i < len; i++) {
		printf ("%c", str_pos (str, (integer_value_t) i));
	}
}

/* Dump. */
static object_t *
exceptionobject_op_dump (object_t *obj)
{
	str_t *temp;
	str_t *res;

	temp = str_concat (g_dump_head, exceptionobject_get_value (obj));
	if (temp == NULL) {
		return NULL;
	}
	res = str_concat (temp, g_dump_tail);
	if (res == NULL) {
		str_free (temp);

		return NULL;
	}

	str_free (temp);

	return exceptionobject_str_new (res, NULL);
}

/* Hash. */
static object_t *
exceptionobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_address_hash ((void *) obj);
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
exceptionobject_op_binary (object_t *obj)
{
	str_t *str;
	size_t len;
	object_t *len_obj;
	object_t *res;

	str = exceptionobject_get_value (obj);
	len = str_len (str);

	len_obj = exceptionobject_new (BINARY (len), sizeof (size_t), NULL);
	if (len_obj == NULL) {
		return NULL;
	}
	
	res = object_add (len_obj, obj);
	object_free (len_obj);

	return res;
}

object_t *
exceptionobject_load_binary (FILE *f)
{
	size_t len;
	char *data;
	object_t *obj;

	if (fread (&len, sizeof (size_t), 1, f) != 1) {
		error ("failed to load size while loading str.");

		return NULL;
	}

	data = pool_calloc (len + 1, sizeof (char));
	if (data == NULL) {
		fatal_error ("out of memory.");
	}

	if (fread (data, sizeof (char), len, f) != len) {
		pool_free (data);
		error ("failed to load str.");

		return NULL;
	}

	obj = exceptionobject_new ((const char *) data, len, NULL);
	pool_free (data);

	return obj;
}

static uint64_t
exceptionobject_digest_fun (void *obj)
{
	return object_address_hash (obj);
}

object_t *
exceptionobject_new (const char *val, size_t len, void *udata)
{
	exceptionobject_t *obj;

	if (val == NULL) {
		val = "";
	}

	obj = (exceptionobject_t *) pool_alloc (sizeof (exceptionobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_EXCEPTION);
	OBJECT_DIGEST_FUN (obj) = exceptionobject_digest_fun;

	obj->val = str_new (val, len);
	if (obj->val == NULL) {
		pool_free ((void *) obj);

		return NULL;
	}

	return (object_t *) obj;
}

object_t *
exceptionobject_str_new (str_t *val, void *udata)
{
	exceptionobject_t *obj;

	obj = (exceptionobject_t *) pool_alloc (sizeof (exceptionobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_EXCEPTION);

	obj->val = val;

	return (object_t *) obj;
}

str_t *
exceptionobject_get_value (object_t *obj)
{
	exceptionobject_t *ob;

	ob = (exceptionobject_t *) obj;

	return ob->val;
}

const char *
exceptionobject_c_str (object_t *obj)
{
	return str_c_str (exceptionobject_get_value (obj));
}

void
exceptionobject_init ()
{
	/* Make dump head and tail. */
	g_dump_head = str_new ("<exception \"", DUMP_HEAD_LENGTH);
	if (g_dump_head == NULL) {
		fatal_error ("failed to init exception dump head.");
	}
	g_dump_tail = str_new ("\">", DUMP_TAIL_LENGTH);
	if (g_dump_tail == NULL) {
		fatal_error ("failed to init exception dump tail.");
	}
}
