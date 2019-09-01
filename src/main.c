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
#include "dictobject.h"
#include "doubleobject.h"
#include "charobject.h"
#include "str.h"
#include "lex.h"
#include "code.h"
#include "parser.h"
#include "interpreter.h"
#include <time.h>

int main(int argc, char *argv[])
{
	/* Init pool utility. */
	pool_init ();
	/* Init object caches. */
	object_init ();
	/* Init lex module. */
	lex_init ();
	/* Init interpreter. */
	interpreter_init ();

	int c = 0;

	while (0) {
		object_t *val = dictobject_new (NULL);
		object_t *idx = intobject_new (2, NULL);
		object_t *str = charobject_new ('f', NULL);
		object_t *dump;

		object_ipindex (val, idx, str);
		dump = object_dump (val);
		printf ("%s\n", strobject_get_value(dump)->s);
		object_free (val);
		object_free (dump);
		c++;
		if (c % 600000 == 0)
			printf ("%d\n", c);
	}

	while (1) {
		code_t *code = parser_load_file ("/Users/bytedance/test.k");
		if (code) {
			//code_print (code);
			//object_t *str = code_binary (code);
			//object_t *str = vecobject_new (40000, NULL);
			//object_t *bin = object_binary (str);
			//code_save_binary (code);
			code_free (code);
			//object_free (str);
			//object_free (bin);
		}
		else {
			printf ("wrong\n");
		}
		break;
	}

	while (1) {
		interpreter_execute ("/Users/bytedance/test.k");
	}

	return 0;
}
