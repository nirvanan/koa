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

#include <string.h>

#include "str.h"
#include "pool.h"

str_t *
str_new (const char *s)
{
	size_t len;
	size_t str_size;
	str_t *str;

	if (s == NULL) {
		len = 0;
	}
	else {
		len = strlen (s);
	}

	str_size = len + sizeof (str_t) + 1;

	str = (str_t *) pool_alloc (str_size);
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
	pool_free (str);
}
