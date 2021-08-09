/*
 * dictobject.c
 * This file is part of koa
 *
 * Copyright (C) 2018 - Gordon Li
 *
 * This program is free software: you can redidictibute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is didictibuted in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "dictobject.h"
#include "pool.h"
#include "hash.h"
#include "gc.h"
#include "error.h"
#include "nullobject.h"
#include "boolobject.h"
#include "uint64object.h"
#include "strobject.h"

/* Object ops. */
static void dictobject_op_free (object_t *obj);
static void dictobject_op_print (object_t *obj);
static object_t *dictobject_op_dump (object_t *obj);
static object_t *dictobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *dictobject_op_index (object_t *obj1, object_t *obj2);
static object_t *dictobject_op_ipindex (object_t *obj1,
										object_t *obj2, object_t *obj3);
static object_t *dictobject_op_hash (object_t *obj);
static object_t *dictobject_op_binary (object_t *obj);
static object_t *dictobject_op_len (object_t *obj);

static object_t *g_dump_head;
static object_t *g_dump_tail;
static object_t *g_dump_sep;
static object_t *g_dump_map;

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	dictobject_op_free, /* Free. */
	dictobject_op_print, /* Print. */
	dictobject_op_dump, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	NULL, /* Addition. */
	NULL, /* Subdictaction. */
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
	dictobject_op_eq, /* Equality. */
	NULL, /* Comparation. */
	dictobject_op_index, /* Index. */
	dictobject_op_ipindex, /* Inplace index. */
	dictobject_op_hash, /* Hash. */
	dictobject_op_binary, /* Binary. */
	dictobject_op_len /* Binary. */
};

/* Free. */
void
dictobject_op_free (object_t *obj)
{
	dict_t *dict;
	vec_t *pairs;
	size_t size;

	dict = dictobject_get_value (obj);
	pairs = dict_pairs (dict);
	if (pairs == NULL) {
		return;
	}

	size = vec_size (pairs);
	/* Unref all pairs. */
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *key;
		object_t *value;

		key = (object_t *) DICT_PAIR_KEY (vec_pos (pairs, i));
		value = (object_t *) DICT_PAIR_VALUE (vec_pos (pairs, i));
		object_unref (key);
		if (value != obj) {
			object_unref (value);
		}
	}

	vec_free (pairs);
	dict_free (dict);

	gc_untrack ((void *) obj);
}

/* Print. */
static void
dictobject_op_print (object_t *obj)
{
	dict_t *dict;
	vec_t *pairs;
	size_t size;
	object_t *key;
	object_t *value;

	dict = dictobject_get_value (obj);
	pairs = dict_pairs (dict);
	if (pairs == NULL) {
		error ("failed to get dict pairs while printing.");

		return;
	}

	size = vec_size (pairs);
	printf ("{");
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		key = (object_t *) DICT_PAIR_KEY (vec_pos (pairs, i));
		value = (object_t *) DICT_PAIR_VALUE (vec_pos (pairs, i));
		object_print (key);
		printf (":");
		object_print (value);
		if (i != (integer_value_t) size - 1) {
			printf (",");
		}
	}
	printf ("}");
	vec_free (pairs);
}

static object_t *
dictobject_dump_concat (object_t *obj1, object_t *obj2, int free)
{
	object_t *res;

	res = object_add (obj1, obj2);
	object_free (obj1);
	if (free) {
		object_free (obj2);
	}

	return res;
}

static object_t *
dictobject_pair_concat (object_t *key, object_t *value)
{
	object_t *dump_key;
	object_t *dump_value;
	object_t *res;
	object_t *temp;

	dump_key = object_dump (key);
	if (dump_key == NULL) {
		return NULL;
	}
	dump_value = object_dump (value);
	if (dump_value == NULL) {
		object_free (dump_key);

		return NULL;
	}

	temp = object_add (dump_key, g_dump_map);
	if (temp == NULL) {
		object_free (dump_key);
		object_free (dump_value);

		return NULL;
	}

	res = object_add (temp, dump_value);

	object_free (dump_key);
	object_free (dump_value);
	object_free (temp);

	return res;
}

