/*
 * uint32object.h
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

#ifndef UINT32OBJECT_H
#define UINT32OBJECT_H

#include "koa.h"
#include "object.h"

typedef struct uint32object_s
{
	object_head_t head;
	uint32_t val;
} uint32object_t;

object_t *
uint32object_load_binary (FILE *f);

object_t *
uint32object_new (uint32_t val, void *udata);

uint32_t
uint32object_get_value (object_t *obj);

void
uint32object_init ();

#endif /* UINT32OBJECT_H */
