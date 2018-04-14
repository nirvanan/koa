/*
 * doubleobject.c
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
 * adouble with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "doubleobject.h"
#include "pool.h"
#include "error.h"
#include "boolobject.h"
#include "intobject.h"

/* Object ops. */
static object_t *doubleobject_op_not (object_t *obj);
static object_t *doubleobject_op_neg (object_t *obj);
static object_t *doubleobject_op_add (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_sub (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_mul (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_div (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_land (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_eq (object_t *obj1, object_t *obj2);
static object_t *doubleobject_op_cmp (object_t *obj1, object_t *obj2);

static object_opset_t g_object_ops =
{
	doubleobject_op_not, /* Logic Not. */
	NULL, /* Free. */
	NULL, /* Dump. */
	doubleobject_op_neg, /* Negative. */
	NULL, /* Call. */
	doubleobject_op_add, /* Addition. */
	doubleobject_op_sub, /* Substraction. */
	doubleobject_op_mul, /* Multiplication. */
	doubleobject_op_div, /* Division. */
	NULL, /* Mod. */
	NULL, /* Bitwise and. */
	NULL, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	doubleobject_op_land, /* Logic and. */
	doubleobject_op_lor, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	doubleobject_op_eq, /* Equality. */
	doubleobject_op_cmp, /* Comparation. */
	NULL  /* Index. */
};

/* Logic Not. */
static object_t *
doubleobject_op_not (object_t *obj)
{
	double val;

	val = doubleobject_get_value (obj);
	if (val == 0) {
		return boolobject_new (true, NULL);
	}

	return boolobject_new (false, NULL);
}

/* Negative. */
static object_t *
doubleobject_op_neg (object_t *obj)
{
	double val;

	val = doubleobject_get_value (obj);

	return doubleobject_new (-val, NULL);
}

/* Addition. */
static object_t *
doubleobject_op_add (object_t *obj1, object_t *obj2)
{
	double val1;
	double val2;

	val1 = doubleobject_get_value (obj1);
	val2 = doubleobject_get_value (obj2);

	return doubleobject_new (val1 + val2, NULL);
}

/* Substraction. */
static object_t *
doubleobject_op_sub (object_t *obj1, object_t *obj2)
{
	double val1;
	double val2;

	val1 = doubleobject_get_value (obj1);
	val2 = doubleobject_get_value (obj2);

	return intobject_new (val1 - val2, NULL);
}

/* Multiplication. */
static object_t *
doubleobject_op_mul (object_t *obj1, object_t *obj2)
{
	double val1;
	double val2;

	val1 = doubleobject_get_value (obj1);
	val2 = doubleobject_get_value (obj2);

	return doubleobject_new (val1 * val2, NULL);
}


/* Division. */
static object_t *
doubleobject_op_div (object_t *obj1, object_t *obj2)
{
	double val1;
	double val2;

	val1 = doubleobject_get_value (obj1);
	val2 = doubleobject_get_value (obj2);

	return doubleobject_new (val1 / val2, NULL);
}

/* Logic and. */
static object_t *
doubleobject_op_land (object_t *obj1, object_t *obj2)
{
	double val1;

	val1 = doubleobject_get_value (obj1);

	return boolobject_new (val1 && NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Logic or. */
static object_t *
doubleobject_op_lor (object_t *obj1, object_t *obj2)
{
	double val1;

	val1 = doubleobject_get_value (obj1);

	return boolobject_new (val1 || NUMBERICAL_GET_VALUE (obj2), NULL);
}

/* Equality. */
static object_t *
doubleobject_op_eq (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return boolobject_new (true, NULL);
	}

	if (NUMBERICAL_TYPE (obj2)) {
		double val1;
	
		val1 = doubleobject_get_value (obj1);

		return boolobject_new (val1 == NUMBERICAL_GET_VALUE (obj2), NULL);
	}
	
	return boolobject_new (false, NULL);
}

/* Comparation. */
static object_t *
doubleobject_op_cmp (object_t *obj1, object_t *obj2)
{
	return intobject_new (object_numberical_compare (obj1, obj2), NULL);
}

object_t *
doubleobject_new (double val, void *udata)
{
	doubleobject_t *obj;

	obj = (doubleobject_t *) pool_alloc (sizeof (doubleobject_t));
	obj->head.ref = 0;
	obj->head.type = OBJECT_TYPE_DOUBLE;
	obj->head.ops = &g_object_ops;
	obj->head.udata = udata;
	obj->val = val;

	return (object_t *) obj;
}

double
doubleobject_get_value (object_t *obj)
{
	doubleobject_t *ob;

	ob = (doubleobject_t *) obj;

	return ob->val;
}

