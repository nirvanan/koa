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

#include "object.h"
#include "pool.h"
#include "error.h"

static object_t *
object_new (object_type_t t, void *udata)
{
	object_t *ob;

	ob = (object_t *) pool_alloc (sizeof (object_t));
	ob->head.ref = 0;
	ob->head.type = t;
	ob->udata = udata;

	return ob;
}

object_t *
object_byte_new (byte_t b, void *udata)
{
	object_t *ob;

	ob = object_new (OBJECT_TYPE_BYTE, udata);
	ob->value.b = b;

	return ob;
}

object_t *
object_int_new (const int i, void *udata)
{
	object_t *ob;

	ob = object_new (OBJECT_TYPE_INT, udata);
	ob->value.i = i;

	return ob;
}

object_t *
object_float_new (const float f, void *udata)
{
	object_t *ob;

	ob = object_new (OBJECT_TYPE_FLOAT, udata);
	ob->value.f = f;

	return ob;
}

object_t *
object_double_new (const double d, void *udata)
{
	object_t *ob;

	ob = object_new (OBJECT_TYPE_DOUBLE, udata);
	ob->value.d = d;

	return ob;
}
