/*
 * parser.c
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

#include "parser.h"
#include "lex.h"
#include "code.h"
#include "str.h"
#include "vec.h"

typedef struct parser_s
{
	reader_t *reader; /* Token stream source. */
	code_t *code; /* The generated code. */
	const char *path; /* Source file path. */
	vec_t *seq; /* The parsed token sequence. */
} parser_t;

typedef struct buf_read_s
{
	str_t *buf;
	size_t p;
} buf_read_t;

static char
parser_file_reader (void *udata)
{
	FILE *f;

	f = (FILE *) udata;

	return fgetc (f);
}

static void
parser_file_close (void *udata)
{
	FILE *f;

	f = (FILE *) udata;

	/* Force to close file. */
	UNUSED (fclose (f));
}

static char
parser_buf_reader (void *udata)
{
	buf_read_t *r;

	r = (buf_read_t *) udata;
	if (r->p >= str_len (r->buf)) {
		return EOF;
	}

	return str_pos (r->buf, r->p++);
}
