/*
 * strobject.c
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

#include "strobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "charobject.h"
#include "intobject.h"
#include "uint64object.h"

#define HASED(x) (((strobject_t*)(x))->hashed)
#define HASH_HANDLE(x) (((strobject_t *)(x))->hn)

#define DUMP_HEAD_LENGTH 6
#define DUMP_TAIL_LENGTH 2
#define INTERNAL_STR_LENGTH 5
#define HASH_M 0xc6a4a7935bd1e995
#define INTERNAL_HASH_SIZE 4096

static hash_t *g_internal_hash;

static unsigned int g_internal_hash_seed;

static str_t *g_dump_head;
static str_t *g_dump_tail;

/* Object ops. */
static void strobject_op_free (object_t *obj);
static void strobject_op_print (object_t *obj);
static object_t *strobject_op_dump (object_t *obj);
static object_t *strobject_op_add (object_t *obj1, object_t *obj2);
static object_t *strobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *strobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *strobject_op_index (object_t *obj1, object_t *obj2);
static object_t *strobject_op_hash (object_t *obj);
static object_t *strobject_op_binary (object_t *obj);
static object_t *strobject_op_len (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	strobject_op_free, /* Free. */
	strobject_op_print, /* Print. */
	strobject_op_dump, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	strobject_op_add, /* Addition. */
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
	strobject_op_eq, /* Equality. */
	strobject_op_cmp, /* Comparation. */
	strobject_op_index, /* Index. */
	NULL, /* Inplace index. */ /* Note that str objects are read-only! */
	strobject_op_hash, /* Hash. */
	strobject_op_binary, /* Binary. */
	strobject_op_len /* Len. */
};

/* Free. */
void
strobject_op_free (object_t *obj)
{
	/* If it's an interned str, delete it. */
	if (str_len (strobject_get_value (obj)) <= INTERNAL_STR_LENGTH && HASED (obj)) {
		hash_fast_remove (g_internal_hash, HASH_HANDLE (obj));
	}
	str_free (strobject_get_value (obj));
}

/* Print. */
static void
strobject_op_print (object_t *obj)
{
	str_t *str;
	size_t len;

	str = strobject_get_value (obj);
	len = str_len (str);
	printf ("\"");
	for (size_t i = 0; i < len; i++) {
		printf ("%c", str_pos (str, (integer_value_t) i));
	}
	printf ("\"");
}

/* Dump. */
static object_t *
strobject_op_dump (object_t *obj)
{
	str_t *temp;
	str_t *res;

	temp = str_concat (g_dump_head, strobject_get_value (obj));
	if (temp == NULL) {
		return NULL;
	}
	res = str_concat (temp, g_dump_tail);
	if (res == NULL) {
		str_free (temp);

		return NULL;
	}

	str_free (temp);

	return strobject_str_new (res, NULL);
}

/* Addition. */
static object_t *
strobject_op_add (object_t *obj1, object_t *obj2)
{
	str_t *cated;
	str_t *s1;
	str_t *s2;

	s1 = strobject_get_value (obj1);
	s2 = strobject_get_value (obj2);
	cated = str_concat (s1, s2);
	if (cated == NULL) {
		return NULL;
	}

	return strobject_str_new (cated, NULL);
}

/* Equality. */
static object_t *
strobject_op_eq (object_t *obj1, object_t *obj2)
{
	str_t *s1;
	str_t *s2;

	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	s1 = strobject_get_value (obj1);
	if (OBJECT_TYPE (obj2) == OBJECT_TYPE_STR) {
		s2 = strobject_get_value (obj2);

		return boolobject_new (str_cmp (s1, s2) == 0, NULL);
	}

	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
strobject_op_cmp (object_t *obj1, object_t *obj2)
{
	str_t *s1;
	str_t *s2;

	s1 = strobject_get_value (obj1);
	s2 = strobject_get_value (obj2);

	return intobject_new (str_cmp (s1, s2), NULL);
}

/* Index. */
static object_t *
strobject_op_index (object_t *obj1, object_t *obj2)
{
	str_t *s;
	integer_value_t pos;

	if (!INTEGER_TYPE (obj2)) {
		error ("str index must be an integer.");

		return NULL;
	}

	s = strobject_get_value (obj1);
	pos = object_get_integer (obj2);
	if (pos >= str_len (s) || pos < 0) {
		error ("str index out of bound.");

		return NULL;
	}

	return charobject_new (str_pos (s, pos), NULL);
}

/* Murmurhash2. */
static uint64_t
strobject_murmur (const char *s, size_t len, unsigned int seed)
{
	const uint8_t *start;
	const uint8_t *end;
	uint64_t m;
	int r;
	uint64_t h;

	start = (const uint8_t *) s;
	end = start + (len - (len & 7));

	m = HASH_M;
	r = 47;
	h = seed ^ (len * m);

	while (start != end) {
		uint64_t k;

		k = *((uint64_t *) start);
		k *= m;
		k ^= k >> r;
		k *= m;
		h ^= k;
		h *= m;

		start += 8;
	}

	switch (len & 7) {
		case 7:
			h ^= (uint64_t) start[6] << 48;
		case 6:
			h ^= (uint64_t) start[5] << 40;
		case 5:
			h ^= (uint64_t) start[4] << 32;
		case 4:
			h ^= (uint64_t) start[3] << 24;
		case 3:
			h ^= (uint64_t) start[2] << 16;
		case 2:
			h ^= (uint64_t) start[1] << 8;
		case 1:
			h ^= (uint64_t) start[0];
			h *= m;
	}

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}

/* Hash. */
static object_t *
strobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		str_t *str;

		str = strobject_get_value (obj);

		OBJECT_DIGEST (obj) = strobject_murmur (str_c_str (str),
												str_len (str),
												g_internal_hash_seed);
	}

	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
