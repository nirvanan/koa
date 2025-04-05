/*
 * shortobject.h
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

#ifndef SHORTOBJECT_H
#define SHORTOBJECT_H

#include "koa.h"
#include "object.h"

typedef struct shortobject_s
{
	object_head_t head;
	short val;
} shortobject_t;

object_t *
shortobject_load_binary (FILE *f);

object_t *
shortobject_load_buf (const char **buf, size_t *len);

object_t *
shortobject_new (short val, void *udata);

short
shortobject_get_value (object_t *obj);

#endif /* SHORTOBJECT_H */
