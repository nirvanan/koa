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

#include "dictobject.h"
#include "pool.h"
#include "hash.h"
#include "error.h"
#include "nullobject.h"

/* Object ops. */
static void dictobject_op_free (object_t *obj);
static object_t *dictobject_op_index (object_t *obj1, object_t *obj2);
static object_t *dictobject_op_ipindex (object_t *obj1,
	object_t *obj2, object_t *obj3);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	dictobject_op_free, /* Free. */
	NULL, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	NULL, /* Addition. */
	NULL, /* Subdictaction. */
	NULL, /* Multiplication. */
	NULL, /* Division. */
	NULL, /* Mod. */
	NULL, /* Bitwise and. */
	NULL, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	NULL, /* Logic and. */
	NULL, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	NULL, /* Equality. */
	NULL, /* Comparation. */
	dictobject_op_index, /* Index. */
	dictobject_op_ipindex, /* Inplace index. */
	NULL
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
		error ("failed to get dict pairs while freeing.");

		return;
	}

	size = vec_size (pairs);
	/* Unref all pairs. */
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_unref ((object_t *) DICT_PAIR_KEY (vec_pos (pairs, i)));
		object_unref ((object_t *) DICT_PAIR_VALUE (vec_pos (pairs, i)));
	}

	vec_free (pairs);
	dict_free (dict);
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

	if (!NUMBERICAL_TYPE (obj2) && OBJECT_TYPE (obj2) != OBJECT_TYPE_STR) {
		error ("dict index must be a number or str.");

		return NULL;
	}

	dict = dictobject_get_value (obj1);

	/* obj3 is returned if successfully inserted. */
	return dict_set (dict, (void *) obj2, (void *) obj3);
}

/* Hash function for dict. */
static uint64_t
dictobject_hash_fun (void *data)
{
	object_t *obj;
	object_t *digest;

	obj = (object_t *) data;
	if (OBJECT_DIGEST (obj) > 0) {
		return OBJECT_DIGEST (obj);
	}

	digest = object_hash (obj);

	object_free (digest);

	return OBJECT_DIGEST (obj);
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
dictobject_new (void *udata)
{
	dictobject_t *obj;

	obj = (dictobject_t *) pool_alloc (sizeof (dictobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_DICT);

	obj->val = dict_new (dictobject_hash_fun, dictobject_test_fun);
	if (obj->val == NULL) {
		pool_free ((void *) obj);

		return NULL;
	}

	return (object_t *) obj;
}

object_t *
dictobject_dict_new (dict_t *val, void *udata)
{
	dictobject_t *obj;

	obj = (dictobject_t *) pool_alloc (sizeof (dictobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_DICT);

	obj->val = val;

	return (object_t *) obj;
}

dict_t *
dictobject_get_value (object_t *obj)
{
	dictobject_t *ob;

	ob = (dictobject_t *) obj;

	return ob->val;
}

