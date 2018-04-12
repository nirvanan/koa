/*
 * object.c
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

#include <stdio.h>

#include "object.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"

/* Note that the 'null' object is shared everywhere. */
static object_t *g_null_object;

/* Object ops. */
static object_t *object_op_compare (object_t *obj1, object_t *obj2);

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
	NULL, /* Mod. */
	NULL, /* division. */
	NULL, /* Bitwise and. */
	NULL, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	NULL, /* Logic and. */
	NULL, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	object_op_compare, /* Comparation. */
	NULL  /* Index. */
};

/* Comparation. */
static object_t *
object_op_compare (object_t *obj1, object_t *obj2)
{
	/* Since there is only one 'null' object, comparing
	 * address is enough. */
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}
	
	return boolobject_new (false, NULL);
}

/* This object is known as 'null'. */
object_t *
object_new (void *udata)
{
	object_t *obj;

	if (g_null_object != NULL) {
		return g_null_object;
	}
	obj = (object_t *) pool_alloc (sizeof (object_t));
	obj->head.ref = 0;
	obj->head.type = OBJECT_TYPE_NONE;
	obj->head.ops = &g_object_ops;
	obj->head.udata = udata;

	return obj;
}

void
object_ref (object_t *obj)
{
	obj->head.ref++;
}

void
object_unref (object_t *obj)
{
	obj->head.ref--;
	if (obj->head.ref <= 0) {
		object_free (obj);
	}
}

void
object_free (object_t *obj)
{
	void_una_op_f free_fun;

	/* Refed objects should not be freed. */
	if (obj->head.ref > 0) {
		return;
	}

	/* Call object specifiled cleanup routine. */
	free_fun = ((object_opset_t *) obj->head.ops)->free;
	if (free_fun != NULL) {
		free_fun (obj);
	}

	pool_free (obj);
}

void
object_init ()
{
	/* The 'null' object should never be freed. */
	g_null_object = object_new (NULL);
	object_ref (g_null_object);
	
	/* Init some types of objects. */
	boolobject_init ();
}
