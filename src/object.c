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
#include <string.h>
#include <math.h>

#include "object.h"
#include "pool.h"
#include "error.h"
#include "nullobject.h"
#include "boolobject.h"
#include "charobject.h"
#include "intobject.h"
#include "longobject.h"
#include "int8object.h"
#include "uint8object.h"
#include "int16object.h"
#include "uint16object.h"
#include "int32object.h"
#include "uint32object.h"
#include "int64object.h"
#include "uint64object.h"
#include "floatobject.h"
#include "doubleobject.h"
#include "strobject.h"
#include "vecobject.h"
#include "dictobject.h"
#include "funcobject.h"
#include "modobject.h"

#define TYPE_NAME(x) (g_type_name[(x)->head.type])

#define FLOATING_INT_TO_HASH_NEG -271828
#define FLOATING_INT_TO_HASH_POS 314159

#define MURMUR3_CONST_A 33
#define MURMUR3_CONST_B 0xff51afd7ed558ccd
#define MURMUR3_CONST_C 0xc4ceb9fe1a85ec53

static const char *g_type_name[] =
{
	"void",
	"null",
	"bool",
	"char",
	"int",
	"long",
	"float",
	"double",
	"str",
	"vec",
	"dict",
	"func",
	"mod",
	"frame"
};

/* Dummy object, used to represent void values. */
static object_opset_t g_dummy_ops =
{
	NULL
};

static object_t g_dummy_object = 
{
	{
		1,
		OBJECT_TYPE_VOID,
		0,
		NULL,
		&g_dummy_ops,
		NULL
	},
};

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
object_unref_without_free (object_t *obj)
{
	obj->head.ref--;
}

integer_value_t
object_get_integer (object_t *obj)
{
	switch (OBJECT_TYPE (obj)) {
		case OBJECT_TYPE_BOOL:
			return (integer_value_t) boolobject_get_value (obj);
		case OBJECT_TYPE_CHAR:
			return (integer_value_t) charobject_get_value (obj);
		case OBJECT_TYPE_INT:
			return (integer_value_t) intobject_get_value (obj);
		case OBJECT_TYPE_LONG:
			return (integer_value_t) longobject_get_value (obj);
		case OBJECT_TYPE_INT8:
			return (integer_value_t) int8object_get_value (obj);
		case OBJECT_TYPE_UINT8:
			return (integer_value_t) uint8object_get_value (obj);
		case OBJECT_TYPE_INT16:
			return (integer_value_t) int16object_get_value (obj);
		case OBJECT_TYPE_UINT16:
			return (integer_value_t) uint16object_get_value (obj);
		case OBJECT_TYPE_INT32:
			return (integer_value_t) int32object_get_value (obj);
		case OBJECT_TYPE_UINT32:
			return (integer_value_t) uint32object_get_value (obj);
		case OBJECT_TYPE_INT64:
			return (integer_value_t) int64object_get_value (obj);
		case OBJECT_TYPE_UINT64:
			return (integer_value_t) uint64object_get_value (obj);
		/* Never reachable. */
		default:
			error ("try to get integer value from %s.", TYPE_NAME (obj));
			return 0;
	}

	return 0;
}

floating_value_t
object_get_floating (object_t *obj)
{
	switch (OBJECT_TYPE (obj)) {
		case OBJECT_TYPE_FLOAT:
			return (floating_value_t) floatobject_get_value (obj);
		case OBJECT_TYPE_DOUBLE:
			return (floating_value_t) doubleobject_get_value (obj);
		/* Never reachable. */
		default:
			error ("try to get floating value from %s.", TYPE_NAME (obj));
			return 0;
	}

	return 0.0;
}

int
object_is_zero (object_t *obj)
{
	return (int) (NUMBERICAL_GET_VALUE (obj) == 0);
}

