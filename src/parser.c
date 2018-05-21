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
#include "pool.h"
#include "lex.h"
#include "code.h"
#include "object.h"
#include "str.h"
#include "vec.h"
#include "error.h"

typedef struct parser_s
{
	reader_t *reader; /* Token stream source. */
	const char *path; /* Source file path. */
	vec_t *seq; /* The parsed token sequence. */
	token_t *token; /* Current token. */
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

static void
parser_buf_clear (void *udata)
{
	buf_read_t *b;

	b = (buf_read_t *) udata;
	str_free (b->buf);
	pool_free (udata);
}

static void
parser_free (parser_t *parser)
{
	if (parser->reader != NULL) {
		lex_reader_free (parser->reader);
	}
	if (parser->seq != NULL) {
		vec_free (parser->seq);
	}

	pool_free ((void *) parser);
}

static void
parser_next_token (parser_t * parser)
{
	token_t *prev;

	prev = parser->token;
	parser->token = lex_next (parser->reader);
	if (prev != NULL) {
		lex_token_free (prev);
	}
}

static int
parser_check (parser_t * parser, token_type_t need)
{
	/* token can be NULL if lex error occurred. */
	if (parser->token == NULL) {
		return 0;
	}

	return TOKEN_TYPE (parser->token) == need;
}

static object_type_t
parser_token_object_type (parser_t *parser)
{
	/* token can be NULL if lex error occurred. */
	if (parser->token == NULL) {
		return (object_type_t) -1;
	}

	switch (TOKEN_TYPE (parser->token)) {
		case TOKEN_NULL:
			return OBJECT_TYPE_NULL;
		case TOKEN_BOOL:
			return OBJECT_TYPE_BOOL;
		case TOKEN_CHAR:
			return OBJECT_TYPE_CHAR;
		case TOKEN_INT:
			return OBJECT_TYPE_INT;
		case TOKEN_LONG:
			return OBJECT_TYPE_LONG;
		case TOKEN_FLOAT:
			return OBJECT_TYPE_FLOAT;
		case TOKEN_DOUBLE:
			return OBJECT_TYPE_DOUBLE;
		case TOKEN_STR:
			return OBJECT_TYPE_STR;
		case TOKEN_VEC:
			return OBJECT_TYPE_VEC;
		case TOKEN_DICT:
			return OBJECT_TYPE_DICT;
		case TOKEN_FUNC:
			return OBJECT_TYPE_FUNC;
		default:
			break;
	}

	return (object_type_t) -1;
}

/* external-declaration:
 * function-definition
 * declaration */
static int
parser_external_declaration (parser_t *parser, code_t *code)
{
	/* Need to look ahead 3 tokens: type id '('*/
	object_type_t type;
	para_offset_t pos;

	type = parser_token_object_type (parser);
	if (type == -1) {
		return 0;
	}

	parser_next_token (parser);
	if (!parser_check (parser, TOKEN_IDENTIFIER)) {
		error ("missing identifier name.");

		return 0;
	}

	parser_next_token (parser);
	if (parser_check (parser, TOKEN ('('))) {
	}

	return 1;
}

/* translation-unit:
 * external-declaration
 * external-declaration translation-unit */
static int
parser_translation_unit (parser_t *parser, code_t *code)
{
	while (!parser_check (parser, TOKEN_END)) {
		if (!parser_external_declaration (parser, code)) {
			return 0;
		}
	}

	return 1;
}

code_t *
parser_load_file (const char *path)
{
	code_t *code;
	parser_t *parser;
	FILE *f;

	code = code_new (path, "@global");
	if (code == NULL) {
		return NULL;
	}

	f = fopen (path, "r");
	if (f == NULL) {
		code_free (code);
		error ("failed to open source file.");

		return NULL;
	}

	parser = (parser_t *) pool_calloc (1, sizeof (parser_t));
	if (parser == NULL) {
		code_free (code);
		error ("out of memory.");

		return NULL;
	}

	parser->reader = lex_reader_new (path, parser_file_reader,
		parser_file_close, (void *) f);
	if (parser->reader == NULL) {
		code_free (code);
		pool_free ((void *) parser);

		return NULL;
	}

	parser->seq = vec_new (0);
	if (parser->seq == NULL) {
		code_free (code);
		parser_free (parser);

		return NULL;
	}

	parser->path = path;

	parser_next_token (parser);
	if (!parser_translation_unit (parser, code)) {
		code_free (code);
		parser_free (parser);

		return NULL;
	}

	return code;
}

code_t *
parser_load_buf (const char *path, str_t *buf)
{
	code_t *code;
	parser_t *parser;
	buf_read_t *b;

	code = code_new (path, "@global");
	if (code == NULL) {
		return NULL;
	}

	b = (buf_read_t *) pool_alloc (sizeof (buf_read_t));
	if (b == NULL) {
		error ("out of memory.");

		return NULL;
	}
	b->buf = buf;
	b->p = 0;

	parser = (parser_t *) pool_calloc (1, sizeof (parser_t));
	if (parser == NULL) {
		error ("out of memory.");

		return NULL;
	}

	parser->reader = lex_reader_new (path, parser_buf_reader,
		parser_buf_clear, (void *) b);
	if (parser->reader == NULL) {
		pool_free ((void *) parser);

		return NULL;
	}

	parser->seq = vec_new (0);
	if (parser->seq == NULL) {
		parser_free (parser);

		return NULL;
	}

	parser->path = path;

	return code;
}
