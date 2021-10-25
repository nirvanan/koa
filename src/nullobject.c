/*
 * nullobject.c
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

#include <stdlib.h>
#include <string.h>

#include "nullobject.h"
#include "pool.h"
#include "error.h"
#include "thread.h"
#include "boolobject.h"
#include "uint64object.h"
#include "strobject.h"

/* Note that the 'null' object is shared everywhere. */
static object_t *g_null_object;

/* Object ops. */
static void nullobject_op_print (object_t *obj);
static object_t *nullobject_op_dump (object_t *obj);
static object_t *nullobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *nullobject_op_hash (object_t *obj);
static object_t *nullobject_op_binary (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	NULL, /* Free. */
	nullobject_op_print, /* Print. */
	nullobject_op_dump, /* Dump. */
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
	nullobject_op_eq, /* Equality. */
	NULL, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	nullobject_op_hash, /* Hash. */
	nullobject_op_binary, /* Binary. */
	NULL /* Len. */
};

/* Equality. */
static object_t *
nullobject_op_eq (object_t *obj1, object_t *obj2)
{
	/* Since there is only one 'null' object, comparing
	 * address is enough. Moreover, comparing 'null'
	 * object to other objects will always be illegal
	 * except for equality comparing. */
	if (OBJECT_IS_NULL (obj1) && OBJECT_IS_NULL (obj2)) {
		return boolobject_new (true, NULL);
	}
	
	return boolobject_new (false, NULL);
}

/* Print. */
static void
nullobject_op_print (object_t *obj)
{
	printf ("null");
}

/* Dump. */
static object_t *
nullobject_op_dump (object_t *obj)
{
	return strobject_new ("<null>", strlen ("<null>"), 1, NULL);
}

/* Hash. */
static object_t *
nullobject_op_hash (object_t *obj)
{
	/* The digest is already computed. */
	return uint64object_new (OBJECT_DIGEST (obj), NULL);
}

/* Binary. */
static object_t *
nullobject_op_binary (object_t *obj)
{
	return strobject_new ("", 0, 1, NULL);
}

object_t *
nullobject_load_binary (FILE *f)
{
	return nullobject_new (NULL);
}

object_t *
nullobject_load_buf (const char **buf, size_t *len)
{
	return nullobject_new (NULL);
}

static uint64_t
nullobject_digest_fun (void *obj)
{
	return OBJECT_DIGEST ((object_t *) obj);
}

/* This object is known as 'null'. */
object_t *
nullobject_new (void *udata)
{
	nullobject_t *obj;

	if (g_null_object != NULL) {
		return g_null_object;
	}

	obj = (nullobject_t *) pool_alloc (sizeof (nullobject_t));
	if (obj == NULL) {
		fatal_error ("out of memory.");
	}

	OBJECT_NEW_INIT (obj, OBJECT_TYPE_NULL, udata);
	OBJECT_DIGEST_FUN (obj) = nullobject_digest_fun;

	return (object_t *) obj;
}

void
nullobject_init ()
{
	if (!thread_is_main_thread ()) {
		return;
	}

	/* The 'null' object should never be freed. */
	g_null_object = nullobject_new (NULL);
	if (g_null_object == NULL) {
		fatal_error ("failed to init object system.");

		return;
	}

	object_set_const (g_null_object);

	OBJECT_DIGEST (g_null_object) = 0;
}