object_t *
object_cast (object_t *obj, object_type_t type)
{
	if (INTEGER_TYPE (obj)) {
		integer_value_t val;

		val = object_get_integer (obj);
		switch (type) {
			case OBJECT_TYPE_BOOL:
				return boolobject_new ((bool) val, NULL);
			case OBJECT_TYPE_CHAR:
				return charobject_new ((char) val, NULL);
			case OBJECT_TYPE_INT:
				return intobject_new ((int) val, NULL);
			case OBJECT_TYPE_LONG:
				return longobject_new ((long) val, NULL);
			case OBJECT_TYPE_INT8:
				return int8object_new ((int8_t) val, NULL);
			case OBJECT_TYPE_UINT8:
				return uint8object_new ((uint8_t) val, NULL);
			case OBJECT_TYPE_INT16:
				return int16object_new ((int16_t) val, NULL);
			case OBJECT_TYPE_UINT16:
				return uint16object_new ((uint16_t) val, NULL);
			case OBJECT_TYPE_INT32:
				return int32object_new ((int32_t) val, NULL);
			case OBJECT_TYPE_UINT32:
				return uint32object_new ((uint32_t) val, NULL);
			case OBJECT_TYPE_INT64:
				return int64object_new ((int64_t) val, NULL);
			case OBJECT_TYPE_UINT64:
				return uint64object_new ((uint64_t) val, NULL);
			case OBJECT_TYPE_FLOAT:
				return floatobject_new ((float) val, NULL);
			case OBJECT_TYPE_DOUBLE:
				return doubleobject_new ((double) val, NULL);
			default:
				error ("try to cast numberical object to %s.", TYPE_NAME (obj));

				return NULL;
		}
	}
	else if (FLOATING_TYPE (obj)) {
		floating_value_t val;

		val = object_get_floating (obj);
		switch (type) {
			case OBJECT_TYPE_BOOL:
				return boolobject_new ((bool) val, NULL);
			case OBJECT_TYPE_CHAR:
				return charobject_new ((char) val, NULL);
			case OBJECT_TYPE_INT:
				return intobject_new ((int) val, NULL);
			case OBJECT_TYPE_LONG:
				return longobject_new ((long) val, NULL);
			case OBJECT_TYPE_INT8:
				return int8object_new ((int8_t) val, NULL);
			case OBJECT_TYPE_UINT8:
				return uint8object_new ((uint8_t) val, NULL);
			case OBJECT_TYPE_INT16:
				return int16object_new ((int16_t) val, NULL);
			case OBJECT_TYPE_UINT16:
				return uint16object_new ((uint16_t) val, NULL);
			case OBJECT_TYPE_INT32:
				return int32object_new ((int32_t) val, NULL);
			case OBJECT_TYPE_UINT32:
				return uint32object_new ((uint32_t) val, NULL);
			case OBJECT_TYPE_INT64:
				return int64object_new ((int64_t) val, NULL);
			case OBJECT_TYPE_UINT64:
				return uint64object_new ((uint64_t) val, NULL);
			case OBJECT_TYPE_FLOAT:
				return floatobject_new ((float) val, NULL);
			case OBJECT_TYPE_DOUBLE:
				return doubleobject_new ((double) val, NULL);
			default:
				error ("try to cast numberical object to %s.", TYPE_NAME (obj));

				return NULL;
		}
	}

	error ("only numberical object can be casted.");

	return NULL;
}

int
object_numberical_compare (object_t *obj1, object_t *obj2)
{
	if (obj1 == obj2) {
		return 0;
	}

	return NUMBERICAL_GET_VALUE (obj1) > NUMBERICAL_GET_VALUE(obj2)? 1:
		NUMBERICAL_GET_VALUE (obj1) == NUMBERICAL_GET_VALUE (obj2)? 0: -1;
}

static void
object_una_cleanup (object_t *obj, object_t *temp)
{
	if (temp != obj) {
		object_free (temp);
	}
}

static void
object_bin_cleanup (object_t *obj1, object_t *t1, object_t *obj2, object_t *t2)
{
	if (t1 != obj1) {
		object_free (t1);
	}
	if (t2 != obj2) {
		object_free (t2);
	}
}

object_t *
object_logic_not (object_t *obj)
{
	una_op_f lnot_fun;

	if (!NUMBERICAL_TYPE (obj)) {
		error ("invalid operand type %s for '!'.", TYPE_NAME (obj));

		return NULL;
	}
	lnot_fun = (OBJECT_OPSET (obj))->lnot;

	return lnot_fun (obj);
}

