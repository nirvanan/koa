/*
 * charobject.c
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

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "charobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "intobject.h"
#include "uint64object.h"
#include "strobject.h"

#define DUMP_BUF_SIZE 12

#define CHAR_CACHE_MIN CHAR_MIN
#define CHAR_CACHE_MAX CHAR_MAX
#define CHAR_CACHE_SIZE ((size_t)(CHAR_CACHE_MAX-CHAR_CACHE_MIN+1))

#define CHAR_HAS_CACHE(x) ((x)>=CHAR_CACHE_MIN&&(x)<=CHAR_CACHE_MAX)

#define CHAR_CACHE_INDEX(x) ((size_t)((x)-CHAR_CACHE_MIN))

/* Char objects are all cached. */
static __thread object_t *g_char_cache[CHAR_CACHE_SIZE];

/* Object ops. */
static object_t *charobject_op_lnot (object_t *obj);
static void charobject_op_print (object_t *obj);
static object_t *charobject_op_dump (object_t *obj);
static object_t *charobject_op_land (object_t *obj1, object_t *obj2);
static object_t *charobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *charobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *charobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *charobject_op_hash (object_t *obj);
static object_t *charobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	charobject_op_lnot, /* Logic Not. */
	NULL, /* Free. */
	charobject_op_print, /* Print. */
	charobject_op_dump, /* Dump. */
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
	charobject_op_land, /* Logic and. */
	charobject_op_lor, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	charobject_op_eq, /* Equality. */
	charobject_op_cmp, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	charobject_op_hash, /* Hash. */
	charobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Logic Not. */
static object_t *
charobject_op_lnot (object_t *obj)
{
	return boolobject_new (!charobject_get_value (obj), NULL);
}

/* Print. */
static void
charobject_op_print (object_t *obj)
{
	printf ("%c", charobject_get_value (obj));
}

/* Dump. */
static object_t *
charobject_op_dump (object_t *obj)
{
	char buf[DUMP_BUF_SIZE];

	snprintf (buf, DUMP_BUF_SIZE, "<char %d>", charobject_get_value (obj));

	return strobject_new (buf, strlen (buf), 1, NULL);
}

/* Logic and. */
static object_t *
charobject_op_land (object_t *obj1, object_t *obj2)
{
	char val1;

	val1 = charobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
charobject_op_lor (object_t *obj1, object_t *obj2)
{
	char val1;

	val1 = charobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Equality. */
static object_t *
charobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		char val1;
	
		val1 = charobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}
	
	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
charobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

/* Hash. */
static object_t *
charobject_op_hash(object_t *obj)
{
	if (OBJECT_DIGEST (obj) == 0) {
		OBJECT_DIGEST (obj) = object_integer_hash (object_get_integer (obj));
	}

	/* The digest is already computed. */
	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
charobject_op_binary (object_t *obj)
{
	return strobject_new (BINARY (((charobject_t *) obj)->val),
						  sizeof (char), 1, NULL);
}

object_t *
charobject_load_binary (FILE *f)
{
	char val;

	if (fread (&val, sizeof (char), 1, f) != 1) {
		error ("failed to load char binary.");

		return NULL;
	}

	return charobject_new (val, NULL);
}

object_t *
charobject_load_buf (const char **buf, size_t *len)
{
	char val;

	if (*len < sizeof (char)) {
		error ("failed to load char buffer.");

		return NULL;
	}

	val = *(char *) *buf;
	*buf += sizeof (char);
	*len -= sizeof (char);

	return charobject_new (val, NULL);
}

static uint64_t
charobject_digest_fun (void *obj)
{
	return object_integer_hash (object_get_integer ((object_t *) obj));
}

object_t *
charobject_new (char val, void *udata)
{
	charobject_t *obj;

	/* Return cached object. */
	if (CHAR_HAS_CACHE (val) && g_char_cache[CHAR_CACHE_INDEX (val)] != NULL) {
		return g_char_cache[CHAR_CACHE_INDEX (val)];
	}

	obj = (charobject_t *) pool_alloc (sizeof (charobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_CHAR, udata);
	OBJECT_DIGEST_FUN (obj) = charobject_digest_fun;

	obj->val = val;

	return (object_t *) obj;
}

char
charobject_get_value (object_t *obj)
{
	charobject_t *ob;

	ob = (charobject_t *) obj;

	return ob->val;
}

void
charobject_init ()
{
	/* Make char cache. */
	for (char i = CHAR_CACHE_MIN; i <= CHAR_CACHE_MAX; i++) {
		g_char_cache[CHAR_CACHE_INDEX (i)] = charobject_new (i, NULL);
		if (g_char_cache[CHAR_CACHE_INDEX (i)] == NULL) {
			fatal_error ("failed to init object system.");

			return;
		}
		
		/* Should never be freed. */
		object_ref (g_char_cache[CHAR_CACHE_INDEX (i)]);
	}
}
