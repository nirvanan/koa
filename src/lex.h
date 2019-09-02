/*
 * lex.h
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

#ifndef LEX_H
#define LEX_H

#include <stdint.h>

#include "koa.h"
#include "object.h"

#define TOKEN_MIN 257
#define TOKEN_LEN_STEP 200 /* Step size for token expansion. */
#define LOADED_BUF_SIZE 20

#define TOKEN_TYPE(x) ((x)?(x)->type:TOKEN_BROKEN)
#define TOKEN_ID(x) ((x)?(x)->token:"")
#define TOKEN_LINE(x) ((x)?(x)->lineno:0)
#define TOKEN(x) ((token_type_t)(x))

#define TOKEN_IS_TYPE(x) (TOKEN_TYPE((x))==TOKEN_VOID||\
	TOKEN_TYPE((x))==TOKEN_NULL||\
	TOKEN_TYPE((x))==TOKEN_BOOL||\
	TOKEN_TYPE((x))==TOKEN_CHAR||\
	TOKEN_TYPE((x))==TOKEN_INT||\
	TOKEN_TYPE((x))==TOKEN_LONG||\
	TOKEN_TYPE((x))==TOKEN_INT8||\
	TOKEN_TYPE((x))==TOKEN_UINT8||\
	TOKEN_TYPE((x))==TOKEN_INT16||\
	TOKEN_TYPE((x))==TOKEN_UINT16||\
	TOKEN_TYPE((x))==TOKEN_INT32||\
	TOKEN_TYPE((x))==TOKEN_UINT32||\
	TOKEN_TYPE((x))==TOKEN_INT64||\
	TOKEN_TYPE((x))==TOKEN_UINT64||\
	TOKEN_TYPE((x))==TOKEN_FLOAT||\
	TOKEN_TYPE((x))==TOKEN_DOUBLE||\
	TOKEN_TYPE((x))==TOKEN_STR||\
	TOKEN_TYPE((x))==TOKEN_VEC||\
	TOKEN_TYPE((x))==TOKEN_DICT||\
	TOKEN_TYPE((x))==TOKEN_FUNC)

#define TOKEN_IS_CON(x) (TOKEN_TYPE((x))==TOKEN('?')||\
	TOKEN_TYPE((x))==TOKEN_LOR||\
	TOKEN_TYPE((x))==TOKEN_LAND||\
	TOKEN_TYPE((x))==TOKEN('|')||\
	TOKEN_TYPE((x))==TOKEN('^')||\
	TOKEN_TYPE((x))==TOKEN('&')||\
	TOKEN_TYPE((x))==TOKEN_EQ||\
	TOKEN_TYPE((x))==TOKEN_NEQ||\
	TOKEN_TYPE((x))==TOKEN('<')||\
	TOKEN_TYPE((x))==TOKEN('>')||\
	TOKEN_TYPE((x))==TOKEN_LEEQ||\
	TOKEN_TYPE((x))==TOKEN_LAEQ||\
	TOKEN_TYPE((x))==TOKEN_LSHFT||\
	TOKEN_TYPE((x))==TOKEN_RSHFT||\
	TOKEN_TYPE((x))==TOKEN('+')||\
	TOKEN_TYPE((x))==TOKEN('-')||\
	TOKEN_TYPE((x))==TOKEN('*')||\
	TOKEN_TYPE((x))==TOKEN('/')||\
	TOKEN_TYPE((x))==TOKEN('%'))

#define TOKEN_IS_ASSIGN(x) (TOKEN_TYPE((x))==TOKEN('=')||\
	TOKEN_TYPE((x))==TOKEN_IPMUL||\
	TOKEN_TYPE((x))==TOKEN_IPDIV||\
	TOKEN_TYPE((x))==TOKEN_IPMOD||\
	TOKEN_TYPE((x))==TOKEN_IPADD||\
	TOKEN_TYPE((x))==TOKEN_IPSUB||\
	TOKEN_TYPE((x))==TOKEN_IPLS||\
	TOKEN_TYPE((x))==TOKEN_IPRS||\
	TOKEN_TYPE((x))==TOKEN_IPAND||\
	TOKEN_TYPE((x))==TOKEN_IPXOR||\
	TOKEN_TYPE((x))==TOKEN_IPOR)

/* Reserved single-char tokens stand for themselves, such as '+', '^', '?'.
 * Those are not listed here. */