void
object_free (object_t *obj)
{
	void_una_op_f free_fun;

	/* Refed objects should not be freed. */
	if (OBJECT_REF (obj) > 0) {
		return;
	}

	/* Call object specifiled cleanup routine. */
	free_fun = (OBJECT_OPSET (obj))->free;
	if (free_fun != NULL) {
		free_fun (obj);
	}

	pool_free ((void *) obj);
}

object_t *
object_dump (object_t *obj)
{
	una_op_f dump_fun;

	dump_fun = (OBJECT_OPSET (obj))->dump;
	if (dump_fun == NULL) {
		if (OBJECT_IS_DUMMY (obj)) {
			return strobject_new ("<dummy>", strlen ("<dummy>"), 1, NULL);
		}

		error ("%s has no dump routine.", TYPE_NAME (obj));

		return NULL;
	}

	return dump_fun (obj);
}

object_t *
object_neg (object_t *obj)
{
	object_t *temp;
	object_t *res;
	una_op_f neg_fun;

	if (!NUMBERICAL_TYPE (obj)) {
		error ("invalid operand type %s for '-'.", TYPE_NAME (obj));

		return NULL;
	}

	temp = obj;
	if (OBJECT_TYPE (obj) < OBJECT_TYPE_INT) {
		temp = object_cast (obj, OBJECT_TYPE_INT);
		if (temp == NULL) {
			return NULL;
		}
	}

	neg_fun = (OBJECT_OPSET (obj))->neg;
	res = neg_fun (temp);
	object_una_cleanup (obj, temp);

	return res;
}

object_t *
object_add (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f add_fun;

	if (OBJECT_TYPE (obj1) == OBJECT_TYPE_STR) {
		add_fun = (OBJECT_OPSET (obj1))->add;
		
		return add_fun (obj1, obj2);
	}
	else if (OBJECT_TYPE (obj1) == OBJECT_TYPE_VEC) {
		if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
			error ("invalid right operand type %s for '+'.", TYPE_NAME (obj2));
		
			return NULL;
		}

		add_fun = (OBJECT_OPSET (obj1))->add;
		
		return add_fun (obj1, obj2);
	}

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type %s for '+'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type %s for '+'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	left = obj1;
	right = obj2;
	if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
		left = OBJECT_BIGGER (obj1, obj2);
		right = OBJECT_BIGGER (obj2, obj1);
	}
	else if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = object_cast (obj1, OBJECT_TYPE_INT);
		right = object_cast (obj2, OBJECT_TYPE_INT);
	}

	if (left == NULL || right == NULL) {
		if (left != NULL) {
			object_free (left);
		}
		if (right != NULL) {
			object_free (right);
		}

		return NULL;
	}

	add_fun = (OBJECT_OPSET (left))->add;
	res = add_fun (left, right);
	object_bin_cleanup (obj1, left, obj2, right);
	
	return res;
}

object_t *
object_sub (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f sub_fun;

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type %s for '-'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type %s for '-'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	left = obj1;
	right = obj2;
	if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
		left = OBJECT_BIGGER (obj1, obj2);
		right = OBJECT_BIGGER (obj2, obj1);
	}
	else if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = object_cast (obj1, OBJECT_TYPE_INT);
		right = object_cast (obj2, OBJECT_TYPE_INT);
	}

	if (left == NULL || right == NULL) {
		if (left != NULL) {
			object_free (left);
		}
		if (right != NULL) {
			object_free (right);
		}

		return NULL;
	}

	sub_fun = (OBJECT_OPSET (left))->sub;
	res = sub_fun (left, right);
	object_bin_cleanup (obj1, left, obj2, right);
	
	return res;
}

object_t *
object_mul (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f mul_fun;

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type %s for '*'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type %s for '*'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	left = obj1;
	right = obj2;
	if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
		left = OBJECT_BIGGER (obj1, obj2);
		right = OBJECT_BIGGER (obj2, obj1);
	}
	else if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = object_cast (obj1, OBJECT_TYPE_INT);
		right = object_cast (obj2, OBJECT_TYPE_INT);
	}

	if (left == NULL || right == NULL) {
		if (left != NULL) {
			object_free (left);
		}
		if (right != NULL) {
			object_free (right);
		}

		return NULL;
	}

	mul_fun = (OBJECT_OPSET (left))->mul;
	res = mul_fun (left, right);
	object_bin_cleanup (obj1, left, obj2, right);
	
	return res;
}

