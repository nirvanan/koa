/*
 * unionobject.c
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

#include "unionobject.h"
#include "pool.h"
#include "gc.h"
#include "error.h"
#include "compound.h"
#include "nullobject.h"
#include "boolobject.h"
#include "uint64object.h"
#include "strobject.h"

static object_t *g_dump_head;
static object_t *g_dump_tail;
static object_t *g_dump_unset;

/* Object ops. */
static void unionobject_op_free (object_t *obj);
static void unionobject_op_print (object_t *obj);
static object_t *unionobject_op_dump (object_t *obj);
static object_t *unionobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *unionobject_op_hash (object_t *obj);
static object_t *unionobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	unionobject_op_free, /* Free. */
	unionobject_op_print, /* Print. */
	unionobject_op_dump, /* Dump. */
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
	unionobject_op_eq, /* Equality. */
	NULL, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	unionobject_op_hash, /* Hash. */
	unionobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Free. */
void
unionobject_op_free (object_t *obj)
{
	unionobject_t *union_obj;

	union_obj = (unionobject_t *) obj;
	if (union_obj->value != NULL) {
		object_unref ((object_t *) union_obj->value);
	}

	gc_untrack ((void *) obj);
}

/* Print. */
static void
unionobject_op_print (object_t *obj)
{
	unionobject_t *union_obj;

	union_obj = (unionobject_t *) obj;
	printf ("<");
	if (union_obj->value != NULL) {
		object_print ((object_t *) union_obj->value);
	}
	else {
		printf ("unset");
	}
	printf (">");	
}

static object_t *
unionobject_dump_concat (object_t *obj1, object_t *obj2, int free)
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
unionobject_op_dump (object_t *obj)
{
	object_t *res;
	unionobject_t *union_obj;

	union_obj = (unionobject_t *) obj;
	if (union_obj->value != NULL) {
		object_t *dump;

		dump = object_dump ((object_t *) union_obj->value);
		if (dump == NULL) {
			return NULL;
		}
		res = object_add (g_dump_head, dump);
	}
	else {
		res = object_add (g_dump_head, g_dump_unset);
	}
	if (res == NULL) {
		return NULL;
	}

	return unionobject_dump_concat (res, g_dump_tail, 0);
}

/* Equality. */
static object_t *
unionobject_op_eq (object_t *obj1, object_t *obj2)
{
	return boolobject_new (obj1 == obj2, NULL);
}

/* Hash. */
static object_t *
unionobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_address_hash ((void *) obj);
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
unionobject_op_binary (object_t *obj)
{
	unionobject_t *union_obj;

	union_obj = (unionobject_t *) obj;
	if (union_obj->value != NULL) {
		return object_binary ((object_t *) union_obj->value);
	}

	return object_binary (object_get_default (OBJECT_TYPE_VOID, NULL));
}

static uint64_t
unionobject_digest_fun (void *obj)
{
	return object_address_hash (obj);
}

