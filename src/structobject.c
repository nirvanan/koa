/*
 * structobject.c
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

#include "structobject.h"
#include "pool.h"
#include "gc.h"
#include "error.h"
#include "struct.h"
#include "boolobject.h"
#include "uint64object.h"
#include "strobject.h"

static object_t *g_dump_head;
static object_t *g_dump_tail;
static object_t *g_dump_sep;

/* Object ops. */
static void structobject_op_free (object_t *obj);
static void structobject_op_print (object_t *obj);
static object_t *structobject_op_dump (object_t *obj);
static object_t *structobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *structobject_op_hash (object_t *obj);
static object_t *structobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	structobject_op_free, /* Free. */
	structobject_op_print, /* Print. */
	structobject_op_dump, /* Dump. */
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
	structobject_op_eq, /* Equality. */
	NULL, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	structobject_op_hash, /* Hash. */
	structobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Free. */
void
structobject_op_free (object_t *obj)
{
	vec_t *members;
	size_t size;

	members = ((structobject_t *) obj)->members;
	size = vec_size (members);
	for (size_t i = 0; i < size; i++) {
		object_unref ((object_t *) vec_pos (members, (integer_value_t) i));
	}

	vec_free (members);

	gc_untrack ((void *) obj);
}

/* Print. */
static void
structobject_op_print (object_t *obj)
{
	vec_t *members;
	size_t size;

	members = ((structobject_t *) obj)->members;
	size = vec_size (members);
	printf ("{");
	for (size_t i = 0; i < size; i++) {
		object_print ((object_t *) vec_pos (members, (integer_value_t) i));
		if (i < size - 1) {
			printf (",");
		}
	}
	printf ("}");	
}

static object_t *
structobject_dump_concat (object_t *obj1, object_t *obj2, int free)
{
	object_t *res;

	res = object_add (obj1, obj2);
	object_free (obj1);
	if (free) {
		object_free (obj2);
	}

	return res;
}

/* Dump. */
static object_t *
structobject_op_dump (object_t *obj)
{
	object_t *res;
	object_t *element;
	vec_t *members;
	size_t size;

	members = ((structobject_t *) obj)->members;
	size = vec_size (members);
	if (!size) {
		return object_add (g_dump_head, g_dump_tail);
	}

	res = object_add (g_dump_head, (object_t *) vec_pos (members, 0));
	if (res == NULL) {
		return NULL;
	}

	for (integer_value_t i = 1; i < (integer_value_t) size; i++) {
		object_t *dump;

		element = (object_t *) vec_pos (members, i);
		dump = object_dump (element);
		if (dump == NULL) {
			object_free (res);

			return NULL;
		}

		if ((res = structobject_dump_concat (res, g_dump_sep, 0)) == NULL ||
			(res = structobject_dump_concat (res, dump, 1)) == NULL) {
			return NULL;
		}
	}

	return structobject_dump_concat (res, g_dump_tail, 0);
}

/* Equality. */
static object_t *
structobject_op_eq (object_t *obj1, object_t *obj2)
{
	return boolobject_new (obj1 == obj2, NULL);
}

/* Hash. */
static object_t *
structobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_address_hash ((void *) obj);
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
structobject_op_binary (object_t *obj)
{
	vec_t *vec;
	size_t size;
	object_t *temp;

	vec = ((structobject_t *) obj)->members;
	size = vec_size (vec);

	temp = strobject_new (BINARY (size), sizeof (size_t), 1, NULL);
	if (temp == NULL) {
		return NULL;
	}

	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *obj;
		object_t *res;

		obj = (object_t *) vec_pos (vec, i);
		obj = object_binary (obj);
		if (obj == NULL) {
			object_free (temp);

			return NULL;
		}

		res = object_add (temp, obj);
		object_free (temp);
		object_free (obj);
		if (res == NULL) {
			return NULL;
		}
		
		temp = res;
	}
	
	return temp;
}

static uint64_t
structobject_digest_fun (void *obj)
{
	return object_address_hash (obj);
}

object_t *
structobject_load_binary (object_type_t type, FILE *f)
{
	size_t size;
	vec_t *members;
	structobject_t *obj;

	if (fread (&size, sizeof (size_t), 1, f) != 1) {
		error ("failed to load size while load vec.");

		return NULL;
	}

	members = vec_new (size);
	if (members == NULL) {
		return NULL;
	}

	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *obj;

		obj = object_load_binary (f);
		if (obj == NULL) {
			for (integer_value_t j = 0; j < (integer_value_t) i; j++) {
				object_free ((object_t *) vec_pos (members, j));
			}
			vec_free (members);

			return NULL;
		}

		UNUSED (vec_set (members, i, (void *) obj));
		object_ref (obj);
	}

	obj = (structobject_t *) pool_alloc (sizeof (structobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, type, NULL);
	OBJECT_DIGEST_FUN (obj) = structobject_digest_fun;

	obj->members = members;

	gc_track ((void *) obj);

	return (object_t *) obj;
}

object_t *
structobject_new (code_t *code, object_type_t type, void *udata)
{
	structobject_t *obj;
	struct_t *meta;
	vec_t *members;
	size_t size;

	obj = (structobject_t *) pool_alloc (sizeof (structobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, type, udata);
	OBJECT_DIGEST_FUN (obj) = structobject_digest_fun;

	meta = code_get_struct (code, type);
	if (meta == NULL) {
		pool_free ((void *) obj);
		error ("struct meta not found.");

		return NULL;
	}

	size = struct_size (meta);
	members = vec_new (size);
	obj->members = members;
	for (size_t i = 0; i < size; i++) {
		object_t *member;
		object_type_t field;

		field = struct_get_field_type (meta, (integer_value_t) i);
		member = object_get_default (field, (void *) code);
		UNUSED (vec_set (members, (integer_value_t) i, (void *) member));
		object_ref (member);
	}

	gc_track ((void *) obj);

	return (object_t *) obj;
}

void
structobject_traverse (object_t *obj, traverse_f fun, void *udata)
{
	vec_t *members;
	size_t size;

	members = ((structobject_t *) obj)->members;
	size = vec_size (members);
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		if (fun ((object_t *) vec_pos (members, i), udata) > 0) {
			object_t *dummy;

			dummy = object_get_default (OBJECT_TYPE_VOID, NULL);
			vec_set (members, i, dummy);
			object_ref (dummy);
		}
	}
}

void
structobject_init ()
{
	/* Make dump objects. */
	g_dump_head = strobject_new ("<struct {", strlen ("<struct {"), 1, NULL);
	if (g_dump_head == NULL) {
		fatal_error ("failed to init struct dump head.");
	}
	g_dump_tail = strobject_new ("}>", strlen ("}>"), 1, NULL);
	if (g_dump_tail == NULL) {
		fatal_error ("failed to init struct dump tail.");
	}
	g_dump_sep = strobject_new (", ", strlen (", "), 1, NULL);
	if (g_dump_sep == NULL) {
		fatal_error ("failed to init struct dump sep.");
	}
}