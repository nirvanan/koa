/*
 * struct.h
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

#ifndef STRUCT_H
#define STRUCT_H

#include <stdio.h>

#include "koa.h"
#include "str.h"
#include "vec.h"

typedef struct field_s
{
	int type;
	str_t *name;
} field_t;

typedef struct struct_s
{
	str_t *name;
	vec_t *fields;
} struct_t;

struct_t *
struct_new (const char *name);

void
struct_free (struct_t *meta);

void
struct_push_field (struct_t *meta, const char *name, int type);

struct_t *
struct_load_binary (FILE *f);

str_t *
struct_to_binary (struct_t *meta);

str_t *
struct_get_name (struct_t *meta);

str_t *
struct_get_field_name (struct_t *meta, integer_value_t pos);

int
struct_get_field_type (struct_t *meta, integer_value_t pos);

integer_value_t
struct_find_field (struct_t *meta, str_t *name);

#endif /* STRUCT_H */
