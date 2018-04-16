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

#include <stdbool.h>

#include "strobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "charobject.h"
#include "intobject.h"

/* Object ops. */
static void strobject_op_free (object_t *obj);
static object_t *strobject_op_add (object_t *obj1, object_t *obj2);
static object_t *strobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *strobject_op_cmp (object_t *obj1, object_t *obj2);
static object_t *strobject_op_index (object_t *obj1, object_t *obj2);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	strobject_op_free, /* Free. */
	NULL, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	strobject_op_add, /* Addition. */
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
	strobject_op_eq, /* Equality. */
	strobject_op_cmp, /* Comparation. */
	strobject_op_index /* Index. */
};

/* Free. */
void
strobject_op_free (object_t *obj)
{
	str_free (strobject_get_value (obj));
}

/* Addition. */
static object_t *
strobject_op_add (object_t *obj1, object_t *obj2)
{
	object_t *right;
	str_t *s1;
	str_t *s2;
	str_t *cated;

	right = obj2;
	/* Convert numberical to string. */
	if (NUMBERICAL_TYPE (obj2)) {
		right = object_dump (obj2);
	}

	s1 = strobject_get_value (obj1);
	s2 = strobject_get_value (obj2);
	cated = str_concat (s1, s2);

	if (right != obj2) {
		object_free (right);
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
	s2 = strobject_get_value (obj2);

	return boolobject_new (str_cmp (s1, s2) == 0, NULL);
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

object_t *
strobject_new (const char *val, void *udata)
{
	strobject_t *obj;

	obj = (strobject_t *) pool_alloc (sizeof (strobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	obj->head.ref = 0;
	obj->head.type = OBJECT_TYPE_STR;
	obj->head.ops = &g_object_ops;
	obj->head.udata = udata;
	obj->val = str_new (val);
	if (obj->val == NULL) {
		pool_free ((void *) obj);

		return NULL;
	}


	return (object_t *) obj;
}

object_t *
strobject_str_new (str_t *val, void *udata)
{
	strobject_t *obj;

	obj = (strobject_t *) pool_alloc (sizeof (strobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	obj->head.ref = 0;
	obj->head.type = OBJECT_TYPE_STR;
	obj->head.ops = &g_object_ops;
	obj->head.udata = udata;
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

void
strobject_init ()
{
}
