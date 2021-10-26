/*
 * builtin.h
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

#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdio.h>

#include "koa.h"
#include "object.h"
#include "vec.h"
#include "str.h"

typedef object_t *(*builtin_f) (object_t *args);

typedef struct builtin_s {
	int slot;
} builtin_t;

object_t *
builtin_find (object_t *name);

object_t *
builtin_execute (builtin_t *builtin, object_t *args);

void
builtin_free (builtin_t *builtin);

const char *
builtin_get_name (builtin_t *builtin);

int
builtin_no_arg (builtin_t *builtin);

object_t *
builtin_binary (builtin_t *builtin);

builtin_t *
builtin_load_binary (FILE *f);

builtin_t *
builtin_load_buf (const char **buf, size_t *len);

void
builtin_init ();

#endif /* BUILTIN_H */
