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

#include "nullobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "longobject.h"

/* Note that the 'null' object is shared everywhere. */
static object_t *g_null_object;

/* Object ops. */
static object_t *nullobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *nullobject_op_hash (object_t *obj);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	NULL, /* Free. */
	NULL, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	NULL, /* Addition. */
	NULL, /* Substraction. */
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
	nullobject_op_eq, /* Equality. */
	NULL, /* Comparation. */
	NULL, /* Index. */
	NULL, /* Inplace index. */
	nullobject_op_hash /* Hash. */
};

/* Equality. */
static object_t *
nullobject_op_eq (object_t *obj1, object_t *obj2)
{
	/* Since there is only one 'null' object, comparing
	 * address is enough. Moreover, comparing 'null'
	 * object to other objects will always be illegal
	 * except for equality comparing. */
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}
	
	return boolobject_new (false, NULL);
}

/* Hash. */
static object_t *
nullobject_op_hash(object_t *obj)
{
	/* The digest is already computed. */
	return longobject_new (OBJECT_DIGEST (obj), NULL);
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
		error ("out of memory.");

		return NULL;
	}

	obj->head.ref = 0;
	obj->head.type = OBJECT_TYPE_NONE;
	obj->head.ops = &g_object_ops;
	obj->head.udata = udata;

	return (object_t *) obj;
}

void
nullobject_init ()
{
	/* The 'null' object should never be freed. */
	g_null_object = nullobject_new (NULL);
	if (g_null_object == NULL) {
		fatal_error ("failed to init object system.");

		return;
	}

	object_ref (g_null_object);
	OBJECT_DIGEST (g_null_object) = (uint64_t) random ();
}