strobject_op_binary (object_t *obj)
{
	str_t *str;
	size_t len;
	object_t *len_obj;
	object_t *res;

	str = strobject_get_value (obj);
	len = str_len (str);

	len_obj = strobject_new (BINARY (len), sizeof (size_t), 1, NULL);
	if (len_obj == NULL) {
		return NULL;
	}
	
	res = object_add (len_obj, obj);
	object_free (len_obj);

	return res;
}

/* Len. */
static object_t *
strobject_op_len (object_t *obj)
{
	str_t *str;
	size_t len;

	str = strobject_get_value (obj);
	len = str_len (str);

	return uint64object_new ((uint64_t) len, NULL);
}

object_t *
strobject_load_binary (FILE *f)
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

	obj = strobject_new ((const char *) data, len, 1, NULL);
	pool_free (data);

	return obj;
}

static uint64_t
strobject_digest_fun (void *obj)
{
	str_t *str;

	str = strobject_get_value (obj);

	return strobject_murmur (str_c_str (str), str_len (str),
							 g_internal_hash_seed);
}

object_t *
strobject_new (const char *val, size_t len, int no_hash, void *udata)
{
	strobject_t *obj;

	if (val == NULL) {
		val = "";
	}

	/* If len is small, this object gonna be interned. */
	if (len <= INTERNAL_STR_LENGTH && !no_hash) {
		obj = (strobject_t *) hash_test (g_internal_hash, (void *) val,
			strobject_murmur (val, len, g_internal_hash_seed));
		if (obj != NULL) {
			return (object_t *) obj;
		}
	}

	obj = (strobject_t *) pool_alloc (sizeof (strobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_STR);
	OBJECT_DIGEST_FUN (obj) = strobject_digest_fun;

	obj->hn = NULL;
	obj->hashed = 0;
	obj->val = str_new (val, len);
	if (obj->val == NULL) {
		pool_free ((void *) obj);

		return NULL;
	}

	/* Add to internal hash. */
	if (len <= INTERNAL_STR_LENGTH && !no_hash) {
		void *hn;

		hn = hash_add (g_internal_hash, (void *) obj);
		if (hn == NULL) {
			object_free ((object_t *) obj);
			error ("failed to hash interned str object.");

			return NULL;
		}

		obj->hn = hn;
		obj->hashed = 1;
		object_ref ((object_t *) obj);
	}

	return (object_t *) obj;
}

object_t *
strobject_str_new (str_t *val, void *udata)
{
	strobject_t *obj;

	obj = (strobject_t *) pool_alloc (sizeof (strobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_STR);
	OBJECT_DIGEST_FUN (obj) = strobject_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

str_t *
strobject_get_value (object_t *obj)
{
	strobject_t *ob;

	ob = (strobject_t *) obj;

	return ob->val;
}

/* For hash table. */
static uint64_t
strobject_hash_fun (void *data)
{
	object_t *obj;
	str_t *str;
	size_t len;
	unsigned int seed;

	obj = (object_t *) data;
	str = strobject_get_value (obj);
	len = str_len (str);
	seed = g_internal_hash_seed;

	return strobject_murmur (str_c_str (str), len, seed);
}

static int
strobject_test_fun (void *value, void *hd)
{
	object_t *obj;

	obj = (object_t *) value;

	return (int) str_cmp_c_str (strobject_get_value (obj),
		(const char *) hd) == 0;
}

static void
strobject_cleanup_fun (void *value)
{
	object_free (value);
}

uint64_t
strobject_get_hash (object_t *obj)
{
	str_t *str;

	str = strobject_get_value (obj);

	return strobject_murmur (str_c_str (str),
							 str_len (str),
							 g_internal_hash_seed);
}

int
strobject_equal (object_t *obj1, object_t *obj2)
{
	str_t *str1;
	str_t *str2;

	str1 = strobject_get_value (obj1);
	str2 = strobject_get_value (obj2);

	return str_cmp (str1, str2) == 0;
}

const char *
strobject_c_str (object_t *obj)
{
	return str_c_str (strobject_get_value (obj));
}

void
strobject_init ()
{
	g_internal_hash = hash_new (INTERNAL_HASH_SIZE,
								strobject_hash_fun,
								strobject_test_fun,
								strobject_cleanup_fun);
	if (g_internal_hash == NULL) {
		fatal_error ("failed to init str internal hash.");
	}

	g_internal_hash_seed = random () & ((~(unsigned int) 0));

	/* Make dump head and tail. */
	g_dump_head = str_new ("<str \"", DUMP_HEAD_LENGTH);
	if (g_dump_head == NULL) {
		fatal_error ("failed to init str dump head.");
	}
	g_dump_tail = str_new ("\">", DUMP_TAIL_LENGTH);
	if (g_dump_tail == NULL) {
		fatal_error ("failed to init str dump tail.");
	}
}
