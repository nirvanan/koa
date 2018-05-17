/*
 * main.c
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
#include "pool.h"
#include "object.h"
#include "intobject.h"
#include "strobject.h"
#include "vecobject.h"
#include "str.h"
#include "lex.h"
#include <time.h>

int main(int argc, char *argv[])
{
	/* Init pool utility. */
	pool_init ();
	/* Init object caches. */
	object_init ();
	/* Init lex module. */
	lex_init ();
	int c = 0;

	while (1) {
		object_t *str = vecobject_new (10, NULL);
		object_t *idx = intobject_new (4, NULL);
		object_t *val = strobject_new ("fuck", NULL);

		object_add (str, idx);
		object_ipindex (str, idx, val);
		object_ref (val);

		object_free (str);
		object_free (idx);
		c++;
		if (c % 6000000 == 0)
			printf ("%d\n", c);
	}

	const char *path = "/home/nirvanan/sam/stage_8/valid/for_variable_shadow.c";
	reader_t *reader = lex_reader_new (path);
	token_t *token;

	while (1) {
		token = lex_next (reader);
		printf ("%d \"%s\"\n", token->type, token->token);
		if (token->type == TOKEN_END) {
			break;
		}
	}

	return 0;
}
