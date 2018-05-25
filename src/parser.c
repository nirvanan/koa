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
#include <stdlib.h>
#include <limits.h>

#include "parser.h"
#include "pool.h"
#include "lex.h"
#include "code.h"
#include "object.h"
#include "str.h"
#include "vec.h"
#include "error.h"
#include "nullobject.h"
#include "boolobject.h"
#include "charobject.h"
#include "intobject.h"
#include "longobject.h"
#include "doubleobject.h"
#include "strobject.h"

#define TOP_LEVEL_TAG "#"

#define TOKEN_IS_TYPE(x) (TOKEN_TYPE((x))==TOKEN_VOID||\
	TOKEN_TYPE((x))==TOKEN_NULL||\
	TOKEN_TYPE((x))==TOKEN_BOOL||\
	TOKEN_TYPE((x))==TOKEN_CHAR||\
	TOKEN_TYPE((x))==TOKEN_INT||\
	TOKEN_TYPE((x))==TOKEN_LONG||\
	TOKEN_TYPE((x))==TOKEN_FLOAT||\
	TOKEN_TYPE((x))==TOKEN_DOUBLE||\
	TOKEN_TYPE((x))==TOKEN_STR||\
	TOKEN_TYPE((x))==TOKEN_VEC||\
	TOKEN_TYPE((x))==TOKEN_DICT||\
	TOKEN_TYPE((x))==TOKEN_FUNC)

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

