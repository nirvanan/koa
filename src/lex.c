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
	{"void", TOKEN_VOID},
	{"null", TOKEN_NULL},
	{"bool", TOKEN_BOOL},
	{"char", TOKEN_CHAR},
	{"int", TOKEN_INT},
	{"long", TOKEN_LONG},
	{"float", TOKEN_FLOAT},
	{"double", TOKEN_DOUBLE},
	{"str", TOKEN_STR},
	{"vec", TOKEN_VEC},
	{"dict", TOKEN_DICT},
	{"func", TOKEN_FUNC},
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

	errno = 0;
	reader->current = reader->rf (reader->rd);
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
		errno = 0;
		reader->loaded[reader->loaded_len++] = reader->rf (reader->rd);
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
lex_reader_new (const char *path, get_char_f rf, clear_f cf, void *udata)
{
	reader_t *reader;

	reader = (reader_t *) pool_calloc (1, sizeof (reader_t));
	if (reader == NULL) {
		error ("out of memory.");

		return NULL;
	}

	reader->path = path;
	reader->rf = rf;
	reader->cf = cf;
	reader->rd = udata;
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
	/* Call users clear rountine. */
	reader->cf (reader->rd);

	pool_free ((void *) reader);
}

static token_t *
lex_token_new (reader_t *reader)
{
	token_t *token;

	token = (token_t *) pool_calloc (1, sizeof (token_t));
	if (token == NULL) {
		fatal_error ("out of memory.");
	}

	token->token = (char *) pool_calloc (
		(size_t) (TOKEN_LEN_STEP + 1), sizeof (char));
	if (token->token == NULL) {
		pool_free ((void *) token);
		fatal_error ("out of memory.");
	}

	token->lineno = reader->line;
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
lex_new_line (reader_t *reader, token_t *token, char prev)
{
	lex_next_char (reader);
	/* Skip "\r\n" and "\n\r" sequences. */
	if ((reader->current == '\r' || reader->current == '\n') &&
		reader->current != prev) {
		lex_next_char (reader);
	}
	token->lineno++;
	reader->line++;
	if (reader->line > MAX_SOURCE_LINE) {
		fatal_error ("source line exceeded.");
	}
}

static void
lex_set_type_and_next (reader_t *reader, token_t *token, token_type_t type)
{
	token->type = type;
	lex_next_char (reader);
}

static token_t *
lex_check_one_ahead (reader_t *reader, token_t *token)
{
	lex_set_type_and_next (reader, token, (token_type_t) reader->current);
	switch ((char) token->type) {
		case '|':
			if (reader->current == '|') {
				lex_set_type_and_next (reader, token, TOKEN_LOR);
			}
			else if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_IPOR);
			}
			break;
		case '&':
			if (reader->current == '&') {
				lex_set_type_and_next (reader, token, TOKEN_LAND);
			}
			else if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_IPAND);
			}
			break;
		case '=':
			if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_EQ);
			}
			break;
		case '!':
			if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_NEQ);
			}
			break;
		case '<':
			if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_LEEQ);
			}
			else if (reader->current == '<') {
				lex_set_type_and_next (reader, token, TOKEN_LSHFT);
				if (reader->current == '=') {
					lex_set_type_and_next (reader, token, TOKEN_IPLS);
				}
			}
			break;
		case '>':
			if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_LAEQ);
			}
			else if (reader->current == '>') {
				lex_set_type_and_next (reader, token, TOKEN_RSHFT);
				if (reader->current == '=') {
					lex_set_type_and_next (reader, token, TOKEN_IPRS);
				}
			}
			break;
		case '+':
			if (reader->current == '+') {
				lex_set_type_and_next (reader, token, TOKEN_INC);
			}
			else if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_IPADD);
			}
			break;
		case '-':
			if (reader->current == '-') {
				lex_set_type_and_next (reader, token, TOKEN_DEC);
			}
			else if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_IPADD);
			}
			break;
		case '*':
			if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_IPMUL);
			}
			break;
		case '/':
			if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_IPDIV);
			}
			break;
		case '%':
			if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_IPMOD);
			}
			break;
		case '^':
			if (reader->current == '=') {
				lex_set_type_and_next (reader, token, TOKEN_IPXOR);
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

static token_t *
lex_token_error (reader_t *reader, token_t *token, const char *err)
{
	error ("lex error: %s:%d: %s", reader->path, reader->line, err);
	lex_token_free (token);

	return NULL;
}

static int
lex_char_to_dec (char c)
{
	if (!LEX_IS_XDIGIT (c)) {
		return '\0';
	}

	switch (c) {
		case 'a':
		case 'A':
			return 0xa;
		case 'b':
		case 'B':
			return 0xb;
		case 'c':
		case 'C':
			return 0xc;
		case 'd':
		case 'D':
			return 0xd;
		case 'e':
		case 'E':
			return 0xe;
		case 'f':
		case 'F':
			return 0xf;
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

	c = lex_char_to_dec (reader->current);
	lex_next_char (reader);
	if (!LEX_IS_XDIGIT (reader->current)) {
		return 1;
	}
	c = (c << 4) + lex_char_to_dec (reader->current);
	lex_save_char (reader, token, c, 1);

	return 1;
}

static int
lex_read_octal_char (reader_t *reader, token_t *token)
{
	int c;

	c = reader->current - '0';
	/* The digits in an octal sequence is up to 3. */
	lex_next_char (reader);
	for (int i = 0; i < 2; i++) {
		if (!LEX_IS_ODIGIT (reader->current)) {
			break;
		}
		c = c * 8 + reader->current - '0';
		lex_next_char (reader);
	}

	if (c > 127) {
		return 0;
	}

	lex_save_char (reader, token, (char) c, 1);

	return 1;
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
				if (!lex_read_octal_char (reader, token)) {
					error ("%s:%d: invalid octal char sequence.",
						   reader->path, reader->line);

					return 0;
				}
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
		return lex_token_error (reader, token,
								"multiple chars in char literal.");
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
			return lex_token_error (reader, token, "missing matching '\"'.");
		}
		else {
			lex_save_char (reader, token, -1, 1);
		}
	}

	token->type = TOKEN_STRING;

	return token;
}

