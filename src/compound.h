/*
 * compound.h
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

#ifndef COMPOUND_H
#define COMPOUND_H

#include <stdio.h>

#include "koa.h"
#include "str.h"
#include "vec.h"
#include "object.h"

typedef struct field_s
{
	object_type_t type;
	str_t *name;
} field_t;

typedef struct compound_s
{
	str_t *name;
	vec_t *fields;
} compound_t;

compound_t *
compound_new (const char *name);

void
compound_free (compound_t *meta);

void
compound_push_field (compound_t *meta, const char *name, object_type_t type);

compound_t *
compound_load_binary (FILE *f);

compound_t *
compound_load_buf (const char **buf, size_t *len);

str_t *
compound_to_binary (compound_t *meta);

str_t *
compound_get_name (compound_t *meta);

str_t *
compound_get_field_name (compound_t *meta, integer_value_t pos);

object_type_t
compound_get_field_type (compound_t *meta, integer_value_t pos);

integer_value_t
compound_find_field (compound_t *meta, str_t *name);

size_t
compound_size (compound_t *meta);

#endif /* COMPOUND_H */
