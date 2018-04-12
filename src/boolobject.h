/*
 * boolobject.h
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

#ifndef BOOLOBJECT_H
#define BOOLOBJECT_H

#include <stdbool.h>

#include "koa.h"
#include "object.h"

typedef struct boolobject_s
{
	object_head_t head;
	bool val;
} boolobject_t;

object_t *
boolobject_new (bool val, void *udata);

bool
boolobject_get_value (object_t *obj);

void
boolobject_init ();

#endif /* BOOLOBJECT_H */
