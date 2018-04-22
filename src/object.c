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
#include <math.h>

#include "object.h"
#include "pool.h"
#include "error.h"
#include "nullobject.h"
#include "boolobject.h"
#include "charobject.h"
#include "intobject.h"
#include "longobject.h"
#include "floatobject.h"
#include "doubleobject.h"
#include "strobject.h"

#define FLOATING_INT_TO_HASH_NEG -271828
#define FLOATING_INT_TO_HASH_POS 314159

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
		/* Never reachable. */
		default:
			error ("try to get integer value from wrong type.");
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
			error ("try to get floating value from wrong type.");
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
			case OBJECT_TYPE_FLOAT:
				return floatobject_new ((float) val, NULL);
			case OBJECT_TYPE_DOUBLE:
				return doubleobject_new ((double) val, NULL);
			default:
				error ("try to cast numberical object to other types.");
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
			case OBJECT_TYPE_FLOAT:
				return floatobject_new ((float) val, NULL);
			case OBJECT_TYPE_DOUBLE:
				return doubleobject_new ((double) val, NULL);
			default:
				error ("try to cast numberical object to other types.");
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
object_not (object_t *obj)
{
	una_op_f not_fun;

	if (!NUMBERICAL_TYPE (obj)) {
		error ("invalid operand type for '!'.");

		return NULL;
	}
	not_fun = (OBJECT_OPSET (obj))->not;

	return not_fun (obj);
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
		error ("this type has no dump routine.");

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
		error ("invalid operand type for '!'.");

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
		if (OBJECT_TYPE (obj2) != OBJECT_TYPE_STR && !NUMBERICAL_TYPE (obj2)) {
			error ("invalid right operand type for '+'.");
		
			return NULL;
		}

		add_fun = (OBJECT_OPSET (obj1))->add;
		
		return add_fun (obj1, obj2);
	}
	if (OBJECT_TYPE (obj1) == OBJECT_TYPE_VEC) {
		if (OBJECT_TYPE (obj1) != OBJECT_TYPE (obj2)) {
			error ("invalid right operand type for '+'.");
		
			return NULL;
		}

		add_fun = (OBJECT_OPSET (obj1))->add;
		
		return add_fun (obj1, obj2);
	}

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type for '+'.");

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type for '+'.");
		
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
		error ("invalid left operand type for '-'.");

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type for '-'.");
		
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
		error ("invalid left operand type for '*'.");

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type for '*'.");
		
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
		error ("invalid left operand type for '/'.");

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type for '/'.");
		
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
		error ("invalid left operand type for '%'.");

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type for '%'.");
		
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
object_and (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f and_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type for '&'.");

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type for '&'.");
		
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
object_or (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f or_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type for '|'.");

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type for '|'.");
		
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
object_xor (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *right;
	object_t *res;
	bin_op_f xor_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type for '^'.");

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type for '^'.");
		
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
object_land (object_t *obj1, object_t *obj2)
{
	bin_op_f land_fun;

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type for '&&'.");

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type for '&&'.");
		
		return NULL;
	}

	land_fun = (OBJECT_OPSET (obj1))->land;

	return land_fun (obj1, obj2);
}

object_t *
object_lor (object_t *obj1, object_t *obj2)
{
	bin_op_f lor_fun;

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type for '||'.");

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type for '||'.");
		
		return NULL;
	}

	lor_fun = (OBJECT_OPSET (obj1))->lor;

	return lor_fun (obj1, obj2);
}

object_t *
object_lshift (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *res;
	bin_op_f lshift_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type for '<<'.");

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type for '<<'.");
		
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
object_rshift (object_t *obj1, object_t *obj2)
{
	object_t *left;
	object_t *res;
	bin_op_f rshift_fun;

	if (!INTEGER_TYPE (obj1)) {
		error ("invalid left operand type for '>>'.");

		return NULL;
	}
	if (!INTEGER_TYPE (obj2)) {
		error ("invalid right operand type for '>>'.");
		
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
object_eq (object_t *obj1, object_t *obj2)
{
	bin_op_f eq_fun;

	eq_fun = (OBJECT_OPSET (obj1))->rshift;
	if (eq_fun == NULL) {
		/* Actually all types have equality routine. */
		error ("on equality routine for left operand.");

		return NULL;
	}

	return eq_fun (obj1, obj2);
}

object_t *
object_cmp (object_t *obj1, object_t *obj2)
{
	bin_op_f cmp_fun;

	if (OBJECT_TYPE (obj1) == OBJECT_TYPE_STR
		&& OBJECT_TYPE (obj2) == OBJECT_TYPE_STR) {
		cmp_fun = (OBJECT_OPSET (obj1))->cmp;

		return cmp_fun (obj1, obj2);
	}

	if (!NUMBERICAL_TYPE (obj1)) {
		error ("invalid left operand type for comparation.");

		return NULL;
	}
	if (!NUMBERICAL_TYPE (obj2)) {
		error ("invalid right operand type for comparation.");
		
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
		error ("left operand has no index routine.");
		
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
		error ("left operand has no inplace index routine.");
		
		return NULL;
	}

	return ipindex_fun (obj1, obj2, obj3);
}

uint64_t
object_integer_hash (integer_value_t val)
{
	return val;
}

uint64_t
object_floating_hash (floating_value_t val)
{
	floating_value_t frac_part;
	floating_value_t int_part;
	uint64_t int_to_hash;

	int_to_hash = 0;
	/* f64 can hold all floating values on koa. */
	if (!FLOATING_FINITE (val)) {
		if (FLOATING_IS_INFINITY (val)) {
			int_to_hash = val < 0?
				FLOATING_INT_TO_HASH_NEG: FLOATING_INT_TO_HASH_POS;
		}
		else
			int_to_hash = 0;
	}
	else {
		/* If val has fraction part, it won't bother
		 * to hash as an integer directly. */
		frac_part = modf (val, &int_part);
		if (frac_part == 0.0) {
			int_to_hash = (uint64_t) (int_part + 0.1);
		}
		else {
			void *val_p;

			/* It is safe to convert a floating value (double)
			 * to an uint64_t bitwise. */
			val_p = (void *) &val;
			int_to_hash = *((uint64_t *) val_p);
		}
	}

	return object_integer_hash (int_to_hash);	
}

/* We use a long object to represent hash digest of objects.
 * Although it may be down casted when a long is presented
 * as 32 bits integer, it will still works well if our 64-bit
 * hash algorithm is solid enough. */
object_t *
object_hash (object_t *obj)
{
	una_op_f hash_fun;

	hash_fun = (OBJECT_OPSET (obj))->hash;
	/* At this stage, this is impossible, =_=. */
	if (hash_fun == NULL) {
		error ("this type has no hash routine.");
		
		return NULL;
	}

	return hash_fun (obj);
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
}
