/*
 * lex.c
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

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "lex.h"
#include "pool.h"
#include "error.h"
#include "dictobject.h"
#include "strobject.h"
#include "intobject.h"

/* Warppers for character tests. */
#define LEX_IS_ALPHA(x) (isalpha((x))||(x)=='_')
#define LEX_IS_ALNUM(x) (isalnum((x))||(x)=='_')
#define LEX_IS_DIGIT(x) (isdigit((x)))
#define LEX_IS_XDIGIT(x) (isxdigit((x)))
#define LEX_IS_SPACE(x) (isspace((x)))
#define LEX_IS_PRINT(x) (isprint((x)))

typedef struct reserved_word_s
{
	const char *word;
	token_type_t type;
} reserved_word_t;

/* Pretty simple ^_^, rigit? */
static reserved_word_t g_reserved_list[] =
{
	{"bool", TOKEN_BOOL},
	{"char", TOKEN_CHAR},
	{"int", TOKEN_INT},
	{"long", TOKEN_LONG},
	{"float", TOKEN_FLOAT},
	{"double", TOKEN_DOUBLE},
	{"str", TOKEN_STR},
	{"vec", TOKEN_VEC},
	{"dict", TOKEN_DICT},
	{"case", TOKEN_CASE},
	{"default", TOKEN_DEFAULT},
	{"if", TOKEN_IF},
	{"else", TOKEN_ELSE},
	{"switch", TOKEN_SWITCH},
	{"while", TOKEN_WHILE},
	{"do", TOKEN_DO},
	{"for", TOKEN_FOR},
	{"continue", TOKEN_CONTINUE},
	{"break", TOKEN_BREAK},
	{"return", TOKEN_RETURN},
	{"true", TOKEN_TRUE},
	{"false", TOKEN_FALSE},
	{NULL, 0}
};

static object_t *g_reserved_tokens;

static void
lex_clear_loaded_buff (reader_t *reader)
{
	reader->loaded_len = 0;
	reader->next_loaded = 0;
}

static void
lex_next_char (reader_t *reader)
{
	/* Try get previous loaded char. */
	if (reader->next_loaded < reader->loaded_len) {
		reader->current = reader->loaded[reader->next_loaded++];

		return;
	}

	errno = 0;
	reader->current = fgetc (reader->f);
	if (errno > 0) {
		error ("failed to read source file.");
	}
}

static int
lex_skip_utf8_bom (reader_t *reader)
{
	const char *bom;
	size_t bom_len;

	bom = "\xEF\xBB\xBF";
	bom_len = strlen (bom);
	for (size_t i = 0; i < bom_len && i < MAX_TOKEN_LEN; i++) {
		errno = 0;
		reader->loaded[reader->loaded_len++] = fgetc (reader->f);
		if (errno) {
			error ("failed to read source file.");

			return 0;
		}
	}

	/* Well, hit it. */
	if (strcmp (reader->loaded, bom) == 0) {
		lex_clear_loaded_buff (reader);
	}

	return 1;
}

reader_t *
lex_reader_new (const char *path)
{
	reader_t *reader;

	reader = (reader_t *) pool_calloc ((size_t) 1, sizeof (reader_t));
	if (reader == NULL) {
		error ("out of memory.");

		return NULL;
	}

	reader->f = fopen (path, "r");
	if (reader->f == NULL) {
		pool_free ((void *) reader);
		error ("can not open source file.");

		return NULL;
	}

	reader->current = 0;
	reader->line = 1;

	if (lex_skip_utf8_bom (reader) == 0) {
		lex_reader_free (reader);

		return NULL;
	}
	
	lex_next_char (reader);

	return reader;
}

void
lex_reader_free (reader_t *reader)
{
	UNUSED (fclose (reader->f));

	pool_free ((void *) reader);
}

void
lex_init()
{
	reserved_word_t *re;

	/* Insert all reserved words to look-up table. */
	g_reserved_tokens = dictobject_new (NULL);
	if (g_reserved_tokens == NULL) {
		fatal_error ("failed to allocate the reserved word dict.");
	}

	re = &g_reserved_list[0];
	while (re->word != NULL) {
		object_t *suc;

		suc = object_ipindex (g_reserved_tokens,
			strobject_new (re->word, NULL),
			intobject_new ((int) re->type, NULL));
		if (suc == NULL) {
			fatal_error ("failed to generate the reserved word dict.");
		}

		re++;
	}
}
