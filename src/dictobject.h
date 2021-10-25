/*
 * dictobject.h
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

#ifndef DICTOBJECT_H
#define DICTOBJECT_H

#include "koa.h"
#include "object.h"
#include "dict.h"

typedef struct dictobject_s
{
	object_head_t head;
	dict_t *val;
} dictobject_t;

object_t *
dictobject_load_binary (FILE *f);

object_t *
dictobject_load_buf (const char **buf, size_t *len);

object_t *
dictobject_new (void *udata);

object_t *
dictobject_dict_new (dict_t *val, void *udata);

dict_t *
dictobject_get_value (object_t *obj);

void
dictobject_traverse (object_t *obj, traverse_f fun, void *udata);

int
dictobject_remove (object_t *obj, object_t *key);

object_t *
dictobject_copy (object_t *obj);

void
dictobject_init ();

#endif /* DICTOBJECT_H */
