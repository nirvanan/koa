/*
 * vecobject.h
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

#ifndef VECOBJECT_H
#define VECOBJECT_H

#include "koa.h"
#include "object.h"
#include "vec.h"

typedef struct vecobject_s
{
	object_head_t head;
	vec_t *val;
} vecobject_t;

object_t *
vecobject_new (size_t len, void *udata);

object_t *
vecobject_vec_new (vec_t *val, void *udata);

vec_t *
vecobject_get_value (object_t *obj);

#endif /* VECOBJECT_H */
