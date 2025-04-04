/*
 * vecobject.c
 * This file is part of koa
 *
 * Copyright (C) 2018 - Gordon Li
 *
 * This program is free software: you can redivecibute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is divecibuted in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>

#include "vecobject.h"
#include "pool.h"
#include "gc.h"
#include "error.h"
#include "thread.h"
#include "nullobject.h"
#include "boolobject.h"
#include "uint64object.h"
#include "strobject.h"

static object_t *g_dump_head;
static object_t *g_dump_tail;
static object_t *g_dump_sep;

/* Object ops. */
static void vecobject_op_free (object_t *obj);
static void vecobject_op_print (object_t *obj);
static object_t *vecobject_op_dump (object_t *obj);
static object_t *vecobject_op_add (object_t *obj1, object_t *obj2);
static object_t *vecobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *vecobject_op_index (object_t *obj1, object_t *obj2);
static object_t *vecobject_op_ipindex (object_t *obj1,
									   object_t *obj2, object_t *obj3);
static object_t *vecobject_op_hash (object_t *obj);
static object_t *vecobject_op_binary (object_t *obj);
static object_t *vecobject_op_len (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	vecobject_op_free, /* Free. */
	vecobject_op_print, /* Print. */
	vecobject_op_dump, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	vecobject_op_add, /* Addition. */
	NULL, /* Subvecaction. */
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
	vecobject_op_eq, /* Equality. */
	NULL, /* Comparation. */
	vecobject_op_index, /* Index. */
	vecobject_op_ipindex, /* Inplace index. */
	vecobject_op_hash, /* Hash. */
	vecobject_op_binary, /* Binary. */
	vecobject_op_len /* Len. */
};

/* Free. */
void
vecobject_op_free (object_t *obj)
{
	vec_t *vec;
	size_t size;

	vec = vecobject_get_value (obj);
	size = vec_size (vec);
	/* Unref all elements. */
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *value;

		value = (object_t *) vec_pos (vec, i);
		if (value != obj) {
			object_unref (value);
		}
	}

	vec_free (vec);

	gc_untrack ((void *) obj);
}

/* Print. */
static void
vecobject_op_print (object_t *obj)
{
	object_t *element;
	vec_t *vec;
	size_t size;

	vec = vecobject_get_value (obj);
	size = vec_size (vec);
	printf ("[");
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		element = (object_t *) vec_pos (vec, i);
		object_print (element);
		if (i != (integer_value_t) size - 1) {
			printf (",");
		}
	}
	printf ("]");
}

static object_t *
vecobject_dump_concat (object_t *obj1, object_t *obj2, int free)
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
vecobject_op_dump (object_t *obj)
{
	object_t *res;
	object_t *element;
	vec_t *vec;
	size_t size;

	vec = vecobject_get_value (obj);
	size = vec_size (vec);
	if (!size) {
		return object_add (g_dump_head, g_dump_tail);
	}

	res = g_dump_head;
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *dump;

		element = (object_t *) vec_pos (vec, i);
		dump = object_dump (element);
		if (dump == NULL) {
			object_free (res);

			return NULL;
		}

		if ((res = vecobject_dump_concat (res, dump, 1)) == NULL) {
			return NULL;
		}
		if (i != (integer_value_t) size - 1 && (res = vecobject_dump_concat (res, g_dump_sep, 0)) == NULL) {
			return NULL;
		}
	}

	return vecobject_dump_concat (res, g_dump_tail, 0);
}

/* Addition. */
static object_t *
vecobject_op_add (object_t *obj1, object_t *obj2)
{
	vec_t *v1;
	vec_t *v2;
	vec_t *cated;

	v1 = vecobject_get_value (obj1);
	v2 = vecobject_get_value (obj2);
	cated = vec_concat (v1, v2);
	if (cated == NULL) {
		return NULL;
	}

	return vecobject_vec_new (cated, NULL);
}

