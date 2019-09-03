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
#include <stdio.h>

#include "koa.h"
#include "str.h"

/* In koa world, we do not present unsigned integers explitly,
 * as negetive values of signed objects can be parsed as BIG
 * unsigned value if you are willing to do so.*/

#define BINARY(x) ((const char*)&(x))

#define INTEGER_TYPE(x) ((x)->head.type==OBJECT_TYPE_BOOL||\
	(x)->head.type==OBJECT_TYPE_CHAR||\
	(x)->head.type==OBJECT_TYPE_INT||\
	(x)->head.type==OBJECT_TYPE_LONG||\
	(x)->head.type==OBJECT_TYPE_INT8||\
	(x)->head.type==OBJECT_TYPE_UINT8||\
	(x)->head.type==OBJECT_TYPE_INT16||\
	(x)->head.type==OBJECT_TYPE_UINT16||\
	(x)->head.type==OBJECT_TYPE_INT32||\
	(x)->head.type==OBJECT_TYPE_UINT32||\
	(x)->head.type==OBJECT_TYPE_INT64||\
	(x)->head.type==OBJECT_TYPE_UINT64)

#define FLOATING_TYPE(x) ((x)->head.type==OBJECT_TYPE_FLOAT||\
	(x)->head.type==OBJECT_TYPE_DOUBLE)

#define NUMBERICAL_TYPE(x) ((x)->head.type == OBJECT_TYPE_BOOL||\
	(x)->head.type==OBJECT_TYPE_CHAR||\
	(x)->head.type==OBJECT_TYPE_INT||\
	(x)->head.type==OBJECT_TYPE_LONG||\
	(x)->head.type==OBJECT_TYPE_INT8||\
	(x)->head.type==OBJECT_TYPE_UINT8||\
	(x)->head.type==OBJECT_TYPE_INT16||\
	(x)->head.type==OBJECT_TYPE_UINT16||\
	(x)->head.type==OBJECT_TYPE_INT32||\
	(x)->head.type==OBJECT_TYPE_UINT32||\
	(x)->head.type==OBJECT_TYPE_INT64||\
	(x)->head.type==OBJECT_TYPE_UINT64||\
	(x)->head.type==OBJECT_TYPE_FLOAT||\
	(x)->head.type==OBJECT_TYPE_DOUBLE)

#define OBJECT_REF(x) ((x)->head.ref)
#define OBJECT_TYPE(x) ((x)->head.type)
#define OBJECT_OPSET(x) ((object_opset_t *) ((x)->head.ops))
#define OBJECT_OPSET_P(x) ((x)->head.ops)
#define OBJECT_DIGEST(x) ((x)->head.digest)
#define OBJECT_DIGEST_FUN(x) ((x)->head.digest_fun)
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

