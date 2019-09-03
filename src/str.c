/*
 * str.c
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

#include <stdio.h>
#include <string.h>

#include "str.h"
#include "pool.h"
#include "error.h"

str_t *
str_new (const char *s, size_t len)
{
	size_t str_size;
	str_t *str;

	str_size = len + sizeof (str_t) + 1;

	str = (str_t *) pool_alloc (str_size);
	if (str == NULL) {
		fatal_error ("out of memory.");
	}

	str->len = len;
	if (len > 0) {
		memcpy (str->s, s, len);
	}
	str->s[len] = '\0';

	return str;
}

void
str_free (str_t *str)
{
	pool_free ((void *) str);
}

size_t
str_len (str_t *str)
{
	return str->len;
}

const char *
str_c_str (str_t *str)
{
	return str->s;
}

static str_t *
str_empty_str_new (size_t len)
{
	size_t str_size;
	str_t *str;

	str_size = len + sizeof (str_t) + 1;

	str = (str_t *) pool_alloc (str_size);
	if (str == NULL) {
		fatal_error ("out of memory.");
	}

	str->len = len;
	memset ((void *) str->s, 0, (len + 1) * sizeof (char));

	return str;
}

str_t *
str_concat (str_t *str1, str_t *str2)
{
	size_t new_len;
	str_t *str;

	new_len = str_len (str1) + str_len (str2);
	str = str_empty_str_new (new_len);
	if (str == NULL) {
		return NULL;
	}

	memcpy (str->s, str1->s, str1->len);
	memcpy (str->s + str1->len, str2->s, str2->len);

	return str;
}

char
str_pos (str_t *str, integer_value_t pos)
{
	if (pos >= 0 && pos < str->len) {
		return str->s[pos];
	}

	return '\0';
}

int
str_cmp (str_t *str1, str_t *str2)
{
	if (str1 == str2) {
		return 0;
	}

	return strcmp (str1->s, str2->s);
}

int
str_cmp_c_str (str_t *str, const char *s)
{
	return strcmp (str->s, s);
}