object_t *
object_div (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f div_fun;

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type %s for '/'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type %s for '/'.", TYPE_NAME (obj2));
		
		return NULL;
	}
	if (object_is_zero (obj2)) {
		error ("division by zero.");
		
		return NULL;
	}

	left = obj1;
	right = obj2;
	if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
		left = OBJECT_BIGGER (obj1, obj2);
		right = OBJECT_BIGGER (obj2, obj1);
	}
	else if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = object_cast (obj1, OBJECT_TYPE_INT);
		right = object_cast (obj2, OBJECT_TYPE_INT);
	}

	if (left == NULL || right == NULL) {
		if (left != NULL) {
			object_free (left);
		}
		if (right != NULL) {
			object_free (right);
		}

		return NULL;
	}

	div_fun = (OBJECT_OPSET (left))->div;
	res = div_fun (left, right);
	object_bin_cleanup (obj1, left, obj2, right);
	
	return res;
}

object_t *
object_mod (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f mod_fun;

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type %s for '%'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type %s for '%'.", TYPE_NAME (obj2));
		
		return NULL;
	}
	if (object_is_zero (obj2)) {
		error ("division by zero.");
		
		return NULL;
	}

	left = obj1;
	right = obj2;
	if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
		left = OBJECT_BIGGER (obj1, obj2);
		right = OBJECT_BIGGER (obj2, obj1);
	}
	else if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = object_cast (obj1, OBJECT_TYPE_INT);
		right = object_cast (obj2, OBJECT_TYPE_INT);
	}

	if (left == NULL || right == NULL) {
		if (left != NULL) {
			object_free (left);
		}
		if (right != NULL) {
			object_free (right);
		}

		return NULL;
	}

	mod_fun = (OBJECT_OPSET (left))->mod;
	res = mod_fun (left, right);
	object_bin_cleanup (obj1, left, obj2, right);
	
	return res;
}

object_t *
object_bit_not (object_t *obj)
{
	object_t *temp;
	object_t *res;
	una_op_f not_fun;

	if (!INTEGER_TYPE (obj)) {
		error ("invalid operand type %s for '~'.", TYPE_NAME (obj));

		return NULL;
	}

	temp = obj;
	if (OBJECT_TYPE (obj) < OBJECT_TYPE_INT) {
		temp = object_cast (obj, OBJECT_TYPE_INT);
		if (temp == NULL) {
			return NULL;
		}
	}

	not_fun = (OBJECT_OPSET (obj))->not;
	res = not_fun (temp);
	object_una_cleanup (obj, temp);

	return res;
}

object_t *
object_bit_and (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f and_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type %s for '&'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type %s for '&'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	left = obj1;
	right = obj2;
	if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
		left = OBJECT_BIGGER (obj1, obj2);
		right = OBJECT_BIGGER (obj2, obj1);
	}
	else if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = object_cast (obj1, OBJECT_TYPE_INT);
		right = object_cast (obj2, OBJECT_TYPE_INT);
	}

	if (left == NULL || right == NULL) {
		if (left != NULL) {
			object_free (left);
		}
		if (right != NULL) {
			object_free (right);
		}

		return NULL;
	}

	and_fun = (OBJECT_OPSET (left))->and;
	res = and_fun (left, right);
	object_bin_cleanup (obj1, left, obj2, right);
	
	return res;
}

object_t *
object_bit_or (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f or_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type %s for '|'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type %s for '|'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	left = obj1;
	right = obj2;
	if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
		left = OBJECT_BIGGER (obj1, obj2);
		right = OBJECT_BIGGER (obj2, obj1);
	}
	else if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = object_cast (obj1, OBJECT_TYPE_INT);
		right = object_cast (obj2, OBJECT_TYPE_INT);
	}

	if (left == NULL || right == NULL) {
		if (left != NULL) {
			object_free (left);
		}
		if (right != NULL) {
			object_free (right);
		}

		return NULL;
	}

	or_fun = (OBJECT_OPSET (left))->or;
	res = or_fun (left, right);
	object_bin_cleanup (obj1, left, obj2, right);
	
	return res;
}

