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
#include "funcobject.h"

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

typedef struct parser_s
{
	reader_t *reader; /* Token stream source. */
	const char *path; /* Source file path. */
	token_t *token; /* Current token. */
} parser_t;

typedef struct buf_read_s
{
	str_t *buf;
	size_t p;
} buf_read_t;

static int parser_cast_expression (parser_t *parser, code_t *code);
static int parser_assignment_expression (parser_t *parser, code_t *code);
static int parser_expression (parser_t *parser, code_t *code);

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

static int
parser_test_and_next (parser_t *parser, token_type_t need, const char *err)
{
	if (!parser_check (parser, need)) {
		parser_syntax_error (parser, err);

		return 0;
	}

	parser_next_token (parser);

	return 1;
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
parser_get_unary_op (token_type_t type)
{
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

static op_t
parser_get_equality_op (parser_t *parser)
{
	if (parser_check (parser, TOKEN_EQ)) {
		return OP_EQUAL;
	}
	else if (parser_check (parser, TOKEN_NEQ)) {
		return OP_NOT_EQUAL;
	}

	return (op_t) 0;
}

static op_t
parser_get_relational_op (parser_t *parser)
{
	if (parser_check (parser, TOKEN ('<'))) {
		return OP_LESS_THAN;
	}
	else if (parser_check (parser, TOKEN ('>'))) {
		return OP_LARGE_THAN;
	}
	else if (parser_check (parser, TOKEN_LEEQ)) {
		return OP_LESS_EQUAL;
	}
	else if (parser_check (parser, TOKEN_LAEQ)) {
		return OP_LARGE_EQUAL;
	}

	return (op_t) 0;
}

static op_t
parser_get_shift_op (parser_t *parser)
{
	if (parser_check (parser, TOKEN_LSHFT)) {
		return OP_LEFT_SHIFT;
	}
	else if (parser_check (parser, TOKEN_RSHFT)) {
		return OP_RIGHT_SHIFT;
	}

	return (op_t) 0;
}

static op_t
parser_get_additive_op (parser_t *parser)
{
	if (parser_check (parser, TOKEN ('+'))) {
		return OP_ADD;
	}
	else if (parser_check (parser, TOKEN ('-'))) {
		return OP_SUB;
	}

	return (op_t) 0;
}

static op_t
parser_get_multiplicative_op (parser_t *parser)
{
	if (parser_check (parser, TOKEN ('*'))) {
		return OP_MUL;
	}
	else if (parser_check (parser, TOKEN ('/'))) {
		return OP_DIV;
	}
	else if (parser_check (parser, TOKEN ('%'))) {
		return OP_MOD;
	}

	return (op_t) 0;
}

static op_t
parser_get_var_assign_op (token_type_t type)
{
	if (type == TOKEN ('=')) {
		return OP_STORE_VAR;
	}

	switch (type) {
		case TOKEN_IPMUL:
			return OP_VAR_IPMUL;
		case TOKEN_IPDIV:
			return OP_VAR_IPDIV;
		case TOKEN_IPMOD:
			return OP_VAR_IPMOD;
		case TOKEN_IPADD:
			return OP_VAR_IPADD;
		case TOKEN_IPSUB:
			return OP_VAR_IPSUB;
		case TOKEN_IPLS:
			return OP_VAR_IPLS;
		case TOKEN_IPRS:
			return OP_VAR_IPRS;
		case TOKEN_IPAND:
			return OP_VAR_IPAND;
		case TOKEN_IPXOR:
			return OP_VAR_IPXOR;
		case TOKEN_IPOR:
			return OP_VAR_IPOR;
		default:
			break;
	}

	return (op_t) 0;
}

static op_t
parser_get_index_assign_op (token_type_t type)
{
	if (type == TOKEN ('=')) {
		return OP_STORE_INDEX;
	}

	switch (type) {
		case TOKEN_IPMUL:
			return OP_INDEX_IPMUL;
		case TOKEN_IPDIV:
			return OP_INDEX_IPDIV;
		case TOKEN_IPMOD:
			return OP_INDEX_IPMOD;
		case TOKEN_IPADD:
			return OP_INDEX_IPADD;
		case TOKEN_IPSUB:
			return OP_INDEX_IPSUB;
		case TOKEN_IPLS:
			return OP_INDEX_IPLS;
		case TOKEN_IPRS:
			return OP_INDEX_IPRS;
		case TOKEN_IPAND:
			return OP_INDEX_IPAND;
		case TOKEN_IPXOR:
			return OP_INDEX_IPXOR;
		case TOKEN_IPOR:
			return OP_INDEX_IPOR;
		default:
			break;
	}

	return (op_t) 0;
}

/* labeled-statement:
 * case conditional-expression : statement
 * default : statement */
static int
parser_labeled_statement (parser_t *parser, code_t *code)
{
	return 1;
}

/* expression-statement:
 * expressionopt ; */
static int
parser_expression_statement (parser_t *parser, code_t *code)
{
	if (parser_check (parser, TOKEN (';'))) {
		/* Empty statement, do nothing. */
		parser_next_token (parser);

		return 1;
	}

	if (!parser_expression (parser, code)) {
		return 0;
	}

	/* Skip ';'. */
	return parser_test_and_next (parser, TOKEN (';'),
		"missing ';' in the end of the statement.");
}

/* selection-statement:
 * if ( expression ) statement
 * if ( expression ) statement else statement
 * switch ( expression ) statement */
static int
parser_selection_statement (parser_t *parser, code_t *code)
{
	uint32_t line;

	line = TOKEN_LINE (parser->token);
	if (parser_check (parser, TOKEN_IF)) {
		integer_value_t false_pos;
		integer_value_t out_pos;

		parser_next_token (parser);
		/* Check '('. */
		if (!parser_test_and_next (parser, TOKEN ('('),
			"expected '(' after if.")) {
			return 0;
		}

		/* Emit a JUMP_FALSE, the para is pending. */
		if (!(false_pos = code_push_opcode (code,
			OPCODE (OP_JUMP_FALSE, 0), line) - 1)) {
			return 0;
		}

		if (!parser_expression (parser, code)) {
			return 0;
		}

		/* Check ')'. */
		if (!parser_test_and_next (parser, TOKEN (')'),
			"missing matching ')'.")) {
			return 0;
		}

		if (!parser_statement (parser, code)) {
			return 0;
		}

		if (parser_check (parser, TOKEN_ELSE)) {
			integer_value_t force_pos;

			parser_next_token (parser);
			/* Emit a JUMP_FORCE, the para is pending. */
			if (!(force_pos = code_push_opcode (code,
				OPCODE (OP_JUMP_FORCE, 0), line) - 1)) {
				return 0;
			}

			/* Modify last JUMP_FALSE. */
			if (!code_modify_opcode (code, false_pos,
				OPCODE (OP_JUMP_FALSE, force_pos + 1), line)) {
				return 0;
			}

			line = TOKEN_LINE (parser->token);
			if (!parser_statement (parser, code)) {
				return 0;
			}

			/* Modify last JUMP_FORCE. */
			out_pos = code_current_pos (code) + 1;

			return code_modify_opcode (code, force_pos,
				OPCODE (OP_JUMP_FORCE, out_pos), line);
		}
		else {
			out_pos = code_current_pos (code) + 1;
			
			return code_modify_opcode (code, false_pos,
				OPCODE (OP_JUMP_FALSE, out_pos), line);
		}
	}
	else {
	}

	return 1;
}

/* statement:
 * labeled-statement
 * compound-statement
 * expression-statement
 * selection-statement
 * iteration-statement
 * jump-statement */
static int
parser_statement (parser_t *parser, code_t *code)
{
	switch (TOKEN_TYPE (parser->token)) {
		case TOKEN_CASE:
		case TOKEN_DEFAULT:
			return parser_labeled_statement (parser, code);
		case TOKEN_IF:
		case TOKEN_SWITCH:
			return parser_selection_statement (parser, code);
		case TOKEN_WHILE:
		case TOKEN_DO:
		case TOKEN_FOR:
			return parser_iteration_statement (parser, code);
		case TOKEN_CONTINUE:
		case TOKEN_BREAK:
		case TOKEN_RETURN:
			return parser_jump_statement (parser, code);
		default:
			break;
	}

	return parser_expression_statement (parser, code);
}

/* multiplicative-expression:
 * cast-expression
 * cast-expression * multiplicative-expression
 * cast-expression / multiplicative-expression
 * cast-expression % multiplicative-expression */
static int
parser_multiplicative_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;
	op_t op;

	if (!skip) {
		if (!parser_cast_expression (parser, code)) {
			return 0;
		}
	}

	while ((op = parser_get_multiplicative_op (parser))) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_cast_expression (parser, code)) {
			return 0;
		}

		/* Emit a multiplicative opcode. */
		if (!code_push_opcode (code, OPCODE (op, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* additive-expression:
 * multiplicative-expression
 * multiplicative-expression + additive-expression
 * multiplicative-expression - additive-expression */
static int
parser_additive_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;
	op_t op;

	if (!skip) {
		if (!parser_multiplicative_expression (parser, code, 0)) {
			return 0;
		}
	}

	while ((op = parser_get_additive_op (parser))) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_multiplicative_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a additive opcode. */
		if (!code_push_opcode (code, OPCODE (op, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* shift-expression:
 * additive-expression
 * additive-expression << shift-expression
 * additive-expression >> shift-expression */
static int
parser_shift_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;
	op_t op;

	if (!skip) {
		if (!parser_additive_expression (parser, code, 0)) {
			return 0;
		}
	}

	while ((op = parser_get_shift_op (parser))) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_additive_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a shift opcode. */
		if (!code_push_opcode (code, OPCODE (op, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* relational-expression:
 * shift-expression
 * shift-expression < relational-expression
 * shift-expression > relational-expression
 * shift-expression <= relational-expression
 * shift-expression >= relational-expression */
static int
parser_relational_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;
	op_t op;

	if (!skip) {
		if (!parser_shift_expression (parser, code, 0)) {
			return 0;
		}
	}

	while ((op = parser_get_relational_op (parser))) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_shift_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a relational opcode. */
		if (!code_push_opcode (code, OPCODE (op, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* equality-expression:
 * relational-expression
 * relational-expression == equality-expression
 * relational-expression != equality-expression */
static int
parser_equality_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;
	op_t op;

	if (!skip) {
		if (!parser_relational_expression (parser, code, 0)) {
			return 0;
		}
	}

	while ((op = parser_get_equality_op (parser))) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_relational_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a equality opcode. */
		if (!code_push_opcode (code, OPCODE (op, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* AND-expression:
 * equality-expression
 * equality-expression & AND-expression */
static int
parser_and_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;

	if (!skip) {
		if (!parser_equality_expression (parser, code, 0)) {
			return 0;
		}
	}

	while (parser_check (parser, TOKEN ('&'))) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_equality_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a BIT_AND. */
		if (!code_push_opcode (code, OPCODE (OP_BIT_AND, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* exclusive-OR-expression:
 * AND-expression
 * AND-expression ˆ exclusive-OR-expression */
static int
parser_exclusive_or_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;

	if (!skip) {
		if (!parser_and_expression (parser, code, 0)) {
			return 0;
		}
	}

	while (parser_check (parser, TOKEN ('^'))) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_and_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a BIT_XOR. */
		if (!code_push_opcode (code, OPCODE (OP_BIT_XOR, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* inclusive-OR-expression:
 * exclusive-OR-expression
 * exclusive-OR-expression | inclusive-OR-expression */
static int
parser_inclusive_or_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;

	if (!skip) {
		if (!parser_exclusive_or_expression (parser, code, 0)) {
			return 0;
		}
	}

	while (parser_check (parser, TOKEN ('|'))) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_exclusive_or_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a BIT_OR. */
		if (!code_push_opcode (code, OPCODE (OP_BIT_OR, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* logical-AND-expression:
 * inclusive-OR-expression
 * inclusive-OR-expression && logical-AND-expression */
static int
parser_logical_and_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;

	if (!skip) {
		if (!parser_inclusive_or_expression (parser, code, 0)) {
			return 0;
		}
	}

	while (parser_check (parser, TOKEN_LAND)) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_inclusive_or_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a LOGIC_AND. */
		if (!code_push_opcode (code, OPCODE (OP_LOGIC_AND, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* logical-OR-expression:
 * logical-AND-expression
 * logical-AND-expression || logical-OR-expression */
static int
parser_logical_or_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;

	if (!skip) {
		if (!parser_logical_and_expression (parser, code, 0)) {
			return 0;
		}
	}

	while (parser_check (parser, TOKEN_LOR)) {
		line = TOKEN_LINE (parser->token);
		parser_next_token (parser);
		if (!parser_logical_and_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a LOGIC_OR. */
		if (!code_push_opcode (code, OPCODE (OP_LOGIC_OR, 0), line)) {
			return 0;
		}
	}

	return 1;
}

/* conditional-expression:
 * logical-OR-expression
 * logical-OR-expression ? expression : conditional-expression */
static int
parser_conditional_expression (parser_t *parser, code_t *code, int skip)
{
	uint32_t line;

	if (!skip) {
		if (!parser_logical_or_expression (parser, code, 0)) {
			return 0;
		}
	}

	line = TOKEN_LINE (parser->token);
	if (parser_check (parser, TOKEN ('?'))) {
		parser_next_token (parser);
		if (!parser_expression (parser, code)) {
			return 0;
		}

		/* Check ':'. */
		if (!parser_test_and_next (parser, TOKEN (':'),
			"missing ':' in conditional expression.")) {
			return 0;
		}

		/* Recursion needed. */
		if (!parser_conditional_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit a CON_SEL. */
		return code_push_opcode (code, OPCODE (OP_CON_SEL, 0), line);
	}

	return 1;
}

/* argument-expression-list:
 * assignment-expression
 * assignment-expression , argument-expression-list */
static int 
parser_argument_expression_list (parser_t *parser, code_t *code)
{
	para_t size;
	uint32_t line;

	size = 1;
	line = TOKEN_LINE (parser->token);
	if (!parser_assignment_expression (parser, code)) {
		return 0;
	}

	while (parser_check (parser, TOKEN (','))) {
		size++;
		parser_next_token (parser);
		if (!parser_assignment_expression (parser, code)) {
			return 0;
		}
		if (size > MAX_PARA) {
			break;
		}
	}
	
	/* Check argument list size. */
	if (size > MAX_PARA) {
		parser_syntax_error (parser, "number of arguments exceeded.");

		return 0;
	}
	
	/* Emit a MAKE_VEC. */
	return code_push_opcode (code, OPCODE (OP_MAKE_VEC, size), line);
}

/* expression-postfix:
 * [ expression ]
 * ( argument-expression-listopt )
 * ++
 * -- */
static int
parser_expression_postfix (parser_t *parser, code_t *code)
{
	uint32_t line;
	opcode_t last;

	line = TOKEN_LINE (parser->token);
	if (parser_check (parser, TOKEN ('['))) {
		parser_next_token (parser);
		if (!parser_expression (parser, code)) {
			return 0;
		}
		
		/* Skip ']'. */
		if (!parser_test_and_next (parser, TOKEN (']'),
			"missing matching ']' for indexing."))

			return 0;
		}

		/* Emit a LOAD_INDEX. */
		return code_push_opcode (code, OPCODE (OP_LOAD_INDEX, 0), line);
	}
	else if (parser_check (parser, TOKEN ('('))) {
		parser_next_token (parser);
		if (parser_check (parser, TOKEN (')'))) {
			/* Empty argument list, emit a zero MAKE_VEC and CALL_FUNC. */
			parser_next_token (parser);

			return code_push_opcode (code, OPCODE (OP_MAKE_VEC, 0), line) &&
				code_push_opcode (code, OPCODE (OP_CALL_FUNC, 0), line);
		}

		if (!parser_argument_expression_list (parser, code)) {
			return 0;
		}

		/* Skip ')'. */
		if (!parser_test_and_next (parser, TOKEN (')'),
			"missing matching ')'.")) {
			return 0;
		}

		/* Emit a CALL_FUNC code. */
		return code_push_opcode (code, OPCODE (OP_CALL_FUNC, 0), line);
	}
	else if (parser_check (parser, TOKEN_INC)){
		parser_next_token (parser);
		last = code_last_opcode (code);
		if (OPCODE_OP (last) == OP_LOAD_VAR) {
			return code_modify_opcode (code, -1, OPCODE (OP_VAR_POINC, OPCODE_PARA (last)), line);
		}
		else if (OPCODE_OP (last) == OP_LOAD_INDEX) {
			return code_modify_opcode (code, -1, OPCODE (OP_INDEX_POINC, OPCODE_PARA (last)), line);
		}
		else {
			parser_syntax_error (parser, "lvalue requierd.");

			return 0;
		}
	}
	else if (parser_check (parser, TOKEN_DEC)){
		parser_next_token (parser);
		last = code_last_opcode (code);
		if (OPCODE_OP (last) == OP_LOAD_VAR) {
			return code_modify_opcode (code, -1, OPCODE (OP_VAR_PODEC, OPCODE_PARA (last)), line);
		}
		else if (OPCODE_OP (last) == OP_LOAD_INDEX) {
			return code_modify_opcode (code, -1, OPCODE (OP_INDEX_PODEC, OPCODE_PARA (last)), line);
		}
		else {
			parser_syntax_error (parser, "lvalue requierd.");

			return 0;
		}
	}

	return 0;
}

/* expression-postfix-list:
 * expression-postfix expression-postfix-list. */
static int
parser_expression_postfix_list (parser_t *parser, code_t *code)
{
	for (;;) {
		if (parser_check (parser, TOKEN ('[')) ||
			parser_check (parser, TOKEN ('(')) ||
			parser_check (parser, TOKEN_INC) ||
			parser_check (parser, TOKEN_DEC)) {
			if (!parser_expression_postfix (parser, code)) {
				return 0;
			}

		}
		else {
			break;
		}
	}

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

		parser_next_token (parser);
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
		return parser_test_and_next (parser, TOKEN (')'),
			"missing matching ')'.");
	}

	line = TOKEN_LINE (parser->token);
	switch (TOKEN_TYPE (parser->token)) {
		case TOKEN_IDENTIFIER:
			parser_next_token (parser);
			pos = code_push_varname (code,TOKEN_ID (parser->token), 0);	
			if (pos == -1) {
				return 0;
			}
			/* Emit a LOAD_VAR. */
			return code_push_opcode (code, OPCODE (OP_LOAD_VAR, pos), line);
		case TOKEN_NULL:
			parser_next_token (parser);
			pos = parser_push_const (code,
				OBJECT_TYPE_CHAR, nullobject_new (NULL));
			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		case TOKEN_TRUE:
		case TOKEN_FALSE:
			parser_next_token (parser);
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
			parser_next_token (parser);

			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		}
		case TOKEN_FLOATING:
		case TOKEN_EXPO: {
			double val;

			val = strtod (TOKEN_ID (parser->token), NULL);
			parser_next_token (parser);
			pos = parser_push_const (code,
				OBJECT_TYPE_DOUBLE, doubleobject_new (val, NULL));

			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		}
		case TOKEN_CHARACTER: {
			char val;

			val = (char) *TOKEN_ID (parser->token);
			parser_next_token (parser);
			pos = parser_push_const (code,
				OBJECT_TYPE_CHAR, charobject_new (val, NULL));

			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		}
		case TOKEN_STRING:
			pos = parser_push_const (code,
				OBJECT_TYPE_CHAR, strobject_new (TOKEN_ID (parser->token), NULL));
			parser_next_token (parser);
			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		default:
			if (parser_check (parser, TOKEN ('('))) {
				parser_next_token (parser);
				if (!parser_expression (parser, code)) {
					return 0;
				}

				/* Skip ')'. */
				return parser_test_and_next (parser, TOKEN (')'),
					"missing matching ')'.");
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
	if (!parser_primary_expression (parser, code, leading_par) ||
		!parser_expression_postfix_list (parser, code)) {
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
	opcode_t last;

	if (leading_par) {
		/* Just fall into a postfix-expression. */
		return parser_postfix_expression (parser, code, leading_par);
	}

	type = TOKEN_TYPE (parser->token);
	line = TOKEN_LINE (parser->token);
	switch (type) {
		case TOKEN_INC:
		case TOKEN_DEC:
			parser_next_token (parser);
			if (!parser_unary_expression (parser, code, 0)) {
				return 0;
			}
			/* The last opcode must be a LOAD_VAR or LOAD_INDEX,
			 * otherwise it's not a lvalue. */
			last = code_last_opcode (code);
			if (OPCODE_OP (last) == OP_LOAD_VAR) {
				return type == TOKEN_INC?
					code_modify_opcode (code, -1, OPCODE (OP_VAR_INC, OPCODE_PARA (last)), line):
					code_modify_opcode (code, -1, OPCODE (OP_VAR_DEC, OPCODE_PARA (last)), line);
			}
			else if (OPCODE_OP (last) == OP_LOAD_INDEX){
				return type == TOKEN_INC?
					code_modify_opcode (code, -1, OPCODE (OP_INDEX_INC, OPCODE_PARA (last)), line):
					code_modify_opcode (code, -1, OPCODE (OP_INDEX_DEC, OPCODE_PARA (last)), line);
			}
			else {
				parser_syntax_error (parser, "lvalue required.");

				return 0;
			}
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
					OPCODE (parser_get_unary_op (type), 0), line);	
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
		
		parser_next_token (parser);
		/* Skip ')'. */
		if (!parser_test_and_next (parser, TOKEN (')'),
			"missing matching ')'.")) {
			return 0;
		}

		/* Recursion needed. */
		if (!parser_cast_expression (parser, code)) {
			return 0;
		}

		/* Emit a TYPE_CAST opcode. */
		return code_push_opcode (code, OPCODE (OP_TYPE_CAST, (para_t) type), line);
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

	if (TOKEN_IS_CON (parser->token)) {
		return parser_conditional_expression (parser, code, 1);
	}
	else if (TOKEN_IS_ASSIGN (parser->token)) {
		integer_value_t pos;
		opcode_t last;
		token_type_t type;
		op_t op;
		uint32_t line;

		/* The last opcode must be a LOAD_VAR or LOAD_INDEX,
		 * otherwise it's not a lvalue. */
		line = TOKEN_LINE (parser->token);
		type = TOKEN_TYPE (parser->token);
		pos = code_current_pos (code);
		if (pos == -1) {
			return 0;
		}
		last = code_last_opcode (code);
		if (OPCODE_OP (last) != OP_LOAD_VAR &&
			OPCODE_OP (last) != OP_LOAD_INDEX) {
			parser_syntax_error (parser, "lvalue required.");

			return 0;
		}

		/* Recursion needed. */
		if (!parser_assignment_expression (parser, code)) {
			return 0;
		}
		
		/* Need to remove that LOAD_* opcode and emit an assignment. */
		if (!code_remove_pos (code, pos)) {
			return 0;
		}
		op = OPCODE_OP (last) != OP_LOAD_VAR?
			parser_get_var_assign_op (type):
			parser_get_index_assign_op (type);
		if (!op) {
			parser_syntax_error (parser, "unknown assignment operation.");

			return 0;
		}

		return code_push_opcode (code, OPCODE (op, OPCODE_PARA (last)), line);
	}

	return 0;
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
		parser_next_token (parser);
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
	return code_push_opcode (code, OPCODE (OP_STORE_LOCAL, var_pos), line);
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
	object_type_t t;

	t = type;
	if (t == -1) {
		t = parser_token_object_type (parser);
		if (t == OBJECT_TYPE_VOID) {
			parser_syntax_error (parser, "variable can not be void.");

			return 0;
		}
	}
	else {
		/* Skip type-specifier. */
		parser_next_token (parser);
	}

	if (!parser_init_declarator_list (parser, code, t, first_id)) {
		return 0;
	}

	/* Skip ';'. */
	return parser_test_and_next (parser, TOKEN (';'),
		"missing ';' in declaration.");
}

/* block-item:
 * declaration
 * statement */
static int
parser_block_item (parser_t *parser, code_t *code)
{
	if (TOKEN_IS_TYPE (parser->token)) {
		return parser_declaration (parser, code, -1, NULL);
	}

	return parser_statement (parser, code);
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
	return parser_test_and_next (parser, TOKEN ('}'),
		"missing matching '}' for function body.");
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

	line = TOKEN_LINE (parser->token);
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
	return code_push_opcode (code, OPCODE (OP_LOAD_CONST, const_pos), line) &&
		code_push_opcode (code, OPCODE (OP_STORE_LOCAL, var_pos), line);
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
 * type-specifier(*) identifier(*) ( parameter-listopt ) compound-statement */
static int
parser_function_definition (parser_t *parser, code_t *code,
	object_type_t ret_type, const char *id)
{
	code_t *func_code;
	para_t var_pos;
	para_t const_pos;
	object_t *func_obj;
	uint32_t line;

	/* Make a new code for this function. */
	func_code = code_new (parser->path, id);
	if (func_code == NULL) {
		return 0;
	}

	/* Push func name. */
	var_pos = code_push_varname (code, id, 0);
	if (var_pos == -1) {
		return 0;
	}

	/* Skip '('. */
	parser_next_token (parser);

	/* Has parameter? */
	if (!parser_check (parser, TOKEN (')'))) {
		if (!parser_parameter_list (parser, func_code)) {
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

	line = TOKEN_LINE (parser->token);
	if (!parser_compound_statement (parser, func_code)) {
		return 0;
	}

	/* Make a new funcobject and push this const. */
	func_obj = funcobject_code_new (func_code, NULL);
	if (func_obj == NULL) {
		return 0;
	}
	const_pos = parser_push_const (code, OBJECT_TYPE_FUNC, func_obj);
	if (const_pos == -1) {
		return 0;
	}

	/* Make opcodes and insert them. */
	return code_push_opcode (code, OPCODE (OP_LOAD_CONST, const_pos), line) &&
		code_push_opcode (code, OPCODE (OP_STORE_LOCAL, var_pos), line);
}

/* external-declaration:
 * function-definition
 * declaration */
static int
parser_external_declaration (parser_t *parser, code_t *code)
{
	/* Need to look ahead 3 tokens: type id '('. */
	object_type_t type;
	const char *id;

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

	id = TOKEN_ID (parser->token);
	parser_next_token (parser);
	if (parser_check (parser, TOKEN ('('))) {
		/* Goes to function-definition. */
		return parser_function_definition (parser, code, type, id);
	}

	/* Goes to declaration. */
	if (type == OBJECT_TYPE_VOID) {
		parser_syntax_error (parser, "variable can not be a void.");

		return 0;
	}

	return parser_declaration (parser, code, type, id);
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

	parser->path = path;

	parser_next_token (parser);
	if (!parser_translation_unit (parser, code)) {
		code_free (code);
		parser_free (parser);

		return NULL;
	}

	return code;
}
