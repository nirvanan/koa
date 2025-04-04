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
#include <stdlib.h>
#include "pool.h"
#include "object.h"
#include "lex.h"
#include "code.h"
#include "interpreter.h"
#include "builtin.h"
#include "cmdline.h"
#include "parser.h"
#include "gc.h"
#include "thread.h"
#include "opt.h"
#include "misc.h"

void koa_init ()
{
	thread_set_main_thread ();
	/* Init gc. */
	gc_init ();
	/* Init pool utility. */
	pool_init ();
	/* Init object caches. */
	object_init ();
	/* Init lex module. */
	lex_init ();
	/* Init interpreter. */
	interpreter_init ();
	/* Init builtin. */
	builtin_init ();
	/* Init thread. */
	thread_init ();
}

int main(int argc, char *argv[])
{
	opt_t *opts;

	opts = opt_parse_opts (argc, argv);
	if (opts == NULL) {
		return 0;
	}

	if (opts->help) {
		misc_print_usage (0);

		/* Quit after printing. */
		return 0;
	}
	if (opts->version) {
		misc_print_version ();

		/* Quit after printing. */
		return 0;
	}

	koa_init ();

	if (opts->print) {
		code_t *code;

		code = parser_load_file (opts->path);
		if (code == NULL) {
			return 0;
		}
		code_print (code);

		/* Quit after printing. */
		return 0;
	}

	if (opts->path[0] != '\0') {
		interpreter_execute (opts->path);
	}
	else {
		cmdline_start ();
	}

	return 0;
}
