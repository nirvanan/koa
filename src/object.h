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

#include "koa.h"

typedef unsigned char byte_t;

typedef enum object_type_e {
	OBJECT_TYPE_NONE = 0x00,
	OBJECT_TYPE_BOOL = 0x01,
	OBJECT_TYPE_BYTE = 0x02,
	OBJECT_TYPE_INT = 0x03,
	OBJECT_TYPE_FLOAT = 0x04,
	OBJECT_TYPE_DOUBLE = 0x05,
	OBJECT_TYPE_STR = 0x06,
	OBJECT_TYPE_VEC = 0x07,
	OBJECT_TYPE_DICT = 0x08,
	OBJECT_TYPE_RAW = 0x09,
} object_type_t;

typedef struct object_head_s
{
	int ref;
	object_type_t type;
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
	void_una_op_f dump;
	una_op_f neg;
	una_op_f call;
	bin_op_f add;
	bin_op_f sub;
	bin_op_f mul;
	bin_op_f mod;
	bin_op_f div;
	bin_op_f and;
	bin_op_f or;
	bin_op_f xor;
	bin_op_f lshift;
	bin_op_f rshift;
	bin_op_f cmp;
	bin_op_f index;
} object_opset_t;

object_t *
object_new (void *udata);

void
object_ref (object_t *ob);

void
object_unref (object_t *ob);

void
object_free (object_t *ob);

void
object_init ();

#endif /* OBJECT_H */