typedef enum token_type_e
{
	TOKEN_BROKEN = -2, /* Broken token stream. */
	TOKEN_END = -1, /* End mark of token stream. */
	TOKEN_UNKNOWN = 0, /* Unknown token. */
	TOKEN_STATIC = TOKEN_MIN, /* Storage-class-specifier: static. */
	TOKEN_VOID, /* Type-specifier: void. */
	TOKEN_NULL, /* Type-specifier: null. */
	TOKEN_BOOL, /* Type-specifier: bool. */
	TOKEN_CHAR, /* Type-specifier: char. */
	TOKEN_INT, /* Type-specifier: int. */
	TOKEN_LONG, /* Type-specifier: long. */
	TOKEN_INT8, /* Type-specifier: int8. */
	TOKEN_UINT8, /* Type-specifier: uint8. */
	TOKEN_INT16, /* Type-specifier: int16. */
	TOKEN_UINT16, /* Type-specifier: uint16. */
	TOKEN_INT32, /* Type-specifier: int32. */
	TOKEN_UINT32, /* Type-specifier: uint32. */
	TOKEN_INT64, /* Type-specifier: int64. */
	TOKEN_UINT64, /* Type-specifier: uint64. */
	TOKEN_FLOAT, /* Type-specifier: float. */
	TOKEN_DOUBLE, /* Type-specifier: double. */
	TOKEN_STR, /* Type-specifier: str. */
	TOKEN_VEC, /* Type-specifier: vec. */
	TOKEN_DICT, /* Type-specifier: dict. */
	TOKEN_FUNC, /* Type-specifier: func. */
	TOKEN_EXCEPTION, /* Type-specifier: exception. */
	TOKEN_LOR, /* Logic-or-expression: ||. */
	TOKEN_LAND, /* Logic-and-expression: &&. */
	TOKEN_EQ, /* Equality-expression: ==. */
	TOKEN_NEQ, /* Equality-expression: !=. */
	TOKEN_LEEQ, /* Relatioal-expression: <=. */
	TOKEN_LAEQ, /* Relatioal-expression: >=. */
	TOKEN_LSHFT, /* Shift-expression: <<. */
	TOKEN_RSHFT, /* Shift-expression: >>. */
	TOKEN_INC, /* Unary-expression: ++. */
	TOKEN_DEC, /* Unary-expression: --. */
	TOKEN_IPMUL, /* Assignment-operator: *=. */
	TOKEN_IPDIV, /* Assignment-operator: /=. */
	TOKEN_IPMOD, /* Assignment-operator: %=. */
	TOKEN_IPADD, /* Assignment-operator: +=. */
	TOKEN_IPSUB, /* Assignment-operator: -=. */
	TOKEN_IPLS, /* Assignment-operator: <<=. */
	TOKEN_IPRS, /* Assignment-operator: >>=. */
	TOKEN_IPAND, /* Assignment-operator: &=. */
	TOKEN_IPXOR, /* Assignment-operator: ^=. */
	TOKEN_IPOR, /* Assignment-operator: |=. */
	TOKEN_CASE, /* Labeled-statement: case. */
	TOKEN_DEFAULT, /* Labeled-statement: default. */
	TOKEN_IF, /* Selection-statement: if. */
	TOKEN_ELSE, /* Selection-statement: else. */
	TOKEN_SWITCH, /* Selection-statement: switch. */
	TOKEN_WHILE, /* Iteration-statement: while. */
	TOKEN_DO, /* Iteration-statement: do. */
	TOKEN_FOR, /* Iteration-statement: for. */
	TOKEN_CONTINUE, /* Jump-statement: continue. */
	TOKEN_BREAK, /* Jump-statement: break. */
	TOKEN_RETURN, /* Jump-statement: return. */
	TOKEN_TRY, /* Try-statement: try. */
	TOKEN_CATCH, /* Try-statement: catch. */
	TOKEN_TRUE, /* Constant: true. */
	TOKEN_FALSE, /* Constant: false. */
	TOKEN_INTEGER, /* Constant: integer. */
	TOKEN_LINTEGER, /* Constant: long integer. */
	TOKEN_HEXINT, /* Constant: hexadecimal integer. */
	TOKEN_FLOATING, /* Constant: floating. */
	TOKEN_EXPO, /* Constant: exponential floating. */
	TOKEN_CHARACTER, /* Constant: character. */
	TOKEN_IDENTIFIER, /* Primary-expression: identifier. */
	TOKEN_STRING /* Primary-expression: string. */
} token_type_t;

typedef struct token_s
{
	token_type_t type;
	uint32_t lineno;
	size_t len;
	size_t allocated;
	char *token;
} token_t;

typedef char (*get_char_f) (void *udata);

typedef void (*clear_f) (void *udata);

typedef struct reader_s
{
	char current;
	const char *path;
	get_char_f rf;
	clear_f cf;
	void *rd;
	uint32_t line;
	size_t loaded_len;
	size_t next_loaded;
	char loaded[LOADED_BUF_SIZE + 1];
	int broken;
} reader_t;

reader_t *
lex_reader_new (const char *path, get_char_f rf, clear_f cf, void *udata);

void
lex_reader_free (reader_t *reader);

int
lex_reader_broken (reader_t *reader);

void
lex_token_free (token_t *token);

token_t *
lex_next (reader_t *reader);

object_type_t
lex_get_token_object_type (token_t *token);

void
lex_init ();

#endif /* LEX_H */
