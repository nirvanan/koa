/*
 * structobject.h
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

#ifndef STRUCTOBJECT_H
#define STRUCTOBJECT_H

#include "koa.h"
#include "object.h"
#include "code.h"
#include "vec.h"

typedef struct structobject_s
{
	object_head_t head;
	vec_t *members;
} structobject_t;

object_t *
structobject_load_binary (object_type_t type, FILE *f);

object_t *
structobject_new (code_t *code, object_type_t type, void *udata);

void
structobject_traverse (object_t *obj, traverse_f fun, void *udata);

object_t *
structobject_get_member (object_t *obj, object_t *name, code_t *code);

object_t *
structobject_store_member (object_t *obj, object_t *name,
						   object_t *value, code_t *code);

object_t *
structobject_copy (object_t *obj);

void
structobject_init ();

#endif /* STRUCTOBJECT_H */
