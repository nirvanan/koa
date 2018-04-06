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

#include "str.h"

typedef unsigned char byte_t;

typedef enum object_type_e {
	OBJECT_TYPE_BYTE = 0x01,
	OBJECT_TYPE_INT = 0x02,
	OBJECT_TYPE_FLOAT = 0x04,
	OBJECT_TYPE_DOUBLE = 0x08,
	OBJECT_TYPE_STR = 0x10,
	OBJECT_TYPE_VEC = 0x20,
	OBJECT_TYPE_DICT = 0x40,
	OBJECT_TYPE_RAW = 0x80,
} object_type_t;

typedef struct object_head_s
{
	int ref;
	object_type_t type;
} object_head_t;

typedef struct object_s
{
	object_head_t head;
	union
	{
		byte_t b;
		int i;
		float f;
		double d;
		str_t *str;
	} value;
	void *udata;
} object_t;

object_t *
object_byte_new (byte_t b, void *udata);

object_t *
object_int_new (const int i, void *udata);

object_t *
object_float_new (const float f, void *udata);

object_t *
object_double_new (const double d, void *udata);

object_t *
object_str_new (const char *s, void *udata);

#endif /* OBJECT_H */
