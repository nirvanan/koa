/*
 * longobject.h
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

#ifndef LONGOBJECT_H
#define LONGOBJECT_H

#include "koa.h"
#include "object.h"

typedef struct longobject_s
{
	object_head_t head;
	long val;
} longobject_t;

object_t *
longobject_load_binary (FILE *f);

object_t *
longobject_load_buf (const char **buf, size_t *len);

object_t *
longobject_new (long val, void *udata);

long
longobject_get_value (object_t *obj);

void
longobject_init ();

#endif /* LONGOBJECT_H */
