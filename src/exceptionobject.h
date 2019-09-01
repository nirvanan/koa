/*
 * exceptionobject.h
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

#ifndef EXCEPTIONOBJECT_H
#define EXCEPTIONOBJECT_H

#include "koa.h"
#include "object.h"
#include "str.h"

typedef struct exceptionobject_s
{
	object_head_t head;
	str_t *val;
} exceptionobject_t;

object_t *
exceptionobject_load_binary (FILE *f);

object_t *
exceptionobject_new (const char *val, size_t len, void *udata);

object_t *
exceptionobject_str_new (str_t *val, void *udata);

str_t *
exceptionobject_get_value (object_t *obj);

void
exceptionobject_init ();

#endif /* EXCEPTIONOBJECT_H */
