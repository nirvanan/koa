/*
 * strobject.h
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

#ifndef STROBJECT_H
#define STROBJECT_H

#include "koa.h"
#include "object.h"
#include "str.h"
#include "hash.h"

typedef struct strobject_s
{
	object_head_t head;
	str_t *val;
	void *hn; /* For quick dehash. */
	int hashed;
} strobject_t;

object_t *
strobject_load_binary (FILE *f);

object_t *
strobject_new (const char *val, size_t len, int no_hash, void *udata);

object_t *
strobject_str_new (str_t *val, void *udata);

str_t *
strobject_get_value (object_t *obj);

uint64_t
strobject_get_hash (object_t *obj);

int
strobject_equal (object_t *obj1, object_t *obj2);

const char *
strobject_c_str (object_t *obj);

void
strobject_init ();

#endif /* STROBJECT_H */