object_t *
object_bit_xor (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f xor_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type %s for '^'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type %s for '^'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	left = obj1;
	right = obj2;
	if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
		left = OBJECT_BIGGER (obj1, obj2);
		right = OBJECT_BIGGER (obj2, obj1);
	}
	else if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = object_cast (obj1, OBJECT_TYPE_INT);
		right = object_cast (obj2, OBJECT_TYPE_INT);
	}

	if (left == NULL || right == NULL) {
		if (left != NULL) {
			object_free (left);
		}
		if (right != NULL) {
			object_free (right);
		}

		return NULL;
	}

	xor_fun = (OBJECT_OPSET (left))->xor;
	res = xor_fun (left, right);
	object_bin_cleanup (obj1, left, obj2, right);
	
	return res;
}

object_t *
object_logic_and (object_t *obj1, object_t *obj2)
{
	bin_op_f land_fun;

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type %s for '&&'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type %s for '&&'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	land_fun = (OBJECT_OPSET (obj1))->land;

	return land_fun (obj1, obj2);
}

object_t *
object_logic_or (object_t *obj1, object_t *obj2)
{
	bin_op_f lor_fun;

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type %s for '||'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type %s for '||'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	lor_fun = (OBJECT_OPSET (obj1))->lor;

	return lor_fun (obj1, obj2);
}

object_t *
object_left_shift (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *res;
	bin_op_f lshift_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type %s for '<<'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type %s for '<<'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	left = obj1;
	if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = intobject_new ((int) object_get_integer (obj1), NULL);
		if (left == NULL) {
			return NULL;
		}
	}

	lshift_fun = (OBJECT_OPSET (left))->lshift;
	res = lshift_fun (left, obj2);
	object_una_cleanup (obj1, left);

	return res;
}

object_t *
object_right_shift (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *res;
	bin_op_f rshift_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type %s for '>>'.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type %s for '>>'.", TYPE_NAME (obj2));
		
		return NULL;
	}

	left = obj1;
	if (OBJECT_TYPE (obj1) < OBJECT_TYPE_INT) {
		left = intobject_new ((int) object_get_integer (obj1), NULL);
		if (left == NULL) {
			return NULL;
		}
	}

	rshift_fun = (OBJECT_OPSET (left))->rshift;
	res = rshift_fun (left, obj2);
	object_una_cleanup (obj1, left);

	return res;
}

object_t *
object_equal (object_t *obj1, object_t *obj2)
{
	bin_op_f eq_fun;

	if (OBJECT_TYPE (obj1) == OBJECT_TYPE_STR
		&& OBJECT_TYPE (obj2) == OBJECT_TYPE_STR) {
		eq_fun = (OBJECT_OPSET (obj1))->eq;

		return eq_fun (obj1, obj2);
	}

	eq_fun = (OBJECT_OPSET (obj1))->eq;
	if (eq_fun == NULL) {
		/* Actually all types have equality routine. */
		error ("no equality routine for left operand %s.", TYPE_NAME (obj1));

		return NULL;
	}

	return eq_fun (obj1, obj2);
}

object_t *
object_compare (object_t *obj1, object_t *obj2)
{
	bin_op_f cmp_fun;

	if (OBJECT_TYPE (obj1) == OBJECT_TYPE_STR
		&& OBJECT_TYPE (obj2) == OBJECT_TYPE_STR) {
		cmp_fun = (OBJECT_OPSET (obj1))->cmp;

		return cmp_fun (obj1, obj2);
	}

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type %s for comparation.", TYPE_NAME (obj1));

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type %s for comparation.", TYPE_NAME (obj2));
		
		return NULL;
	}

	cmp_fun = (OBJECT_OPSET (obj1))->cmp;

	return cmp_fun (obj1, obj2);
}

object_t *
object_index (object_t *obj1, object_t *obj2)
{
	bin_op_f index_fun;

	index_fun = (OBJECT_OPSET (obj1))->index;
	if (index_fun == NULL) {
		error ("left operand %s has no index routine.", TYPE_NAME (obj1));
		
		return NULL;
	}

	return index_fun (obj1, obj2);
}

object_t *
object_ipindex (object_t *obj1, object_t *obj2, object_t *obj3)
{
	ter_op_f ipindex_fun;

	ipindex_fun = (OBJECT_OPSET (obj1))->ipindex;
	if (ipindex_fun == NULL) {
		error ("left operand %s has no inplace index routine.", TYPE_NAME (obj1));
		
		return NULL;
	}

	return ipindex_fun (obj1, obj2, obj3);
}