static token_t *
lex_read_decimal_floating (reader_t *reader, token_t *token, int digit_part)
{
	if (reader->current == '.') {
		token->type = TOKEN_FLOATING;
		lex_save_char (reader, token, -1, 1);
		/* Test next for digits. */
		if (!digit_part && !LEX_IS_DIGIT (reader->current)) {
			return lex_token_error (reader, token,
									"invalid floating sequence.");
		}
	}

	for (;;) {
		if (LEX_IS_DIGIT (reader->current)) {
			lex_save_char (reader, token, -1, 1);
		}
		else if (reader->current == 'e' || reader->current == 'E') {
			/* Multiple exponent parts? */
			if (token->type == TOKEN_EXPO) {
				return lex_token_error (reader, token,
										"multiple exponent parts.");
			}

			lex_save_char (reader, token, -1, 1);
			token->type = TOKEN_EXPO;
			/* Read optional sign. */
			if (reader->current == '+' || reader->current == '-') {
				lex_save_char (reader, token, -1, 1);
			}
			/* Test next for digits. */
			if (!LEX_IS_DIGIT (reader->current)) {
				return lex_token_error (reader, token,
					"invalid floating literal sequence.");
			}
		}
		else if (reader->current == 'f' || reader->current == 'F') {
			lex_save_char (reader, token, -1, 1);
			break;
		}
		else if (reader->current == '.') {
			return lex_token_error (reader, token,
				"invalid decimal point in floating sequence.");
		}
		else if (LEX_IS_ALPHA (reader->current)) {
			return lex_token_error (reader, token,
									"invalid floating literal postfix.");
		}
		else {
			break;
		}
	}

	return token;
}

static token_t *
lex_read_hexadecimal_floating (reader_t *reader, token_t *token, int hex_part)
{
	int p;

	if (reader->current == '.') {
		lex_save_char (reader, token, -1, 1);
		/* Test next for xdigits. */
		if (!hex_part && !LEX_IS_XDIGIT (reader->current)) {
			return lex_token_error (reader, token,
									"invalid hexadecimal floating sequence.");
		}
	}

	p = 0;
	for (;;) {
		if (LEX_IS_DIGIT (reader->current)) {
			lex_save_char (reader, token, -1, 1);
		}
		else if (LEX_IS_XDIGIT (reader->current)) {
			if (token->type != TOKEN_HEXINT || p) {
				return lex_token_error (reader, token,
										"invalid hexadecimal floating sequence.");
			}
			lex_save_char (reader, token, -1, 1);
		}
		else if (reader->current == 'p' || reader->current == 'P') {
			lex_save_char (reader, token, -1, 1);
			p = 1;
			/* Read optional sign. */
			if (reader->current == '+' || reader->current == '-') {
				lex_save_char (reader, token, -1, 1);
			}
			/* Test next for digits. */
			if (!LEX_IS_DIGIT (reader->current)) {
				return lex_token_error (reader, token,
										"invalid floating literal sequence.");
			}
		}
		else if (reader->current == 'f' || reader->current == 'F') {
			lex_save_char (reader, token, -1, 1);
			break;
		}
		else if (reader->current == '.') {
			return lex_token_error (reader, token,
									"invalid decimal point in floating sequence.");
		}
		else if (LEX_IS_ALPHA (reader->current)) {
			return lex_token_error (reader, token,
									"invalid floating literal postfix.");
		}
		else {
			break;
		}
	}

	token->type = TOKEN_FLOATING;

	return token;
}

