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
#include "boolobject.h"
#include "byteobject.h"
#include "longobject.h"

#define INT_CACHE_MIN -1000
#define INT_CACHE_MAX 10000
#define INT_CACHE_SIZE ((size_t)INT_CACHE_MAX-INT_CACHE_MIN+1)

/* Small ints should be cached. */
static object_t *g_int_cache[INT_CACHE_SIZE];

/* Object ops. */
static object_t *intobject_op_not (object_t *obj);
static object_t *intobject_op_neg (object_t *obj);
static object_t *intobject_op_add (object_t *obj1, object_t *obj2);
static object_t *intobject_op_sub (object_t *obj1, object_t *obj2);
static object_t *intobject_op_mul (object_t *obj1, object_t *obj2);
static object_t *intobject_op_mod (object_t *obj1, object_t *obj2);
static object_t *intobject_op_div (object_t *obj1, object_t *obj2);
static object_t *intobject_op_and (object_t *obj1, object_t *obj2);
static object_t *intobject_op_or (object_t *obj1, object_t *obj2);
static object_t *intobject_op_xor (object_t *obj1, object_t *obj2);
static object_t *intobject_op_land (object_t *obj1, object_t *obj2);
static object_t *intobject_op_lor (object_t *obj1, object_t *obj2);
static object_t *intobject_op_lshift (object_t *obj1, object_t *obj2);
static object_t *intobject_op_rshift (object_t *obj1, object_t *obj2);
static object_t *intobject_op_compare (object_t *obj1, object_t *obj2);

static object_opset_t g_object_ops =
{
	intobject_op_not, /* Logic Not. */
	NULL, /* Free. */
	NULL, /* Dump. */
	intobject_op_neg, /* Negative. */
	NULL, /* Call. */
	intobject_op_add, /* Addition. */
	intobject_op_sub, /* Substraction. */
	intobject_op_mul, /* Multiplication. */
	intobject_op_mod, /* Mod. */
	intobject_op_div, /* Division. */
	intobject_op_and, /* Bitwise and. */
	intobject_op_or, /* Bitwise or. */
	intobject_op_xor, /* Bitwise xor. */
	intobject_op_land, /* Logic and. */
	intobject_op_lor, /* Logic or. */
	intobject_op_lshift, /* Left shift. */
	intobject_op_rshift, /* Right shift. */
	intobject_op_compare, /* Comparation. */
	NULL  /* Index. */
};

/* Logic Not. */
static object_t *
intobject_op_not (object_t *obj)
{
	int val;

	val = intobject_get_value (obj);
	if (val == 0) {
		return boolobject_new (true, NULL);
	}

	return boolobject_new (false, NULL);
}

/* Negative. */
static object_t *
intobject_op_neg (object_t *obj)
{
	int val;

	val = intobject_get_value (obj);

	return intobject_new (-val, NULL);
}

static int
intobject_get_lower_type_value (object_t *obj)
{
	if (OBJECT_TYPE (obj2) == OBJECT_TYPE_BOOL) {
		return (int) boolobject_get_value (obj2);
	}
	else if (OBJECT_TYPE (obj2) == OBJECT_TYPE_BYTE) {
		return (int) byteobject_get_value (obj2);
	}
	else if (OBJECT_TYPE (obj2) == OBJECT_TYPE_INT) {
		return intobject_get_value (obj2);
	}

	/* Unreachable. */
	return 0;
}

statoc object_t *
intobject_arithmetical (object_t *obj1, object_t *obj2, char op)
{
	int val1;
	int val2;
	object_t *obj;

	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand");

		return NULL;
	}

	val1 = intobject_get_value (obj1);
	if (OBJECT_TYPE (obj2) == OBJECT_TYPE_LONG) {
		long long_val2;

		longval2 = longobject_get_value (obj2);
		switch (op) {
			case '+':
				obj = longobject_new (val1 + long_val2, NULL);
				break;
			case '-':
				obj = longobject_new (val1 - long_val2, NULL);
				break;
			case '*':
				obj = longobject_new (val1 * long_val2, NULL);
				break;
			case '/':
				obj = longobject_new (val1 / long_val2, NULL);
				break;
			case '%':
				obj = longobject_new (val1 % long_val2, NULL);
				break;
			case '&':
				obj = longobject_new (val1 & long_val2, NULL);
				break;
			case '|':
				obj = longobject_new (val1 | long_val2, NULL);
				break;
			case '^':
				obj = longobject_new (val1 ^ long_val2, NULL);
				break;
		}
	}
	else {
		val2 = intobject_get_lower_type_value (obj2);
		switch (op) {
			case '+':
				obj = intobject_new (val1 + val2, NULL);
				break;
			case '-':
				obj = intobject_new (val1 - val2, NULL);
				break;
			case '*':
				obj = intobject_new (val1 * val2, NULL);
				break;
			case '/':
				obj = intobject_new (val1 / val2, NULL);
				break;
			case '%':
				obj = intobject_new (val1 % val2, NULL);
				break;
			case '&':
				obj = intobject_new (val1 & val2, NULL);
				break;
			case '|':
				obj = intobject_new (val1 | val2, NULL);
				break;
			case '^':
				obj = intobject_new (val1 ^ val2, NULL);
				break;
		}
	}

	return obj;
}

int
intobject_right_operand_is_zero (object_t *t)
{
}

/* Addition. */
static object_t *
intobject_op_add (object_t *obj1, object_t *obj2)
{
	return intobject_arithmetical (obj1, obj2, '+');
}

/* Substraction. */
static object_t *
intobject_op_sub (object_t *obj1, object_t *obj2)
{
	return intobject_arithmetical (obj1, obj2, '-');
}

/* Multiplication. */
static object_t *
intobject_op_mul (object_t *obj1, object_t *obj2)
{
	return intobject_arithmetical (obj1, obj2, '*');
}

/* Mod. */
static object_t *
intobject_op_mod (object_t *obj1, object_t *obj2)
{
	
	return intobject_arithmetical (obj1, obj2, '%');
}

/* Division. */

/* Comparation. */
static object_t *
intobject_op_compare (object_t *obj1, object_t *obj2)
{
	/* Since there is only one 'true' or 'false' object, comparing
	 * address is enough. */
	if (obj1 == obj2) {
		return intobject_new (true, NULL);
	}
	
	return intobject_new (false, NULL);
}

object_t *
intobject_new (int val, void *udata)
{
	intobject_t *ob;

	if (val && g_true_object != NULL) {
		return g_true_object;
	}
	else if (!val && g_false_object != NULL) {
		return g_false_object;
	}
	ob = (intobject_t *) pool_alloc (sizeof (intobject_t));
	ob->head.ref = 0;
	ob->head.type = OBJECT_TYPE_BOOL;
	ob->head.ops = &g_object_ops;
	ob->head.udata = udata;
	ob->val = val;

	return (object_t *) ob;
}

int
intobject_get_value (object_t *obj)
{
	intobject_t *ob;

	ob = (intobject_t *) obj;

	return ob->val;
}

void
intobject_init ()
{
}