/* 64-bit Murmurhash3 finalizer. */
uint64_t
object_integer_hash (integer_value_t val)
{
	uint64_t h;

	h = (uint64_t) val;
	h ^= h >> MURMUR3_CONST_A;
	h *= MURMUR3_CONST_B;
	h ^= h >> MURMUR3_CONST_A;
	h *= MURMUR3_CONST_C;
	h ^= h >> MURMUR3_CONST_A;

	return h;
}

uint64_t
object_floating_hash (floating_value_t val)
{
	floating_value_t frac_part;
	floating_value_t int_part;
	integer_value_t int_to_hash;

	int_to_hash = 0;
	if (!FLOATING_FINITE (val)) {
		if (FLOATING_IS_INFINITY (val)) {
			int_to_hash = val < 0?
				FLOATING_INT_TO_HASH_NEG: FLOATING_INT_TO_HASH_POS;
		}
		else
			int_to_hash = 0;
	}
	else {
		frac_part = modf (val, &int_part);
		if (frac_part == 0.0) {
			/* It is ok that int_part is bigger than the max
			 * of integer_value_t. Since all we need is to
			 * guarantee that an integer_value_t has the same
			 * hash digest as the equivalent floating value. */
			int_to_hash = (integer_value_t)
				(int_part >= 0.0? int_part + 0.1: int_part - 0.1);
		}
		else {
			void *val_p;

			/* It is safe to convert a double value (64 bits)
			 * to a long (<= 64 bits) bitwise. */
			val_p = (void *) &val;
			int_to_hash = *((integer_value_t *) val_p);
		}
	}

	return object_integer_hash (int_to_hash);	
}

uint64_t
object_address_hash (void *obj)
{
	return object_integer_hash ((integer_value_t) obj);
}

/* We use a long object to represent hash digest of objects.
 * Although it may be down casted when a long is presented
 * as 32 bits integer, it will still works fine if our 64-bit
 * hash algorithm is solid enough. */
object_t *
object_hash (object_t *obj)
{
	una_op_f hash_fun;

	hash_fun = (OBJECT_OPSET (obj))->hash;
	/* At this stage, this is impossible, =_=. */
	if (hash_fun == NULL) {
		error ("type %s has no hash routine.", TYPE_NAME (obj));
		
		return NULL;
	}

	return hash_fun (obj);
}

uint64_t
object_digest (object_t *obj)
{
	digest_f digest_fun;

	if (OBJECT_DIGEST (obj)) {
		return OBJECT_DIGEST (obj);
	}

	digest_fun = obj->head.digest_fun;
	if (digest_fun == NULL) {
		error ("type %s has no hash routine.", TYPE_NAME (obj));
		
		return 0;
	}

	return digest_fun (obj);
}

/* At this stage, this operation is only used by dumping code.
 * TODO: Check loop refererence. */
object_t *
object_binary (object_t *obj)
{
	una_op_f binary_fun;
	object_t *head_obj;
	object_t *content_obj;
	object_t *res;

	head_obj = strobject_new (BINARY (OBJECT_TYPE (obj)),
							  sizeof (object_type_t), 1, NULL);
	if (head_obj == NULL) {
		return NULL;
	}

	binary_fun = (OBJECT_OPSET (obj))->binary;
	if (binary_fun == NULL) {
		if (OBJECT_TYPE (obj) == OBJECT_TYPE_VOID) {
			return head_obj;
		}

		object_free (head_obj);

		error ("type %s has no binary routine.", TYPE_NAME (obj));
		
		return NULL;
	}
	
	content_obj = binary_fun (obj);
	if (content_obj == NULL) {
		object_free (head_obj);

		return NULL;
	}

	res = object_add (head_obj, content_obj);
	object_free (head_obj);
	object_free (content_obj);

	return res;
}

