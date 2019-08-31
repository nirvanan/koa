/*
 * int8object.h
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

#ifndef INT8OBJECT_H
#define INT8OBJECT_H

#include "koa.h"
#include "object.h"

typedef struct int8object_s
{
	object_head_t head;
	int8_t val;
} int8object_t;

object_t *
int8object_load_binary (FILE *f);

object_t *
int8object_new (int8_t val, void *udata);

int8_t
int8object_get_value (object_t *obj);

void
int8object_init ();

#endif /* INT8OBJECT_H */