#define OBJECT_IS_DUMMY(x) (OBJECT_TYPE((x))==OBJECT_TYPE_VOID)
#define OBJECT_IS_NULL(x) (OBJECT_TYPE((x))==OBJECT_TYPE_NULL)
#define OBJECT_IS_BOOL(x) (OBJECT_TYPE((x))==OBJECT_TYPE_BOOL)
#define OBJECT_IS_CHAR(x) (OBJECT_TYPE((x))==OBJECT_TYPE_CHAR)
#define OBJECT_IS_INT(x) (OBJECT_TYPE((x))==OBJECT_TYPE_INT)
#define OBJECT_IS_LONG(x) (OBJECT_TYPE((x))==OBJECT_TYPE_LONG)
#define OBJECT_IS_INT8(x) (OBJECT_TYPE((x))==OBJECT_TYPE_INT8)
#define OBJECT_IS_UINT8(x) (OBJECT_TYPE((x))==OBJECT_TYPE_UINT8)
#define OBJECT_IS_INT16(x) (OBJECT_TYPE((x))==OBJECT_TYPE_INT16)
#define OBJECT_IS_UINT16(x) (OBJECT_TYPE((x))==OBJECT_TYPE_UINT16)
#define OBJECT_IS_INT32(x) (OBJECT_TYPE((x))==OBJECT_TYPE_INT)
#define OBJECT_IS_UINT32(x) (OBJECT_TYPE((x))==OBJECT_TYPE_UINT32)
#define OBJECT_IS_INT64(x) (OBJECT_TYPE((x))==OBJECT_TYPE_INT64)
#define OBJECT_IS_UINT64(x) (OBJECT_TYPE((x))==OBJECT_TYPE_UINT64)
#define OBJECT_IS_FLOAT(x) (OBJECT_TYPE((x))==OBJECT_TYPE_FLOAT)
#define OBJECT_IS_DOUBLE(x) (OBJECT_TYPE((x))==OBJECT_TYPE_DOUBLE)
#define OBJECT_IS_STR(x) (OBJECT_TYPE((x))==OBJECT_TYPE_STR)
#define OBJECT_IS_VEC(x) (OBJECT_TYPE((x))==OBJECT_TYPE_VEC)
#define OBJECT_IS_DICT(x) (OBJECT_TYPE((x))==OBJECT_TYPE_DICT)
#define OBJECT_IS_FUNC(x) (OBJECT_TYPE((x))==OBJECT_TYPE_FUNC)
#define OBJECT_IS_FRAME(x) (OBJECT_TYPE((x))==OBJECT_TYPE_FRAME)
#define OBJECT_IS_EXCEPTION(x) (OBJECT_TYPE((x))==OBJECT_TYPE_EXCEPTION)

typedef enum object_type_e
{
	OBJECT_TYPE_ALL = -0x02,
	OBJECT_TYPE_ERR = -0x01,
	OBJECT_TYPE_VOID = 0x00, /* ONLY for function return type. */
	OBJECT_TYPE_NULL = 0x01,
	OBJECT_TYPE_BOOL = 0x02,
	OBJECT_TYPE_CHAR = 0x03,
	OBJECT_TYPE_INT = 0x04,
	OBJECT_TYPE_LONG = 0x05,
	OBJECT_TYPE_INT8 = 0x06,
	OBJECT_TYPE_UINT8 = 0x07,
	OBJECT_TYPE_INT16 = 0x08,
	OBJECT_TYPE_UINT16 = 0x09,
	OBJECT_TYPE_INT32 = 0x0a,
	OBJECT_TYPE_UINT32 = 0x0b,
	OBJECT_TYPE_INT64 = 0x0c,
	OBJECT_TYPE_UINT64 = 0x0d,
	OBJECT_TYPE_FLOAT = 0x0e,
	OBJECT_TYPE_DOUBLE = 0x0f,
	OBJECT_TYPE_STR = 0x10,
	OBJECT_TYPE_VEC = 0x11,
	OBJECT_TYPE_DICT = 0x12,
	OBJECT_TYPE_FUNC = 0x13,
	OBJECT_TYPE_MOD = 0x14,
	OBJECT_TYPE_FRAME = 0x15,
	OBJECT_TYPE_EXCEPTION = 0x16,
} object_type_t;

typedef uint64_t (*digest_f) (void *obj);

typedef struct object_head_s
{
	int ref;
	object_type_t type;
	uint64_t digest;
	digest_f digest_fun;
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
	una_op_f lnot;
	void_una_op_f free;
	void_una_op_f print;
	una_op_f dump;
	una_op_f neg;
	una_op_f call;
	bin_op_f add;
	bin_op_f sub;
	bin_op_f mul;
	bin_op_f div;
	bin_op_f mod;
	una_op_f not;
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
	una_op_f binary;
} object_opset_t;

void
object_ref (object_t *obj);

void
object_unref (object_t *obj);

void
object_unref_without_free (object_t *obj);

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
object_logic_not (object_t *obj1);

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
object_bit_not (object_t *obj);

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

uint64_t
object_address_hash (void *val);

object_t *
object_hash (object_t *obj);

uint64_t
object_digest (object_t *obj);

object_t *
object_binary (object_t *obj);

object_t *
object_get_default (object_type_t type);

object_t *
object_load_binary (FILE *f);

void
object_print (object_t *obj);

void
object_init ();

#endif /* OBJECT_H */