/* Equality. */
static object_t *
vecobject_op_eq (object_t *obj1, object_t *obj2)
{
	return boolobject_new (obj1 == obj2, NULL);
}

/* Index. */
static object_t *
vecobject_op_index (object_t *obj1, object_t *obj2)
{
	vec_t *v;
	integer_value_t pos;

	if (!INTEGER_TYPE (obj2)) {
		error ("vec index must be an integer.");

		return NULL;
	}

	v = vecobject_get_value (obj1);
	pos = object_get_integer (obj2);
	if (pos >= vec_size (v) || pos < 0) {
		error ("vec index out of bound.");

		return NULL;
	}

	return (object_t *) vec_pos (v, pos);
}

/* Inplace index. */
static object_t *
vecobject_op_ipindex (object_t *obj1, object_t *obj2, object_t *obj3)
{
	vec_t *v;
	integer_value_t pos;
	object_t *prev;

	if (!INTEGER_TYPE (obj2)) {
		error ("vec index must be an integer.");

		return NULL;
	}

	v = vecobject_get_value (obj1);
	pos = object_get_integer (obj2);
	if (pos >= vec_size (v) || pos < 0) {
		error ("vec index out of bound.");

		return NULL;
	}

	prev = (object_t *) vec_set (v, pos, obj3);
	object_ref (obj3);
	object_unref (prev);

	return obj3;
}

/* Hash. */
static object_t *
vecobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_address_hash ((void *) obj);
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
vecobject_op_binary (object_t *obj)
{
	vec_t *vec;
	size_t size;
	object_t *temp;

	vec = vecobject_get_value (obj);
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

/* Len. */
static object_t *
vecobject_op_len (object_t *obj)
{
	vec_t *vec;
	size_t size;

	vec = vecobject_get_value (obj);
	size = vec_size (vec);

	return uint64object_new ((uint64_t) size, NULL);
}

object_t *
vecobject_load_binary (FILE *f)
{
	size_t size;
	vec_t *vec;

	if (fread (&size, sizeof (size_t), 1, f) != 1) {
		error ("failed to load size while load vec.");

		return NULL;
	}

	vec = vec_new (size);
	if (vec == NULL) {
		return NULL;
	}

	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *obj;

		obj = object_load_binary (f);
		if (obj == NULL) {
			for (integer_value_t j = 0; j < (integer_value_t) i; j++) {
				object_free ((object_t *) vec_pos (vec, j));
			}
			vec_free (vec);

			return NULL;
		}

		UNUSED (vec_set (vec, i, (void *) obj));
		object_ref (obj);
	}

	return vecobject_vec_new (vec, NULL);
}

object_t *
vecobject_load_buf (const char **buf, size_t *len)
{
	size_t size;
	vec_t *vec;

	if (*len < sizeof (size_t)) {
		error ("failed to load size while load vec.");

		return NULL;
	}

	size = *(size_t *) *buf;
	*buf += sizeof (size_t);
	*len -= sizeof (size_t);

	vec = vec_new (size);
	if (vec == NULL) {
		return NULL;
	}

	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *obj;

		obj = object_load_buf (buf, len);
		if (obj == NULL) {
			for (integer_value_t j = 0; j < (integer_value_t) i; j++) {
				object_free ((object_t *) vec_pos (vec, j));
			}
			vec_free (vec);

			return NULL;
		}

		UNUSED (vec_set (vec, i, (void *) obj));
		object_ref (obj);
	}

	return vecobject_vec_new (vec, NULL);
}

static int
vecobject_empty_init (vecobject_t *obj)
{
	vec_t *vec;
	size_t size;

	vec = obj->val;
	size = vec_size (vec);
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *null_obj;

		null_obj = nullobject_new (NULL);
		if (null_obj == NULL) {
			return 0;
		}

		UNUSED (vec_set (vec, i, (void *) null_obj));
		object_ref (null_obj);
	}

	return 1;
}

