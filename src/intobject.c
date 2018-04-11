/*
 * intobject.c
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

#include "intobject.h"
#include "pool.h"
#include "error.h"

#define INT_CACHE_MIN -1000
#define INT_CACHE_MAX 10000
#define INT_CACHE_SIZE ((size_t)INT_CACHE_MAX-INT_CACHE_MIN+1)

/* Small ints should be cached. */
static object_t *g_int_cache[INT_CACHE_SIZE];

/* Object ops. */
static object_t *intobject_op_not (object_t *obj);
static object_t *intobject_op_compare (object_t *obj1, object_t *obj2);

static object_opset_t g_object_ops =
{
	intobject_op_not, /* Not. */
	NULL, /* Free. */
	NULL, /* Dump. */
	intobject_op_neg, /* Negative. */
	NULL, /* Call. */
	intobject_op_add, /* Addition. */
	intobject_op_sub, /* Substraction. */
	intobject_op_mul, /* Multiplication. */
	intobject_op_mod, /* Mod. */
	intobject_op_div, /* division. */
	intobject_op_and, /* Bitwise and. */
	intobject_, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	boolobject_op_compare, /* Comparation. */
	NULL  /* Index. */
};

/* Not. */
static object_t *
boolobject_op_not (object_t *obj)
{
	if (obj == g_false_object) {
		return g_true_object;
	}

	return g_false_object;
}

/* Comparation. */
static object_t *
boolobject_op_compare (object_t *obj1, object_t *obj2)
{
	/* Since there is only one 'true' or 'false' object, comparing
	 * address is enough. */
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}
	
	return boolobject_new (false, NULL);
}

/* This object is known as 'null'. */
object_t *
boolobject_new (bool val, void *udata)
{
	boolobject_t *ob;

	if (val && g_true_object != NULL) {
		return g_true_object;
	}
	else if (!val && g_false_object != NULL) {
		return g_false_object;
	}
	ob = (boolobject_t *) pool_alloc (sizeof (boolobject_t));
	ob->head.ref = 0;
	ob->head.type = OBJECT_TYPE_BOOL;
	ob->head.ops = &g_object_ops;
	ob->head.udata = udata;
	ob->val = val;

	return (object_t *) ob;
}

void
boolobject_init ()
{
	/* This two objects should never be freed. */
	g_true_object = boolobject_new (true, NULL);
	object_ref (g_true_object);

	g_false_object = boolobject_new (false, NULL);
	object_ref (g_false_object);
}
