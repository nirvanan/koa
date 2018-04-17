/*
 * str.h
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

#ifndef STR_H
#define STR_H

#include <stddef.h>

#include "koa.h"

typedef struct str_s {
	size_t len;
	char s[];
} str_t;

str_t *
str_new (const char *s, size_t len);

void
str_free (str_t *str);

size_t
str_len (str_t *str);

const char *
str_c_str (str_t *str);

str_t *
str_concat (str_t *str1, str_t *str2);

char
str_pos (str_t *str, integer_value_t pos);

int
str_cmp (str_t *str1, str_t *str2);

int
str_cmp_c_str (str_t *str, const char *s);

#endif /* STR_H */
