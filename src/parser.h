/*
 * parser.h
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

#ifndef PARSER_H
#define PARSER_H

#include "koa.h"
#include "code.h"
#include "lex.h"

typedef struct parser_s
{
	reader_t *reader; /* Token stream source. */
	const char *path; /* Source file path. */
	token_t *token; /* Current token. */
	code_t *global; /* Top level. */
	int cmdline; /* Flag for cmdline parser. */
} parser_t;

code_t *
parser_load_file (const char *path);

code_t *
parser_load_buf (const char *path, str_t *buf);

int
parser_command_line (parser_t *parser, code_t *code);

parser_t *
parser_new_cmdline (const char *path, code_t *global, get_char_f rf,
					clear_f cf, void *udata);

void
parser_cmdline_done (parser_t *parser);

#endif /* PARSER_H */