static uint64_t
vecobject_digest_fun (void *obj)
{
	return object_address_hash (obj);
}

object_t *
vecobject_new (size_t len, void *udata)
{
	vecobject_t *obj;

	obj = (vecobject_t *) pool_alloc (sizeof (vecobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_VEC, udata);
	OBJECT_DIGEST_FUN (obj) = vecobject_digest_fun;

	obj->val = vec_new (len);
	if (obj->val == NULL) {
		pool_free ((void *) obj);

		return NULL;
	}

	/* Full obj with 'null' object. */
	if (vecobject_empty_init (obj) == 0) {
		object_free ((object_t *) obj);

		return NULL;
	}

	gc_track ((void *) obj);

	return (object_t *) obj;
}

object_t *
vecobject_vec_new (vec_t *val, void *udata)
{
	vecobject_t *obj;

	obj = (vecobject_t *) pool_alloc (sizeof (vecobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_VEC, udata);
	OBJECT_DIGEST_FUN (obj) = vecobject_digest_fun;

	obj->val = val;

	gc_track ((void *) obj);

	return (object_t *) obj;
}

vec_t *
vecobject_get_value (object_t *obj)
{
	vecobject_t *ob;

	ob = (vecobject_t *) obj;

	return ob->val;
}

void
vecobject_traverse (object_t *obj, traverse_f fun, void *udata)
{
	vec_t *vec;
	size_t size;

	vec = vecobject_get_value (obj);
	size = vec_size (vec);
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *element;

		element = (object_t *) vec_pos (vec, i);
		object_traverse (element, fun, udata);
		if (fun (element, udata) > 0) {
			object_t *dummy;

			dummy = object_get_default (OBJECT_TYPE_VOID, NULL);
			vec_set (vec, i, dummy);
			object_ref (dummy);
		}
	}
}

int
vecobject_append (object_t *obj, object_t *element)
{
	vec_t *vec;

	vec = vecobject_get_value (obj);
	if (vec_push_back (vec, (void *) element) == 0) {
		return 0;
	}

	object_ref (element);

	return 1;
}

int
vecobject_remove (object_t *obj, integer_value_t pos)
{
	vec_t *vec;

	vec = vecobject_get_value (obj);
	return vec_remove (vec, pos);
}

object_t *
vecobject_copy (object_t *obj)
{
	vec_t *new_vec;
	vec_t *old_vec;
	size_t size;

	old_vec = vecobject_get_value (obj);
	size = vec_size (old_vec);
	new_vec = vec_new (size);
	if (new_vec == NULL) {
		return NULL;
	}

	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *new_element;

		new_element = object_copy ((object_t *) vec_pos (old_vec, i));
		if (new_element == NULL) {
			for (integer_value_t j = 0; j < i; j++) {
				object_free ((object_t *) vec_pos (new_vec, j));
			}
			vec_free (new_vec);

			return NULL;
		}
		vec_set (new_vec, i, (void *) new_element);
		object_ref (new_element);
	}

	return vecobject_vec_new (new_vec, NULL);
}

void
vecobject_init ()
{
	if (!thread_is_main_thread ()) {
		return;
	}

	/* Make dump objects. */
	g_dump_head = strobject_new ("<vec [", strlen ("<vec ["), 1, NULL);
	if (g_dump_head == NULL) {
		fatal_error ("failed to init vec dump head.");
	}
	object_set_const (g_dump_head);
	g_dump_tail = strobject_new ("]>", strlen ("]>"), 1, NULL);
	if (g_dump_tail == NULL) {
		fatal_error ("failed to init vec dump tail.");
	}
	object_set_const (g_dump_tail);
	g_dump_sep = strobject_new (", ", strlen (", "), 1, NULL);
	if (g_dump_sep == NULL) {
		fatal_error ("failed to init vec dump sep.");
	}
	object_set_const (g_dump_sep);
}
