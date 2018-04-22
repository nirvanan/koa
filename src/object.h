/*
 * object.h
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

#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

#include "koa.h"

/* In koa world, we do not present unsigned integers
 * explitly, as negetive values of signed objects can
 * be parsed as BIG unsigned value if you are willing
 * to do so.*/

#define INTEGER_TYPE(x) ((x)->head.type == OBJECT_TYPE_BOOL\
	|| (x)->head.type == OBJECT_TYPE_CHAR\
	|| (x)->head.type == OBJECT_TYPE_INT\
	|| (x)->head.type == OBJECT_TYPE_LONG)

#define FLOATING_TYPE(x) ((x)->head.type == OBJECT_TYPE_FLOAT\
	|| (x)->head.type == OBJECT_TYPE_DOUBLE)

#define NUMBERICAL_TYPE(x) ((x)->head.type == OBJECT_TYPE_BOOL \
	|| (x)->head.type == OBJECT_TYPE_CHAR \
	|| (x)->head.type == OBJECT_TYPE_INT \
	|| (x)->head.type == OBJECT_TYPE_LONG \
	|| (x)->head.type == OBJECT_TYPE_FLOAT \
	|| (x)->head.type == OBJECT_TYPE_DOUBLE)

#define TYPE_NAME(x) (g_type_name[(x)->head.type])

#define OBJECT_REF(x) ((x)->head.ref)

#define OBJECT_TYPE(x) ((x)->head.type)

#define OBJECT_OPSET(x) ((object_opset_t *) ((x)->head.ops))

#define OBJECT_OPSET_P(x) ((x)->head.ops)

#define OBJECT_DIGEST(x) ((x)->head.digest)

#define OBJECT_UDATA(x) ((x)->head.udata)

#define OBJECT_NEW_INIT(x, t) OBJECT_REF ((x)) = 0;\
	OBJECT_TYPE ((x)) = t;\
	OBJECT_OPSET_P ((x)) = (void *) &g_object_ops;\
	OBJECT_DIGEST ((x)) = 0;\
	OBJECT_UDATA ((x)) = udata

#define OBJECT_BIGGER(o1, o2) (OBJECT_TYPE((o1))<OBJECT_TYPE((o2))?\
	object_cast((o1),OBJECT_TYPE((o2))):(o1))

#define NUMBERICAL_GET_VALUE(x) (INTEGER_TYPE((x))?\
	object_get_integer((x)):object_get_floating((x)))

#define FLOATING_IS_NAN isnan
#define FLOATING_IS_INFINITY(x) (!isfinite((x))&&!isnan((x)))
#define FLOATING_FINITE(x) isfinite((x))

typedef enum object_type_e {
	OBJECT_TYPE_NULL = 0x00,
	OBJECT_TYPE_BOOL = 0x01,
	OBJECT_TYPE_CHAR = 0x02,
	OBJECT_TYPE_INT = 0x03,
	OBJECT_TYPE_LONG = 0x04,
	OBJECT_TYPE_FLOAT = 0x05,
	OBJECT_TYPE_DOUBLE = 0x06,
	OBJECT_TYPE_STR = 0x07,
	OBJECT_TYPE_VEC = 0x08,
	OBJECT_TYPE_DICT = 0x09,
} object_type_t;

typedef struct object_head_s
{
	int ref;
	object_type_t type;
	uint64_t digest;
	void *ops;
	void *udata;
} object_head_t;

typedef struct object_s
{
	object_head_t head;
} object_t;

typedef object_t *(*una_op_f) (object_t *obj);

typedef void (*void_una_op_f) (object_t *obj);

typedef object_t *(*bin_op_f) (object_t *obj1, object_t *obj2); 

typedef object_t *(*ter_op_f) (object_t *obj1, object_t *obj2, object_t *ob3);

typedef struct object_opset_s
{
	una_op_f not;
	void_una_op_f free;
	una_op_f dump;
	una_op_f neg;
	una_op_f call;
	bin_op_f add;
	bin_op_f sub;
	bin_op_f mul;
	bin_op_f div;
	bin_op_f mod;
	bin_op_f and;
	bin_op_f or;
	bin_op_f xor;
	bin_op_f land;
	bin_op_f lor;
	bin_op_f lshift;
	bin_op_f rshift;
	bin_op_f eq;
	bin_op_f cmp;
	bin_op_f index;
	ter_op_f ipindex;
	una_op_f hash;
} object_opset_t;

/*
static const char *g_type_name[] =
{
	"null",
	"bool",
	"char",
	"int",
	"long",
	"float",
	"double",
	"str",
	"vec",
	"dict"
};
*/

void
object_ref (object_t *obj);

void
object_unref (object_t *obj);

integer_value_t
object_get_integer (object_t *obj);

floating_value_t
object_get_floating (object_t *obj);

int
object_is_zero (object_t *obj);

object_t *
object_cast (object_t *obj, object_type_t type);

int
object_numberical_compare (object_t *obj1, object_t *obj2);

object_t *
object_not (object_t *obj1);

void
object_free (object_t *obj);

object_t *
object_dump (object_t *obj);

object_t *
object_neg (object_t *obj1);

object_t *
object_add (object_t *obj1, object_t *obj2);

object_t *
object_sub (object_t *obj1, object_t *obj2);

object_t *
object_mul (object_t *obj1, object_t *obj2);

object_t *
object_div (object_t *obj1, object_t *obj2);

object_t *
object_mod (object_t *obj1, object_t *obj2);

object_t *
object_bit_and (object_t *obj1, object_t *obj2);

object_t *
object_bit_or (object_t *obj1, object_t *obj2);

object_t *
object_bit_xor (object_t *obj1, object_t *obj2);

object_t *
object_logic_and (object_t *obj1, object_t *obj2);

object_t *
object_logic_or (object_t *obj1, object_t *obj2);

object_t *
object_left_shift (object_t *obj1, object_t *obj2);

object_t *
object_right_shift (object_t *obj1, object_t *obj2);

object_t *
object_equal (object_t *obj1, object_t *obj2);

object_t *
object_compare (object_t *obj1, object_t *obj2);

object_t *
object_index (object_t *obj1, object_t *obj2);

object_t *
object_ipindex (object_t *obj1, object_t *obj2, object_t *obj3);

uint64_t
object_integer_hash (integer_value_t val);

uint64_t
object_floating_hash (floating_value_t val);

object_t *
object_hash (object_t *obj);

void
object_init ();

#endif /* OBJECT_H */
