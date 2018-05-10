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
#define LEX_IS_ODIGIT(x) (isdigit((x))&&(x)<'8')
#define LEX_IS_SPACE(x) (isspace((x)))
#define LEX_IS_PRINT(x) (isprint((x)))

#define MAX_SOURCE_LINE (0xfffffff0)

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
lex_clear_loaded_buf (reader_t *reader)
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
		if (reader->next_loaded == reader->loaded_len) {
			lex_clear_loaded_buf (reader);
		}

		return;
	}

	reader->current = fgetc (reader->f);
	if (reader->current == EOF && errno > 0) {
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
	for (size_t i = 0; i < bom_len && i < LOADED_BUF_SIZE; i++) {
		reader->loaded[reader->loaded_len++] = fgetc (reader->f);
		if (reader->loaded[reader->loaded_len - 1] == EOF && errno > 0) {
			error ("failed to read source file.");

			return 0;
		}
	}

	/* Well, hit it. */
	if (strcmp (reader->loaded, bom) == 0) {
		lex_clear_loaded_buf (reader);
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

	reader->path = path;
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

static token_t *
lex_token_new ()
{
	token_t *token;

	token = (token_t *) pool_calloc ((size_t) 1, sizeof (token_t));
	if (token == NULL) {
		fatal_error ("out of memory.");
	}

	token->token = (char *) pool_calloc (
		(size_t) (TOKEN_LEN_STEP + 1), sizeof (char));
	if (token->token == NULL) {
		pool_free ((void *) token);
		fatal_error ("out of memory.");
	}

	token->allocated = TOKEN_LEN_STEP + 1;

	return token;
}

void
lex_token_free (token_t *token)
{
	pool_free ((void *) token->token);
	pool_free ((void *) token);
}

static void
lex_new_line (reader_t *reader, char prev)
{
	lex_next_char (reader);
	/* Skip "\r\n" and "\n\r" sequences. */
	if ((reader->current == '\r' || reader->current == '\n') &&
		reader->current != prev) {
		lex_next_char (reader);
	}
	reader->line++;
	if (reader->line > MAX_SOURCE_LINE) {
		fatal_error ("source line exceeded.");
	}
}

static token_t *
lex_check_one_ahead (reader_t *reader, token_t *token)
{
	token->type = (token_type_t) reader->current;
	lex_next_char (reader);
	switch ((char) token->type) {
		case '|':
			if (reader->current == '|') {
				token->type = TOKEN_LOR;
				lex_next_char (reader);
			}
			else if (reader->current == '=') {
				token->type = TOKEN_IPOR;
				lex_next_char (reader);
			}
			break;
		case '&':
			if (reader->current == '&') {
				token->type = TOKEN_LAND;
				lex_next_char (reader);
			}
			else if (reader->current == '=') {
				token->type = TOKEN_IPAND;
				lex_next_char (reader);
			}
			break;
		case '=':
			if (reader->current == '=') {
				token->type = TOKEN_EQ;
				lex_next_char (reader);
			}
			break;
		case '!':
			if (reader->current == '=') {
				token->type = TOKEN_NEQ;
				lex_next_char (reader);
			}
			break;
		case '<':
			if (reader->current == '=') {
				token->type = TOKEN_LEEQ;
				lex_next_char (reader);
			}
			else if (reader->current == '<') {
				token->type = TOKEN_LSHFT;
				lex_next_char (reader);
				if (reader->current == '=') {
					token->type = TOKEN_IPLS;
					lex_next_char (reader);
				}
			}
			break;
		case '>':
			if (reader->current == '=') {
				token->type = TOKEN_LAEQ;
				lex_next_char (reader);
			}
			else if (reader->current == '>') {
				token->type = TOKEN_RSHFT;
				lex_next_char (reader);
				if (reader->current == '=') {
					token->type = TOKEN_IPRS;
					lex_next_char (reader);
				}
			}
			break;
		case '+':
			if (reader->current == '+') {
				token->type = TOKEN_SADD;
				lex_next_char (reader);
			}
			else if (reader->current == '=') {
				token->type = TOKEN_IPADD;
				lex_next_char (reader);
			}
			break;
		case '-':
			if (reader->current == '-') {
				token->type = TOKEN_SSUB;
				lex_next_char (reader);
			}
			else if (reader->current == '=') {
				token->type = TOKEN_IPADD;
				lex_next_char (reader);
			}
			break;
		case '*':
			if (reader->current == '=') {
				token->type = TOKEN_IPMUL;
				lex_next_char (reader);
			}
			break;
		case '/':
			if (reader->current == '=') {
				token->type = TOKEN_IPDIV;
				lex_next_char (reader);
			}
			break;
		case '%':
			if (reader->current == '=') {
				token->type = TOKEN_IPMOD;
				lex_next_char (reader);
			}
			break;
		case '^':
			if (reader->current == '=') {
				token->type = TOKEN_IPXOR;
				lex_next_char (reader);
			}
			break;
	}

	return token;
}

static void
lex_token_buf_expand (token_t *token)
{
	size_t req;
	char *new_buf;

	req = token->allocated + TOKEN_LEN_STEP;

	/* Overflowed? That's crazy! */
	if (req < TOKEN_LEN_STEP) {
		fatal_error ("token length overflowed, are you crazy?");
	}

	new_buf = (char *) pool_calloc (req, sizeof (char));
	if (new_buf == NULL) {
		fatal_error ("out of memory.");
	}

	memcpy ((void *) new_buf, (void *) token->token, token->len);

	pool_free ((void *) token->token);
	token->token = new_buf;
}

static void
lex_save_char (reader_t *reader, token_t *token, char c, int next)
{
	if (token->len >= token->allocated - 1) {
		lex_token_buf_expand (token);
	}

	token->token[token->len++] = c == -1? reader->current: c;

	if (next) {
		lex_next_char (reader);
	}
}

static char
lex_hex_2_char (char c)
{
	if (!LEX_IS_XDIGIT (c)) {
		return '\0';
	}

	switch (c) {
		case 'a':
		case 'A':
			return '\xa';
		case 'b':
		case 'B':
			return '\xb';
		case 'c':
		case 'C':
			return '\xc';
		case 'd':
		case 'D':
			return '\xd';
		case 'e':
		case 'E':
			return '\xe';
		case 'f':
		case 'F':
			return '\xf';
		default:
			return c - '0';
	}
}

static int
lex_read_hexadecimal_char (reader_t *reader, token_t *token)
{
	char c;

	lex_next_char (reader);
	if (!LEX_IS_XDIGIT (reader->current)) {
		return 0;
	}

	c = lex_hex_2_char (reader->current);
	lex_next_char (reader);
	if (!LEX_IS_XDIGIT (reader->current)) {
		lex_save_char (reader, token, c, 0);

		return 1;
	}
	c = (c << 4) + lex_hex_2_char (reader->current);
	lex_save_char (reader, token, c, 1);

	return 1;
}

static void
lex_read_octal_char (reader_t *reader, token_t *token)
{
	char c;

	c = reader->current - '0';
	/* The digits in an octal sequence is up to 3. */
	lex_next_char (reader);
	for (int i = 0; i < 2; i++) {
		if (!LEX_IS_ODIGIT (reader->current)) {
			lex_save_char (reader, token, c, 0);

			return;
		}
		c = c * 8 + reader->current - '0';
		lex_next_char (reader);
	}
	lex_save_char (reader, token, c, 1);
}

static int 
lex_read_escaped_char (reader_t *reader, token_t *token)
{
	lex_next_char (reader);
	switch (reader->current) {
		case 'a':
			lex_save_char (reader, token, '\a', 1);
			break;
		case 'b':
			lex_save_char (reader, token, '\b', 1);
			break;
		case 'f':
			lex_save_char (reader, token, '\f', 1);
			break;
		case 'n':
			lex_save_char (reader, token, '\n', 1);
			break;
		case 'r':
			lex_save_char (reader, token, '\r', 1);
			break;
		case 't':
			lex_save_char (reader, token, '\t', 1);
			break;
		case 'v':
			lex_save_char (reader, token, '\v', 1);
			break;
		case '\\':
			lex_save_char (reader, token, '\\', 1);
			break;
		case '\'':
			lex_save_char (reader, token, '\'', 1);
			break;
		case '\"':
			lex_save_char (reader, token, '\"', 1);
			break;
		case 'x':
			/* A hexadecimal char. */
			if (!lex_read_hexadecimal_char (reader, token)) {
				error ("%s:%d: invalid hexadecimal char sequence.",
					reader->path, reader->line);

				return 0;
			}
			break;
		default:
			/* Octal char? */
			if (LEX_IS_ODIGIT (reader->current)) {
				lex_read_octal_char (reader, token);
				break;
			}

			error ("%s:%d: unknown escape sequence: \\%c.",
				reader->path, reader->line, reader->current);

			return 0;
	}

	return 1;
}

static token_t *
lex_read_char (reader_t *reader, token_t *token)
{
	lex_next_char (reader);
	if (reader->current == '\\') {
		/* Ok, we meet an escape sequence. */
		if (!lex_read_escaped_char (reader, token)) {
			lex_token_free (token);

			return NULL;
		}
	}

	if (reader->current != '\'') {
		error ("%s:%d: multiple chars in char literal.",
			reader->path, reader->line);
		lex_token_free (token);

		return NULL;
	}

	lex_next_char (reader);

	token->type = TOKEN_CHARACTER;

	return token;
}

static token_t *
lex_read_str (reader_t *reader, token_t *token)
{
	lex_next_char (reader);	
	for (;;) {
		if (reader->current == '\\') {
			lex_read_escaped_char (reader, token);
		}
		else if (reader->current == '\"') {
			/* That's the end. */
			lex_next_char (reader);
			break;
		}
		else if (reader->current == EOF) {
			error ("%s:%d: missing matching '\"'.",
				reader->path, reader->line);
			lex_token_free (token);

			return NULL;
		}
	}

	token->type = TOKEN_STRING;

	return token;
}

static token_t *
lex_read_numberical (reader_t *reader, token_t *token)
{
	return NULL;
}

token_t *
lex_next (reader_t *reader)
{
	token_t *token;

	token = lex_token_new ();

	for (;;) {
		switch (reader->current) {
			case '\r':
			case '\n':
				lex_new_line (reader, reader->current);
				break;
			case '|':
			case '&':
			case '=':
			case '!':
			case '<':
			case '>':
			case '+':
			case '-':
			case '*':
			case '/':
			case '%':
			case '^':
				return lex_check_one_ahead (reader, token);
			case '\'':
				return lex_read_char (reader, token);
			case '\"':
				return lex_read_str (reader, token);
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
				return lex_read_numberical (reder, token);
			default:
				break;
		}
	}

	return token;
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