/* Dump. */
static object_t *
dictobject_op_dump (object_t *obj)
{
	object_t *res;
	dict_t *dict;
	vec_t *pairs;
	size_t size;
	object_t *key;
	object_t *value;
	object_t *pair;

	dict = dictobject_get_value (obj);
	pairs = dict_pairs (dict);
	if (pairs == NULL) {
		error ("failed to get dict pairs while dumping.");

		return NULL;
	}

	size = vec_size (pairs);
	if (!size) {
		return object_add (g_dump_head, g_dump_tail);
	}

	key = (object_t *) DICT_PAIR_KEY (vec_pos (pairs, 0));
	value = (object_t *) DICT_PAIR_VALUE (vec_pos (pairs, 0));
	pair = dictobject_pair_concat (key, value);
	res = object_add (g_dump_head, pair);
	if (res == NULL) {
		object_free (pair);

		return NULL;
	}
	object_free (pair);

	for (integer_value_t i = 1; i < (integer_value_t) size; i++) {
		object_t *dump;

		key = (object_t *) DICT_PAIR_KEY (vec_pos (pairs, 0));
		value = (object_t *) DICT_PAIR_VALUE (vec_pos (pairs, 0));
		dump = dictobject_pair_concat (key, value);
		if (dump == NULL) {
			object_free (res);

			return NULL;
		}

		if ((res = dictobject_dump_concat (res, g_dump_sep, 0)) == NULL ||
			(res = dictobject_dump_concat (res, dump, 1)) == NULL) {
			return NULL;
		}
	}

	vec_free (pairs);

	return dictobject_dump_concat (res, g_dump_tail, 0);
}

/* Equality. */
static object_t *
dictobject_op_eq (object_t *obj1, object_t *obj2)
{
	return boolobject_new (obj1 == obj2, NULL);
}

/* Index. */
static object_t *
dictobject_op_index (object_t *obj1, object_t *obj2)
{
	dict_t *dict;
	void *value;

	if (!NUMBERICAL_TYPE (obj2) && OBJECT_TYPE (obj2) != OBJECT_TYPE_STR) {
		error ("dict index must be a number or str.");

		return NULL;
	}

	dict = dictobject_get_value (obj1);
	value = dict_get (dict, (void *) obj2);

	/* Return the 'null' object if the key is not presented. */
	return value == NULL? nullobject_new (NULL): (object_t *) value;
}

/* Inplace index. */
static object_t *
dictobject_op_ipindex (object_t *obj1, object_t *obj2, object_t *obj3)
{
	dict_t *dict;
	object_t *prev;
	object_t *res;

	if (!NUMBERICAL_TYPE (obj2) && !OBJECT_IS_STR (obj2)) {
		error ("dict index must be a number or str.");

		return NULL;
	}

	/* obj3 is returned if successfully inserted. */
	dict = dictobject_get_value (obj1);
	prev = (object_t *) dict_get (dict, (void *) obj2);
	res = (object_t *) dict_set (dict, (void *) obj2, (void *) obj3);
	if (res == NULL) {
		return res;
	}

	object_ref (obj3);
	object_ref (obj2);
	if (prev != NULL) {
		object_unref (prev);
	}

	return obj3;
}

/* Hash. */
static object_t *
dictobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_address_hash ((void *) obj);
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

static object_t *
dictobject_binary_concat (object_t *obj1, object_t *obj2)
{
	object_t *res;

	obj2 = object_binary (obj2);
	if (obj2 == NULL) {
		object_free (obj1);

		return NULL;
	}

	res = object_add (obj1, obj2);
	object_free (obj1);
	object_free (obj2);

	return res;
}

/* Binary. */
static object_t *
dictobject_op_binary (object_t *obj)
{
	dict_t *dict;
	vec_t *pairs;
	size_t size;
	object_t *temp;

	dict = dictobject_get_value (obj);
	size = dict_size (dict);

	temp = strobject_new (BINARY (size), sizeof (size_t), 1, NULL);
	if (temp == NULL) {
		return NULL;
	}

	pairs = dict_pairs (dict);
	if (pairs == NULL) {
		object_free (temp);

		return NULL;
	}

	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *obj;

		obj = (object_t *) DICT_PAIR_KEY (vec_pos (pairs, i));
		temp = dictobject_binary_concat (temp, obj);
		if (temp == NULL) {
			return NULL;
		}

		obj = (object_t *) DICT_PAIR_VALUE (vec_pos (pairs, i));
		temp = dictobject_binary_concat (temp, obj);
		if (temp == NULL) {
			return NULL;
		}
	}
	
	vec_free (pairs);

	return temp;
}

