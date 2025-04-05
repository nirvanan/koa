/*
 * int16object.h
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

#ifndef INT16OBJECT_H
#define INT16OBJECT_H

#include "koa.h"
#include "object.h"

typedef struct int16object_s
{
	object_head_t head;
	int16_t val;
} int16object_t;

object_t *
int16object_load_binary (FILE *f);

object_t *
int16object_load_buf (const char **buf, size_t *len);

object_t *
int16object_new (int16_t val, void *udata);

int16_t
int16object_get_value (object_t *obj);

#endif /* INT16OBJECT_H */
