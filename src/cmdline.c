/*
 * cmdline.c
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

#include "cmdline.h"
#include "misc.h"
#include "code.h"
#include "frame.h"
#include "parser.h"
#include "pool.h"
#include "object.h"
#include "interpreter.h"
#include "error.h"

#define CODE_PATH "stdin"
#define CODE_NAME "#GLOBAL"
#define MAX_LINE_LENGTH 1023

typedef struct stdin_reader_s {
	char line[MAX_LINE_LENGTH + 1];
	size_t len;
	size_t current;
} stdin_reader_t;

static char
cmdline_stdin_reader (void *udata)
{
	stdin_reader_t *r;

	r = (stdin_reader_t *) udata;
	if (r->len == 0) {
		while (r->len == 0) {
			printf (">>> ");
			if (fgets (r->line, MAX_LINE_LENGTH + 1, stdin) == NULL) {
				r->len = 0;
				r->current = 0;
				fatal_error ("stdin read line error.");
			}
			r->len = strlen (r->line);
			r->current = 0;
		}
	}
	else if (r->current >= r->len) {
		r->len = 0;
		r->current = 0;

		return '\n';
	}

	return r->line[r->current++];
}

static void
cmdline_stdin_clear (void *udata)
{
	stdin_reader_t *r;

	r = (stdin_reader_t *) udata;
	pool_free ((void *) r);
}

static void
cmdline_reset (parser_t *parser, stdin_reader_t *r)
{
	r->len = 0;
	r->current = 0;
	parser_cmdline_done (parser);
}

static void
cmdline_print_exception (frame_t *frame)
{
	frame_clear_exception (frame);
}

static void
cmdline_show_help ()
{
	printf ("%s\nCopyright (C) 2018 Gordon Li.\n", misc_get_package_full ());
	printf ("If you have any question, feel free to mail to <%s>.\n", misc_get_bugreport ());
}

void
cmdline_start ()
{
	code_t *code;
	frame_t *frame;
	parser_t *parser;
	stdin_reader_t *r;

	r = (stdin_reader_t *) pool_calloc (1, sizeof (stdin_reader_t));
	if (r == NULL) {
		fatal_error ("out of memory.");
	}

	code = code_new (CODE_PATH, CODE_NAME);
	if (code == NULL) {
		return;
	}

	frame = frame_new (code, NULL, 0, 1, NULL, 1);
	if (frame == NULL) {
		return;
	}
	frame_set_catched (frame);
	interpreter_set_cmdline (frame, code);

	cmdline_show_help ();
	parser = parser_new_cmdline (CODE_PATH, code, cmdline_stdin_reader, cmdline_stdin_clear, (void *) r);
	if (parser == NULL) {
		return;
	}

	for (;;) {
		if (!parser_command_line (parser, code)) {
			cmdline_print_exception (frame);
			cmdline_reset (parser, r);

			continue;
		}
		if (!interpreter_play (code, 1, frame)) {
			frame_reset_esp (frame);
		}
		cmdline_reset (parser, r);
	}
}