/* Len. */
static object_t *
dictobject_op_len (object_t *obj)
{
	dict_t *dict;
	size_t size;

	dict = dictobject_get_value (obj);
	size = dict_size (dict);

	return uint64object_new ((uint64_t) size, NULL);
}

/* Hash function for dict. */
static uint64_t
dictobject_hash_fun (void *data)
{
	object_t *obj;

	obj = (object_t *) data;

	return object_digest (obj);
}

static int
dictobject_test_fun (void *key, void *hd)
{
	object_t *res;

	res = (object_t *) object_equal ((object_t *) key, (object_t *) hd);
	if (res == NULL) {
		error ("hash test failed.");

		return 0;
	}

	return (int) object_get_integer (res);
}

object_t *
dictobject_load_binary (FILE *f)
{
	size_t size;
	dict_t *dict;

	if (fread (&size, sizeof (size_t), 1, f) != 1) {
		error ("failed to load size while load dict.");

		return NULL;
	}

	dict = dict_new (dictobject_hash_fun, dictobject_test_fun);
	if (dict == NULL) {
		return NULL;
	}

	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *key;
		object_t *value;

		key = object_load_binary (f);
		if (key == NULL) {
			dict_free (dict);

			return NULL;
		}
		value = object_load_binary (f);
		if (value == NULL) {
			dict_free (dict);
			object_free (key);

			return NULL;
		}


		if (dict_set (dict, (void *) key, (void *) value) == NULL) {
			dict_free (dict);
			object_free (key);
			object_free (value);

			return NULL;
		}

		object_ref (key);
		object_ref (value);
	}

	return dictobject_dict_new (dict, NULL);
}

static uint64_t
dictobject_digest_fun (void *obj)
{
	return object_address_hash (obj);
}

object_t *
dictobject_new (void *udata)
{
	dictobject_t *obj;

	obj = (dictobject_t *) pool_alloc (sizeof (dictobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_DICT, udata);
	OBJECT_DIGEST_FUN (obj) = dictobject_digest_fun;

	obj->val = dict_new (dictobject_hash_fun, dictobject_test_fun);
	if (obj->val == NULL) {
		pool_free ((void *) obj);

		return NULL;
	}

	gc_track ((void *) obj);

	return (object_t *) obj;
}

object_t *
dictobject_dict_new (dict_t *val, void *udata)
{
	dictobject_t *obj;

	obj = (dictobject_t *) pool_alloc (sizeof (dictobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_DICT, udata);
	OBJECT_DIGEST_FUN (obj) = dictobject_digest_fun;

	obj->val = val;

	gc_track ((void *) obj);

	return (object_t *) obj;
}

dict_t *
dictobject_get_value (object_t *obj)
{
	dictobject_t *ob;

	ob = (dictobject_t *) obj;

	return ob->val;
}

void
dictobject_traverse (object_t *obj, traverse_f fun, void *udata)
{
	dict_t *dict;
	vec_t *pairs;
	size_t size;

	dict = dictobject_get_value (obj);
	pairs = dict_pairs (dict);
	if (pairs == NULL) {
		return;
	}

	size = vec_size (pairs);
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *key;
		object_t *value;

		key = (object_t *) DICT_PAIR_KEY (vec_pos (pairs, i));
		value = (object_t *) DICT_PAIR_VALUE (vec_pos (pairs, i));
		UNUSED (fun (key, udata));
		if (fun (value, udata) > 0) {
			object_t *dummy;

			dummy = object_get_default (OBJECT_TYPE_VOID, NULL);
			dict_set (dict, (void *) key, (void *) dummy);
			object_ref (dummy);
		}
	}

	vec_free (pairs);
}

void
dictobject_init ()
{
	/* Make dump objects. */
	g_dump_head = strobject_new ("<dict {", strlen ("<dict {"), 1, NULL);
	if (g_dump_head == NULL) {
		fatal_error ("failed to init dict dump head.");
	}
	g_dump_tail = strobject_new ("}>", strlen ("}>"), 1, NULL);
	if (g_dump_tail == NULL) {
		fatal_error ("failed to init dict dump tail.");
	}
	g_dump_sep = strobject_new (", ", strlen (", "), 1, NULL);
	if (g_dump_sep == NULL) {
		fatal_error ("failed to init dict dump sep.");
	}
	g_dump_map = strobject_new (": ", strlen (": "), 1, NULL);
	if (g_dump_map == NULL) {
		fatal_error ("failed to init dict dump map.");
	}
}