static token_t *
lex_read_numberical (reader_t *reader, token_t *token)
{
	int hex_part;

	hex_part = 0;
	if (reader->current == '.') {
		return lex_read_decimal_floating (reader, token, 0);
	}
	else if (reader->current == '0') {
		token->type = TOKEN_INTEGER;
		lex_save_char (reader, token, -1, 1);
		/* Hexadecimal? */
		if (reader->current == 'x' || reader->current == 'X') {
			token->type = TOKEN_HEXINT;
			lex_save_char (reader, token, -1, 1);
			/* Test next for xdigits. */
			if (!LEX_IS_XDIGIT (reader->current) && reader->current != '.') {
				return lex_token_error (reader, token,
										"invalid hexadecimal sequence.");
			}
			else if (LEX_IS_XDIGIT (reader->current)) {
				hex_part = 1;
			}
		}
	}
	else {
		token->type = TOKEN_INTEGER;
	}

	for (;;) {
		if (LEX_IS_DIGIT (reader->current)) {
			lex_save_char (reader, token, -1, 1);
		}
		else if (reader->current == 'e' || reader->current == 'E') {
			if (token->type == TOKEN_INTEGER) {
				return lex_read_decimal_floating (reader, token, 1);
			}
			else if (token->type == TOKEN_HEXINT){
				lex_save_char (reader, token, -1, 1);
			}
			else {
				return lex_token_error (reader, token,
										"invalid floating exponent.");
			}
		}
		else if (LEX_IS_XDIGIT (reader->current)) {
			if (token->type != TOKEN_HEXINT) {
				return lex_token_error (reader, token,
										"invalid decimal sequence.");
			}
			lex_save_char (reader, token, -1, 1);
		}
		else if (reader->current == '.') {
			if (token->type == TOKEN_HEXINT) {
				return lex_read_hexadecimal_floating (reader, token, hex_part);
			}
			else {
				return lex_read_decimal_floating (reader, token, 1);
			}
		}
		else if (reader->current == 'p' || reader->current == 'P') {
			if (token->type == TOKEN_HEXINT) {
				return lex_read_hexadecimal_floating (reader, token, hex_part);
			}
			else {
				return lex_token_error (reader, token,
					"invalid hexadecimal floating sequence.");
			}
		}
		else if (reader->current == 'l' || reader->current == 'L') {
			token->type = TOKEN_LINTEGER;
			lex_save_char (reader, token, -1, 1);
			break;
		}
		else if (LEX_IS_ALPHA (reader->current)) {
			return lex_token_error (reader, token,
									"invalid integer literal postfix.");
		}
		else {
			break;
		}
	}
	
	return token;
}

static token_t *
lex_end_of_stream (reader_t *reader, token_t *token)
{
	/* Check whether error occurred. */
	if (errno > 0) {
		lex_token_free (token);

		return NULL;
	}
	
	token->type = TOKEN_END;

	return token;
}

static token_t *
lex_read_identifier (reader_t *reader, token_t *token)
{
	object_t *word;
	object_t *res;

	lex_save_char (reader, token, -1, 1);
	
	for (;;) {
		if (LEX_IS_ALNUM (reader->current)) {
			lex_save_char (reader, token, -1, 1);
		}
		else {
			break;
		}
	}

	/* Check whether this token is a reserved word. */
	word = strobject_new (token->token, strlen (token->token), NULL);
	res = object_index (g_reserved_tokens, word);
	if (OBJECT_IS_INT (res)) {
		/* Ok, a reserved word. */
		token->type = (token_type_t) object_get_integer (res);
	}
	else {
		token->type = TOKEN_IDENTIFIER;
	}

	object_free (word);

	return token;
}

token_t *
lex_next (reader_t *reader)
{
	token_t *token;

	token = lex_token_new (reader);

	for (;;) {
		switch (reader->current) {
			case '\r':
			case '\n':
				lex_new_line (reader, token, reader->current);
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
				return lex_read_numberical (reader, token);
			case EOF:
				return lex_end_of_stream (reader, token);
			default:
				if (LEX_IS_ALPHA (reader->current)) {
					return lex_read_identifier (reader, token);
				}
				else if (LEX_IS_SPACE (reader->current)) {
					lex_next_char (reader);
				}
				else {
					lex_set_type_and_next (reader, token,
										   (token_type_t) reader->current);

					return token;
				}
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
		object_t *word;
		object_t *suc;

		word = strobject_new (re->word, strlen (re->word), NULL);
		if (word == NULL) {
			fatal_error ("failed to generate the reserved word dict.");
		}

		/* These words shall never be freed. */
		object_ref (word);

		suc = object_ipindex (g_reserved_tokens, word,
			intobject_new ((int) re->type, NULL));
		if (suc == NULL) {
			fatal_error ("failed to generate the reserved word dict.");
		}

		re++;
	}
}