object_t *
unionobject_load_binary (object_type_t type, FILE *f)
{
	unionobject_t *union_obj;
	object_t *value_obj;

	value_obj = object_load_binary (f);
	if (value_obj == NULL) {
		return NULL;
	}

	union_obj = (unionobject_t *) pool_alloc (sizeof (unionobject_t));
	if (union_obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (union_obj, type, NULL);
	OBJECT_DIGEST_FUN (union_obj) = unionobject_digest_fun;

	if (OBJECT_IS_DUMMY (value_obj)) {
		object_free (value_obj);
		union_obj->value = NULL;
	}
	else {
		union_obj->value = value_obj;
		object_ref (value_obj);
	}

	gc_track ((void *) union_obj);

	return (object_t *) union_obj;
}

object_t *
unionobject_load_buf (object_type_t type, const char **buf, size_t *len)
{
	unionobject_t *union_obj;
	object_t *value_obj;

	value_obj = object_load_buf (buf, len);
	if (value_obj == NULL) {
		return NULL;
	}

	union_obj = (unionobject_t *) pool_alloc (sizeof (unionobject_t));
	if (union_obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (union_obj, type, NULL);
	OBJECT_DIGEST_FUN (union_obj) = unionobject_digest_fun;

	if (OBJECT_IS_DUMMY (value_obj)) {
		object_free (value_obj);
		union_obj->value = NULL;
	}
	else {
		union_obj->value = value_obj;
		object_ref (value_obj);
	}

	gc_track ((void *) union_obj);

	return (object_t *) union_obj;
}

object_t *
unionobject_new (code_t *code, object_type_t type, void *udata)
{
	unionobject_t *obj;
	compound_t *meta;

	obj = (unionobject_t *) pool_alloc (sizeof (unionobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, type, udata);
	OBJECT_DIGEST_FUN (obj) = unionobject_digest_fun;

	meta = code_get_union (code, type);
	if (meta == NULL) {
		pool_free ((void *) obj);
		error ("union meta not found.");

		return NULL;
	}

	obj->value = NULL;

	gc_track ((void *) obj);

	return (object_t *) obj;
}

void
unionobject_traverse (object_t *obj, traverse_f fun, void *udata)
{
	unionobject_t *union_obj;

	union_obj = (unionobject_t *) obj;
	if (union_obj->value != NULL) {
		if (fun ((object_t *) union_obj->value, udata) > 0) {
			union_obj->value = NULL;
		}
	}
}

object_t *
unionobject_get_member (object_t *obj, object_t *name, code_t *code)
{
	compound_t *meta;
	integer_value_t pos;
	object_type_t target_type;
	unionobject_t *union_obj;

	meta = code_get_union (code, OBJECT_TYPE (obj));
	if (meta == NULL) {
		error ("union not found.");

		return NULL;
	}

	pos = compound_find_field (meta, strobject_get_value (name));
	if (pos == -1) {
		error ("%s has no member named %s.", str_c_str (compound_get_name (meta)), strobject_c_str (name));

		return NULL;
	}

	target_type = compound_get_field_type (meta, pos);
	if (target_type == OBJECT_TYPE_ERR) {
		error ("the type of %s member %s is unknown.", str_c_str (compound_get_name (meta)), strobject_c_str (name));

		return NULL;
	}

	union_obj = (unionobject_t *) obj;
	if (union_obj->value == NULL) {
		return object_get_default (target_type, NULL);
	}

	return object_cast ((object_t *) union_obj->value, target_type);
}

object_t *
unionobject_store_member (object_t *obj, object_t *name,
						  object_t *value, code_t *code)
{
	compound_t *meta;
	integer_value_t pos;
	object_type_t target_type;
	unionobject_t *union_obj;
	object_t *prev;

	meta = code_get_union (code, OBJECT_TYPE (obj));
	if (meta == NULL) {
		error ("union not found.");

		return NULL;
	}

	pos = compound_find_field (meta, strobject_get_value (name));
	if (pos == -1) {
		error ("%s has no member named %s.", str_c_str (compound_get_name (meta)), strobject_c_str (name));

		return NULL;
	}

	target_type = compound_get_field_type (meta, pos);
	if (target_type == OBJECT_TYPE_ERR) {
		error ("the type of %s member %s is unknown.", str_c_str (compound_get_name (meta)), strobject_c_str (name));

		return NULL;
	}

	union_obj = (unionobject_t *) obj;
	prev = (object_t *) union_obj->value;
	if (OBJECT_TYPE (value) == target_type) {
		union_obj->value = value;
		object_ref (value);
	}
	else {
		object_t *new_value;

		new_value = object_cast (value, target_type);
		if (new_value == NULL) {
			return NULL;
		}
		union_obj->value = new_value;
		object_ref (new_value);
	}

	if (prev != NULL) {
		object_unref (prev);
	}

	return (object_t *) union_obj->value;
}

object_t *
unionobject_copy (object_t *obj)
{
	unionobject_t *new_obj;
	unionobject_t *old_obj;
	object_t *new_value;

	old_obj = (unionobject_t *) obj;
	new_obj = (unionobject_t *) pool_alloc (sizeof (unionobject_t));
	if (new_obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (new_obj, OBJECT_TYPE (obj), OBJECT_UDATA (obj));
	OBJECT_DIGEST_FUN (new_obj) = unionobject_digest_fun;

	new_value = object_copy ((object_t *) old_obj->value);
	if (new_value == NULL) {
		pool_free ((void *) new_obj);

		return NULL;
	}

	new_obj->value = new_value;
	object_ref (new_value);

	gc_track ((void *) new_obj);

	return (object_t *) new_obj;
}

void
unionobject_init ()
{
	/* Make dump objects. */
	g_dump_head = strobject_new ("<union <", strlen ("<union <"), 1, NULL);
	if (g_dump_head == NULL) {
		fatal_error ("failed to init union dump head.");
	}
	g_dump_tail = strobject_new (">>", strlen (">>"), 1, NULL);
	if (g_dump_tail == NULL) {
		fatal_error ("failed to init union dump tail.");
	}
	g_dump_unset = strobject_new ("unset", strlen ("unset"), 1, NULL);
	if (g_dump_unset == NULL) {
		fatal_error ("failed to init union dump unset tag.");
	}
}