object_t *
object_get_default (object_type_t type)
{
	switch (type) {
		case OBJECT_TYPE_VOID:
			return &g_dummy_object;
		case OBJECT_TYPE_NULL:
			return nullobject_new (NULL);
		case OBJECT_TYPE_BOOL:
			return boolobject_new (false, NULL);
		case OBJECT_TYPE_CHAR:
			return charobject_new ((char) 0, NULL);
		case OBJECT_TYPE_INT:
			return intobject_new (0, NULL);
		case OBJECT_TYPE_LONG:
			return longobject_new ((long) 0, NULL);
		case OBJECT_TYPE_INT8:
			return int8object_new ((int8_t) 0, NULL);
		case OBJECT_TYPE_UINT8:
			return uint8object_new ((uint8_t) 0, NULL);
		case OBJECT_TYPE_INT16:
			return int16object_new ((int16_t) 0, NULL);
		case OBJECT_TYPE_UINT16:
			return uint16object_new ((uint16_t) 0, NULL);
		case OBJECT_TYPE_INT32:
			return int32object_new ((int32_t) 0, NULL);
		case OBJECT_TYPE_UINT32:
			return uint32object_new ((uint32_t) 0, NULL);
		case OBJECT_TYPE_INT64:
			return int64object_new ((int64_t) 0, NULL);
		case OBJECT_TYPE_UINT64:
			return uint64object_new ((uint64_t) 0, NULL);
		case OBJECT_TYPE_FLOAT:
			return floatobject_new ((float) 0.0, NULL);
		case OBJECT_TYPE_DOUBLE:
			return doubleobject_new ((double) 0.0, NULL);
		case OBJECT_TYPE_STR:
			return strobject_new ("", 0, 0, NULL);
		case OBJECT_TYPE_VEC:
			return vecobject_new ((size_t) 0, NULL);
		case OBJECT_TYPE_DICT:
			return dictobject_new (NULL);
		case OBJECT_TYPE_FUNC:
			return funcobject_new (NULL);
		case OBJECT_TYPE_MOD:
			return modobject_new (NULL);
		default:
			break;
	}

	return NULL;
}

object_t *
object_load_binary (FILE *f)
{
	object_type_t type;

	if (fread (&type, sizeof (object_type_t), 1, f) != 1) {
		error ("failed to load object type while loading binary.");

		return NULL;
	}

	switch (type) {
		case OBJECT_TYPE_VOID:
			return &g_dummy_object;
		case OBJECT_TYPE_NULL:
			return nullobject_load_binary (f);
		case OBJECT_TYPE_BOOL:
			return boolobject_load_binary (f);
		case OBJECT_TYPE_CHAR:
			return charobject_load_binary (f);
		case OBJECT_TYPE_INT:
			return intobject_load_binary (f);
		case OBJECT_TYPE_LONG:
			return longobject_load_binary (f);
		case OBJECT_TYPE_INT8:
			return int8object_load_binary (f);
		case OBJECT_TYPE_UINT8:
			return uint8object_load_binary (f);
		case OBJECT_TYPE_INT16:
			return int16object_load_binary (f);
		case OBJECT_TYPE_UINT16:
			return uint16object_load_binary (f);
		case OBJECT_TYPE_INT32:
			return int32object_load_binary (f);
		case OBJECT_TYPE_UINT32:
			return uint32object_load_binary (f);
		case OBJECT_TYPE_INT64:
			return int64object_load_binary (f);
		case OBJECT_TYPE_UINT64:
			return uint64object_load_binary (f);
		case OBJECT_TYPE_FLOAT:
			return floatobject_load_binary (f);
		case OBJECT_TYPE_DOUBLE:
			return doubleobject_load_binary (f);
		case OBJECT_TYPE_STR:
			return strobject_load_binary (f);
		case OBJECT_TYPE_VEC:
			return vecobject_load_binary (f);
		case OBJECT_TYPE_DICT:
			return dictobject_load_binary (f);
		case OBJECT_TYPE_FUNC:
			return funcobject_load_binary (f);
		case OBJECT_TYPE_MOD:
			return modobject_load_binary (f);
		default:
			break;
	}

	return NULL;
}

void
object_print (object_t *obj)
{
	object_t *dump;

	if (obj == NULL) {
		return;
	}

	dump = object_dump (obj);
	if (dump != NULL) {
		printf ("%s\n", strobject_c_str (dump));
		object_free (dump);
	}
}

void
object_init ()
{
	/* Init some types of objects. */
	nullobject_init ();
	boolobject_init ();
	intobject_init ();
	longobject_init ();
	strobject_init ();
	vecobject_init ();
	dictobject_init ();
}