static int parser_cast_expression (parser_t *parser, code_t *code);
static int parser_assignment_expression (parser_t *parser, code_t *code);

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
parser_syntax_error (parser_t *parser, const char *err)
{
	error ("syntax error: %s:%d: %s",
		parser->path, TOKEN_LINE (parser->token), err);
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
		case TOKEN_VOID:
			return OBJECT_TYPE_VOID;
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

static int
parser_push_const (code_t *code, object_type_t type, object_t *obj)
{
	object_t *const_obj;
	int const_exist;
	para_t const_pos;

	const_exist = 0;
	const_obj = obj;
	if (const_obj == NULL) {
		const_obj = object_get_default (type);
	}
	if (const_obj == NULL ||
		(const_pos = code_push_const (code, const_obj, &const_exist)) == -1) {
		return (para_t) -1;
	}

	if (const_exist) {
		object_free (const_obj);
	}
	else {
		object_ref (const_obj);
	}

	return const_pos;
}

static op_t
parser_get_unary_op (parser_t *parser)
{
	token_type_t type;

	type = TOKEN_TYPE (parser->token);
	if (type == TOKEN ('-')) {
		return OP_VALUE_NEG;
	}
	else if (type == TOKEN ('~')) {
		return OP_BIT_NOT;
	}
	else if (type == TOKEN ('!')) {
		return OP_LOGIC_NOT;
	}

	return (op_t) 0;
}

/* expression-postfix-list:
 * expression-postfix expression-postfix-list. */
static int
parser_expression_postfix_list (parser_t *parser, code_t *code)
{
	return 1;
}

/* expression:
 * assignment-expression
 * assignment-expression , expression */
static int 
parser_expression (parser_t *parser, code_t *code)
{
	if (!parser_assignment_expression (parser, code)) {
		return 0;
	}

	while (parser_check (parser, TOKEN (','))) {
		uint32_t line;

		line = TOKEN_LINE (parser->token);
		/* Emit a POP_STACK to ignore left part. */
		if (!code_push_opcode (code, OPCODE (OP_POP_STACK, 0), line)) {
			return 0;
		}

		if (!parser_assignment_expression (parser, code)) {
			return 0;
		}
	}

	return 1;
}

/* primary-expression:
 * identifier
 * constant
 * string-literal
 * ( expression ) */
static int
parser_primary_expression (parser_t *parser, code_t *code, int leading_par)
{
	para_t pos;
	uint32_t line;
	
	if (leading_par) {
		if (!parser_expression (parser, code)) {
			return 0;
		}
		/* Skip ')'. */
		if (!parser_check (parser, TOKEN (')'))) {
			parser_syntax_error (parser, "missing matching ')'.");

			return 0;
		}
		parser_next_token (parser);

		return 1;
	}

	line = TOKEN_LINE (parser->token);
	switch (TOKEN_TYPE (parser->token)) {
		case TOKEN_IDENTIFIER:
			pos = code_push_varname (code,TOKEN_ID (parser->token), 0);	
			if (pos == -1) {
				return 0;
			}
			/* Emit a LOAD_VAR. */
			return code_push_opcode (code, OPCODE (OP_LOAD_VAR, pos), line);
		case TOKEN_NULL:
			pos = parser_push_const (code,
				OBJECT_TYPE_CHAR, nullobject_new (NULL));
			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		case TOKEN_TRUE:
		case TOKEN_FALSE:
			pos = parser_push_const (code,
				OBJECT_TYPE_CHAR,
				boolobject_new (TOKEN_TYPE (parser->token) == TOKEN_TRUE, NULL));
			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		case TOKEN_INTEGER:
		case TOKEN_HEXINT:
		case TOKEN_LINTEGER: {
			long val;

			val = strtol (TOKEN_ID (parser->token), NULL, 0);
			if (val < INT_MAX || val < INT_MIN ||
				parser_check (parser, TOKEN_LINTEGER)) {
				pos = parser_push_const (code,
					OBJECT_TYPE_LONG, longobject_new (val, NULL));
			}
			else {
				pos = parser_push_const (code,
					OBJECT_TYPE_INT, intobject_new ((int) val, NULL));
			}
			if (pos == -1) {
				return 0;
			}

			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		}
		case TOKEN_FLOATING:
		case TOKEN_EXPO: {
			double val;

			val = strtod (TOKEN_ID (parser->token), NULL);
			pos = parser_push_const (code,
				OBJECT_TYPE_DOUBLE, doubleobject_new (val, NULL));

			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		}
		case TOKEN_CHARACTER: {
			char val;

			val = (char) *TOKEN_ID (parser->token);
			pos = parser_push_const (code,
				OBJECT_TYPE_CHAR, charobject_new (val, NULL));

			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		}
		case TOKEN_STRING:
			pos = parser_push_const (code,
				OBJECT_TYPE_CHAR, strobject_new (TOKEN_ID (parser->token), NULL));
			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		default:
			if (parser_check (parser, TOKEN ('('))) {
				parser_next_token (parser);
				if (!parser_expression (parser, code)) {
					return 0;
				}
				/* Skip ')'. */
				if (!parser_check (parser, TOKEN (')'))) {
					parser_syntax_error (parser, "missing matching ')'.");

					return 0;
				}

				return 1;
			}
			break;
	}

	return 0;
}

/* postfix-expression:
 * primary-expression
 * primary-expression expression-postfix-list */
static int
parser_postfix_expression (parser_t *parser, code_t *code, int leading_par)
{
	if (!parser_primary_expression (parser, code, leading_par)) {
		return 0;
	}
	if (!parser_expression_postfix_list (parser, code)) {
		return 0;
	}

	return 1;
}

/* unary-expression:
 * postfix-expression
 * ++ unary-expression
 * -- unary-expression
 * unary-operator cast-expression */
static int
parser_unary_expression (parser_t *parser, code_t *code, int leading_par)
{
	token_type_t type;

	uint32_t line;

	if (leading_par) {
		/* Just fall into a postfix-expression. */
		return parser_postfix_expression (parser, code, leading_par);
	}

	type = TOKEN_TYPE (parser->token);
	line = TOKEN_LINE (parser->token);
	switch (TOKEN_TYPE (parser->token)) {
		case TOKEN_SADD:
		case TOKEN_SSUB:
			parser_next_token (parser);
			if (!parser_unary_expression (parser, code, 0)) {
				return 0;
			}
			/* The last opcode must be a var loading code. */
			return code_last_var_modify (code,
				TOKEN_TYPE (parser->token) == TOKEN_SADD, line);
		default:
			if (type == TOKEN ('+')) {
				/* Just skip '+'. */
				parser_next_token (parser);

				return parser_cast_expression (parser, code);
			}
			if (type == TOKEN ('-') || type == TOKEN ('~') || type == TOKEN ('!')) {
				parser_next_token (parser);
				if (!parser_cast_expression (parser, code)) {
					return 0;
				}

				/* Emit an unary operation. */
				return code_push_opcode (code,
					OPCODE (parser_get_unary_op (parser), 0), line);	
			}
			break;
	}

	return parser_postfix_expression (parser, code, 0);
}

/* cast-expression:
 * unary-expression
 * ( type-specifier ) cast-expression */
static int
parser_cast_expression (parser_t *parser, code_t *code)
{
	if (parser_check (parser, TOKEN ('('))) {
		object_type_t type;
		uint32_t line;

		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		type = parser_token_object_type (parser);
		/* Oops, it's an unary-expression, and we skipped its leading parenthese. */
		if (type == -1) {
			return parser_unary_expression (parser, code, 1);
		}
		else if (type == OBJECT_TYPE_VOID) {
			parser_syntax_error (parser, "can not cast any type to void.");

			return 0;
		}
		
		/* Skip ')'. */
		if (parser_check (parser, TOKEN (')'))) {
			parser_syntax_error (parser, "missing matching ')'.");

			return 0;
		}
		parser_next_token (parser);

		/* Recursion needed. */
		if (!parser_cast_expression (parser, code)) {
			return 0;
		}

		/* Emit a TYPE_CAST opcode. */
		if (!code_push_opcode (code, OPCODE (OP_TYPE_CAST, (para_t) type), line)) {
			return 0;
		}
	}

	return parser_unary_expression (parser, code, 0);
}

/* assignment-expression:
 * conditional-expression
 * unary-expression assignment-operator assignment-expression */
static int
parser_assignment_expression (parser_t *parser, code_t *code)
{
	/* The first part is always a cast-expression. */
	if (!parser_cast_expression (parser, code)) {
		return 0;
	}
	return 1;
}

/* init-declarator:
 * identifier
 * identifier = assignment-expression */
static int
parser_init_declarator (parser_t *parser, code_t *code,
						object_type_t type, const char *id)
{
	para_t var_pos;
	const char *var;
	uint32_t line;

	line = TOKEN_LINE (parser->token);
	var = id;
	if (var == NULL) {
		if (!parser_check (parser, TOKEN_IDENTIFIER)) {
			parser_syntax_error (parser, "missing identifier name.");

			return 0;
		}

		var = TOKEN_ID (parser->token);
		parser_next_token (parser);
	}

	var_pos = code_push_varname (code, var, 0);
	if (var_pos == -1) {
		return 0;
	}

	if (parser_check (parser, TOKEN ('='))) {
		if (!parser_assignment_expression (parser, code)) {
			return 0;
		}
	}
	else {
		para_t const_pos;

		const_pos = parser_push_const (code, type, NULL);
		if (const_pos == -1) {
			return 0;
		}

		/* Emit a LOAD_CONST opcpde. */
		if (!code_push_opcode (code, OPCODE (OP_LOAD_CONST, const_pos), line)) {
			return 0;
		}
	}

	/* Emit a STORE_LOCAL opcode. */
	if (!code_push_opcode (code, OPCODE (OP_STORE_LOCAL, var_pos), line)) {
		return 0;
	}

	return 1;
}

/* init-declarator-list:
 * init-declarator
 * init-declarator , init-declarator-list */
static int
parser_init_declarator_list (parser_t *parser, code_t *code,
							 object_type_t type, const char *first_id)
{
	if (!parser_init_declarator (parser, code, type, first_id)) {
		return 0;
	}

	while (parser_check (parser, TOKEN (','))) {
		parser_next_token (parser);
		if (!parser_init_declarator (parser, code, type, NULL)) {
			return 0;
		}
	}

	return 1;
}

/* declaration:
 * type-specifier init-declarator-list ; */
static int
parser_declaration (parser_t *parser, code_t *code,
					object_type_t type, const char *first_id)
{
	/* Skip type-specifier. */
	parser_next_token (parser);

	if (!parser_init_declarator_list (parser, code, type, first_id)) {
		return 0;
	}

	if (!parser_check (parser, TOKEN (';'))) {
		parser_syntax_error (parser, "missing ';'.");

		return 0;
	}

	return 1;
}

/* block-item:
 * declaration
 * statement */
static int
parser_block_item (parser_t *parser, code_t *code)
{
	if (TOKEN_IS_TYPE (parser->token)) {
		/* declaration. */
		object_type_t type;

		type = parser_token_object_type (parser);
		if (type == OBJECT_TYPE_VOID) {
			parser_syntax_error (parser, "variable can not be void.");

			return 0;
		}

		if (!parser_declaration (parser, code, type, NULL)) {
			return 0;
		}
	}
	else {
	}

	return 1;
}

/* block-item-list:
 * block-item
 * block-item block-item-list */
static int
parser_block_item_list (parser_t *parser, code_t *code)
{
	while (!parser_check (parser, TOKEN_END) &&
		   !parser_check (parser, TOKEN ('}'))) {
		if (!parser_block_item (parser, code)) {
			return 0;
		}
	}

	return 1;
}

/* compound-statement:
 * { block-item-listopt } */
static int
parser_compound_statement (parser_t *parser, code_t *code)
{
	/* Skip '{'. */
	parser_next_token (parser);
	
	/* Check empty function body. */
	if (!parser_check (parser, TOKEN ('}'))) {
		if (!parser_block_item_list (parser, code)) {
			return 0;
		}
	}

	/* Skip '}'. */
	if (!parser_check (parser, TOKEN ('}'))) {
		parser_syntax_error (parser,
			"missing matching '}' for function body.");

		return 0;
	}
	parser_next_token (parser);

	return 1;
}

/* parameter-declaration:
 * type-specifier identifier */
static int
parser_parameter_declaration (parser_t *parser, code_t *code)
{
	object_type_t type;
	para_t var_pos;
	para_t const_pos;
	uint32_t line;

	type = parser_token_object_type (parser);
	if (type == -1) {
		parser_syntax_error (parser, "unknown parameter type.");

		return 0;
	}
	else if (type == OBJECT_TYPE_VOID) {
		parser_syntax_error (parser, "parameter can not be a void.");

		return 0;
	}

	parser_next_token (parser);
	if (!parser_check (parser, TOKEN_IDENTIFIER)) {
		parser_syntax_error (parser, "missing identifier name.");

		return 0;
	}
	
	/* Insert parameter local var and const. */
	var_pos = code_push_varname (code, TOKEN_ID (parser->token), 1);
	if (var_pos == -1) {
		return 0;
	}
	const_pos = parser_push_const (code, type, NULL);
	if (const_pos == -1) {
		return 0;
	}

	/* Make opcodes and insert them. */
	line = TOKEN_LINE (parser->token);
	if (!code_push_opcode (code, OPCODE (OP_LOAD_CONST, const_pos), line) ||
		!code_push_opcode (code, OPCODE (OP_STORE_LOCAL, var_pos), line)) {
		return 0;
	}

	return 1;
}

/* parameter-list:
 * parameter-declaration
 * parameter-declaration, parameter-list */
static int
parser_parameter_list (parser_t *parser, code_t *code)
{
	/* First parameter. */
	if (!parser_parameter_declaration (parser, code)) {
		return 0;
	}

	while (parser_check (parser, TOKEN (','))) {
		parser_next_token (parser);
		if (!parser_parameter_declaration (parser, code)) {
			return 0;
		}
	}

	return 1;
}

/* function-definition:
 * type-specifier(*) identifier(*) ( parameter-listopt ) */
static int
parser_function_definition (parser_t *parser, code_t *code,
	object_type_t ret_type, para_t id_pos, const char *fun)
{
	code_t *fun_code;

	/* Make a new code for this function. */
	fun_code = code_new (parser->path, fun);
	if (fun_code == NULL) {
		return 0;
	}

	/* Skip '('. */
	parser_next_token (parser);

	/* Has parameter? */
	if (!parser_check (parser, TOKEN (')'))) {
		if (!parser_parameter_list (parser, fun_code)) {
			return 0;
		}
	}

	if (!parser_check (parser, TOKEN (')'))) {
		parser_syntax_error (parser, "missing matching ')'.");

		return 0;
	}
	parser_next_token (parser);

	if (!parser_check (parser, TOKEN ('{'))) {
		parser_syntax_error (parser, "missing '{' in function definition.");

		return 0;
	}
	if (!parser_compound_statement (parser, fun_code))
	{
		return 0;
	}

	return 1;
}

/* external-declaration:
 * function-definition
 * declaration */
static int
parser_external_declaration (parser_t *parser, code_t *code)
{
	/* Need to look ahead 3 tokens: type id '('*/
	object_type_t type;
	para_t pos;
	const char *fun;

	type = parser_token_object_type (parser);
	if (type == -1) {
		parser_syntax_error (parser, "unknown variable type or return value type.");

		return 0;
	}

	parser_next_token (parser);
	if (!parser_check (parser, TOKEN_IDENTIFIER)) {
		parser_syntax_error (parser, "missing identifier name.");

		return 0;
	}

	fun = TOKEN_ID (parser->token);
	pos = code_push_varname (code, fun, 0);
	if (pos == -1) {
		return 0;
	}

	parser_next_token (parser);
	if (parser_check (parser, TOKEN ('('))) {
		return parser_function_definition (parser, code, type, pos, fun);
	}

	if (type == OBJECT_TYPE_VOID) {
		parser_syntax_error (parser, "variable can not be a void.");

		return 0;
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

	code = code_new (path, TOP_LEVEL_TAG);
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

	code = code_new (path, TOP_LEVEL_TAG);
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
