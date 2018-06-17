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
#include <string.h>

#include "parser.h"
#include "pool.h"
#include "lex.h"
#include "code.h"
#include "object.h"
#include "str.h"
#include "vec.h"
#include "error.h"
#include "misc.h"
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
static int parser_statement (parser_t *parser, code_t *code,
							 upper_type_t ut, para_t upper_pos);
static int parser_compound_statement (parser_t *parser, code_t *code,
									  upper_type_t ut, para_t out_pos);
static int parser_conditional_expression (parser_t *parser, code_t *code,
										  int skip);
static int parser_declaration (parser_t *parser, code_t *code,
							   object_type_t type, const char *first_id);

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

static int
parser_syntax_error (parser_t *parser, const char *err)
{
	error ("syntax error: %s:%d: %s",
		parser->path, TOKEN_LINE (parser->token), err);

	return 0;
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

static token_t *
parser_next_no_free (parser_t * parser)
{
	token_t *prev;

	prev = parser->token;
	parser->token = lex_next (parser->reader);

	return prev;
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
		return parser_syntax_error (parser, err);
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
		return OP_NEGATIVE;
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
		return OP_LARGER_THAN;
	}
	else if (parser_check (parser, TOKEN_LEEQ)) {
		return OP_LESS_EQUAL;
	}
	else if (parser_check (parser, TOKEN_LAEQ)) {
		return OP_LARGER_EQUAL;
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

static int
parser_push_dummy_return (parser_t *parser, code_t *code)
{
	para_t pos;
	uint32_t line;

	line = TOKEN_LINE (parser->token);
	pos = parser_push_const (code, OBJECT_TYPE_VOID, NULL);
	if (pos == -1) {
		return 0;
	}

	/* Emit a LOAD_CONST. */
	if (!code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line)) {
		return 0;
	}

	return code_push_opcode (code, OPCODE (OP_RETURN, 0), line);
}

static int
parser_adjust_jump_force (code_t *code, para_t start_pos,
					para_t len, para_t move)
{
	for (para_t i = start_pos; i < start_pos + len; i++) {
		opcode_t opcode;

		opcode = code_get_pos (code, i);
		if (OPCODE_OP (opcode) == OP_JUMP_FORCE &&
			!code_modify_opcode (code, i,
			OPCODE (OP_JUMP_FORCE, OPCODE_PARA (opcode) + move), 0)) {
			return 0;
		}
	}

	return 1;
}

static int
parser_adjust_jump (code_t *code, para_t start_pos,
					para_t len, para_t move)
{
	for (para_t i = start_pos; i < start_pos + len; i++) {
		opcode_t opcode;
		op_t op;
		para_t para;

		opcode = code_get_pos (code, i);
		op = OPCODE_OP (opcode);
		para = OPCODE_PARA (opcode);
		if (!OPCODE_IS_JUMP (opcode) || para >= start_pos + len) {
			continue;
		}

		if (!code_modify_opcode (code, i, OPCODE (op, para + move), 0)) {
			return 0;
		}
	}

	return 1;
}

static int
parser_adjust_case (code_t *code, para_t start_pos, para_t insert_pos,
					para_t case_pos, para_t len)
{
	/* Adjust case opcodes. */
	for (para_t i = case_pos; i < case_pos + len - 3; i++) {
		code_switch_opcode (code, i, i + 1);
	}

	/* Move case opcodes into insert_pos. */
	for (para_t i = insert_pos; i < (insert_pos + case_pos) / 2; i++) {
		code_switch_opcode (code, i, insert_pos + case_pos - i - 1);
	}
	for (para_t i = case_pos; i < (case_pos * 2 + len) / 2; i++) {
		code_switch_opcode (code, i, case_pos * 2 + len - i - 1);
	}
	for (para_t i = insert_pos; i < (insert_pos + case_pos + len) / 2; i++) {
		code_switch_opcode (code, i, insert_pos + case_pos + len - i - 1);
	}

	/* Modify all jump opcodes in this range. */
	if (!parser_adjust_jump (code, insert_pos + len,
		case_pos - insert_pos, len)) {
		return 0;
	}

	/* Modify previous JUMP_FORCE. */
	return parser_adjust_jump_force (code, start_pos,
									 insert_pos - start_pos, len);
}

static int
parser_adjust_default (code_t *code, para_t start_pos,
					   para_t insert_pos, para_t push_pos)
{
	for (para_t i = push_pos; i > insert_pos; i--) {
		code_switch_opcode (code, i, i - 1);
	}
	for (para_t i = push_pos + 1; i > insert_pos + 1; i--) {
		code_switch_opcode (code, i, i - 1);
	}

	/* Modify all jump opcodes in this range. */
	if (!parser_adjust_jump (code, insert_pos + 2,
							 push_pos - insert_pos, 2)) {
		return 0;
	}

	/* Modify previous JUMP_FORCE. */
	return parser_adjust_jump_force (code, start_pos,
									 insert_pos - start_pos, 2);
}

static int
parser_adjust_assignment (code_t *code, para_t start_pos,
						  para_t assign_pos, para_t end_pos)
{
	for (para_t i = start_pos; i < (start_pos + assign_pos) / 2; i++) {
		code_switch_opcode (code, i, start_pos + assign_pos - i - 1);
	}
	for (para_t i = assign_pos; i < (assign_pos + end_pos) / 2; i++) {
		code_switch_opcode (code, i, assign_pos + end_pos - i - 1);
	}
	for (para_t i = start_pos; i < (start_pos + end_pos) / 2; i++) {
		code_switch_opcode (code, i, start_pos + end_pos - i - 1);
	}

	return 1;
}

/* for-statement:
 * for ( expressionopt ; expressionopt ; expressionopt ) statement
 * for ( declaration expressionopt ; expressionopt ) statement */
static int
parser_for_statement (parser_t *parser, code_t *code)
{
	uint32_t line;
	para_t eval_pos;
	para_t eval_true_pos;
	para_t eval_force_pos;
	para_t iter_pos;
	para_t statement_pos;
	para_t out_pos;
	int declared;

	parser_next_token (parser);
	/* Check '('. */
	if (!parser_test_and_next (parser, TOKEN ('('),
		"expected '(' after for.")) {
		return 0;
	}

	declared = 0;
	if (TOKEN_IS_TYPE (parser->token)) {
		declared = 1;
		line = TOKEN_LINE (parser->token);
		/* Emit an ENTER_BLOCK. */
		if (!code_push_opcode (code, OPCODE (OP_ENTER_BLOCK, 0), line)) {
			return 0;
		}

		if (!parser_declaration (parser, code, -1, NULL)) {
			return 0;
		}
	}
	else {
		/* There are initializing codes? */
		if (!parser_check (parser, TOKEN (';')) &&
			!parser_expression (parser, code)) {
			return 0;
		}

		/* Skip ';'. */
		if (!parser_test_and_next (parser, TOKEN (';'),
			"expected ';' after initializer.")) {
			return 0;
		}
	}

	eval_pos = code_current_pos (code) + 1;
	if (parser_check (parser, TOKEN (';'))) {
		/* Push a true object as evalued result. */
		para_t pos;

		pos = parser_push_const (code, OBJECT_TYPE_BOOL,
								 boolobject_new (true, NULL));
		if (pos == -1) {
			return 0;
		}

		/* Emit a LOAD_CONST. */
		line = TOKEN_LINE (parser->token);
		if (!code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line)) {
			return 0;
		}
	}
	else if (!parser_expression (parser, code)) {
		return 0;
	}

	/* Emit a JUMP_TRUE and JUMP_FORCE. */
	line = TOKEN_LINE (parser->token);
	eval_true_pos = code_push_opcode (code, OPCODE (OP_JUMP_TRUE, 0), line)
		- 1;
	eval_force_pos = code_push_opcode (code, OPCODE (OP_JUMP_FORCE, 0), line)
		- 1;
	if (eval_true_pos == -1 || eval_force_pos == -1) {
		return 0;
	}

	/* Skip ';'. */
	if (!parser_test_and_next (parser, TOKEN (';'),
		"expected ';' after evaluation.")) {
		return 0;
	}

	/* There is an iteration part? */
	iter_pos = code_current_pos (code) + 1;
	if (!parser_check (parser, ')') &&
		!parser_expression (parser, code)) {
		return 0;
	}

	/* Emit a JUMP_FORCE. */
	line = TOKEN_LINE (parser->token);
	if (!code_push_opcode (code, OPCODE (OP_JUMP_FORCE, eval_pos), line)) {
		return 0;
	}

	/* Skip ')'. */
	if (!parser_test_and_next (parser, TOKEN (')'),
		"expected ')' after iteration.")) {
		return 0;
	}

	statement_pos = code_current_pos (code) + 1;
	if (!parser_statement (parser, code, UPPER_TYPE_FOR, iter_pos)) {
		return 0;
	}

	/* Emit a JUMP_FORCE. */
	line = TOKEN_LINE (parser->token);
	if (!code_push_opcode (code, OPCODE (OP_JUMP_FORCE, iter_pos), line)) {
		return 0;
	}

	/* Modify the jump opcodes after evaluation part. */
	out_pos = code_current_pos (code) + 1;
	if (!code_modify_opcode (code, eval_true_pos,
		OPCODE (OP_JUMP_TRUE, statement_pos), line)) {
		return 0;
	}
	if (!code_modify_opcode (code, eval_force_pos,
		OPCODE (OP_JUMP_FORCE, out_pos), line)) {
		return 0;
	}

	/* JUMP_BREAK opcodes need modification. */
	for (para_t i = statement_pos; i < out_pos; i++) {
		opcode_t opcode;

		opcode = code_get_pos (code, i);
		/* Check the para to get matching break statements. */
		if (OPCODE_OP (opcode) == OP_JUMP_BREAK &&
			OPCODE_PARA (opcode) == iter_pos) {
			if (!code_modify_opcode (code, i,
				OPCODE (OP_JUMP_BREAK, out_pos), 0)) {
				return 0;
			}
		}
	}

	line = TOKEN_LINE (parser->token);
	/* If declared, emit a LEAVE_BLOCK. */
	if (declared && !code_push_opcode (code,
		OPCODE (OP_LEAVE_BLOCK, 0), line)) {
		return 0;
	}

	return 1;
}

/* do-while-statement:
 * do statement while ( expression ) ; */
static int
parser_do_while_statement (parser_t *parser, code_t *code)
{
	uint32_t line;
	para_t statement_pos;
	para_t eval_pos;
	para_t out_pos;
	para_t blocks;

	parser_next_token (parser);
	statement_pos = code_current_pos (code) + 1;
	if (!parser_statement (parser, code, UPPER_TYPE_DO, statement_pos)) {
		return 0;
	}

	/* Check while and '('. */
	if (!parser_test_and_next (parser, TOKEN_WHILE,
		"expected while after do statement.")) {
		return 0;
	}
	if (!parser_test_and_next (parser, TOKEN ('('),
		"expected '(' after while.")) {
		return 0;
	}

	eval_pos = code_current_pos (code) + 1;
	if (!parser_expression (parser, code)) {
		return 0;
	}

	/* Emit a JUMP_TRUE. */
	line = TOKEN_LINE (parser->token);
	if (!code_push_opcode (code,
		OPCODE (OP_JUMP_TRUE, statement_pos), line)) {
		return 0;
	}

	/* Modify JUMP_CONTINUE and JUMP_BREAK opcodes. */
	out_pos = code_current_pos (code) + 1;
	blocks = 0;
	for (para_t i = statement_pos; i < eval_pos; i++) {
		opcode_t opcode;

		opcode = code_get_pos (code, i);
		/* Check the para to get matching break statements. */
		if (OPCODE_OP (opcode) == OP_ENTER_BLOCK) {
			blocks++;
		}
		else if (OPCODE_OP (opcode) == OP_LEAVE_BLOCK) {
			blocks--;
		}
		else if (OPCODE_OP (opcode) == OP_JUMP_BREAK &&
			OPCODE_PARA (opcode) == statement_pos) {
			if (!code_modify_opcode (code, i,
				OPCODE (OP_JUMP_BREAK, out_pos), 0)) {
				return 0;
			}
		}
		else if (OPCODE_OP (opcode) == OP_JUMP_CONTINUE &&
				 OPCODE_PARA (opcode) == statement_pos) {
			if (!code_modify_opcode (code, i,
				OPCODE (OP_JUMP_CONTINUE, eval_pos), 0)) {
				return 0;
			}
		}
		else if (OPCODE_OP (opcode) == OP_POP_BLOCKS &&
				 OPCODE_PARA (opcode) == statement_pos) {
			if (!code_modify_opcode (code, i,
				OPCODE (OP_POP_BLOCKS, blocks), 0)) {
				return 0;
			}
		}
	}

	/* Check ')' and ';'. */
	return parser_test_and_next (parser, TOKEN (')'), "missing matching ')'.") &&
		parser_test_and_next (parser, TOKEN (';'), "missing ';'.");
}

/* while-statement:
 * while ( expression ) statement */
static int
parser_while_statement (parser_t *parser, code_t *code)
{
	uint32_t line;
	para_t eval_pos;
	para_t false_pos;
	para_t statement_pos;
	para_t out_pos;
	para_t blocks;

	parser_next_token (parser);
	/* Check '('. */
	if (!parser_test_and_next (parser, TOKEN ('('),
		"expected '(' after while.")) {
		return 0;
	}

	line = TOKEN_LINE (parser->token);
	eval_pos = code_current_pos (code) + 1;
	if (!parser_expression (parser, code)) {
		return 0;
	}

	/* Emit a JUMP_FALSE, the para is pending. */
	if ((false_pos = code_push_opcode (code,
		OPCODE (OP_JUMP_FALSE, 0), line) - 1) == -1) {
		return 0;
	}

	/* Check ')'. */
	if (!parser_test_and_next (parser, TOKEN (')'),
		"missing matching ')'.")) {
		return 0;
	}

	statement_pos = code_current_pos (code) + 1;
	if (!parser_statement (parser, code, UPPER_TYPE_WHILE, eval_pos)) {
		return 0;
	}

	/* Emit a JUMP_FORCE. */
	line = TOKEN_LINE (parser->token);
	if (!code_push_opcode (code, OPCODE (OP_JUMP_FORCE, eval_pos), line)) {
		return 0;
	}

	/* Modify last JUMP_FALSE. */
	out_pos = code_current_pos (code) + 1;
	if (!code_modify_opcode (code, false_pos,
		OPCODE (OP_JUMP_FALSE, out_pos), 0)) {
		return 0;
	}

	/* JUMP_BREAK opcodes need modification. */
	blocks = 0;
	for (para_t i = statement_pos; i < out_pos; i++) {
		opcode_t opcode;

		opcode = code_get_pos (code, i);
		/* Check the para to get matching break statements. */
		if (OPCODE_OP (opcode) == OP_ENTER_BLOCK) {
			blocks++;
		}
		else if (OPCODE_OP (opcode) == OP_LEAVE_BLOCK) {
			blocks--;
		}
		else if (OPCODE_OP (opcode) == OP_JUMP_BREAK &&
				 OPCODE_PARA (opcode) == eval_pos) {
			if (!code_modify_opcode (code, i,
				OPCODE (OP_JUMP_BREAK, out_pos), 0)) {
				return 0;
			}
		}
		else if (OPCODE_OP (opcode) == OP_POP_BLOCKS &&
				 OPCODE_PARA (opcode) == eval_pos) {
			if (!code_modify_opcode (code, i,
				OPCODE (OP_POP_BLOCKS, blocks), 0)) {
				return 0;
			}
		}
	}

	return 1;
}

/* switch-statement:
 * switch ( expression ) statement */
static int 
parser_switch_statement (parser_t *parser, code_t *code)
{
	uint32_t line;
	para_t start_pos;
	para_t insert_pos;
	para_t jump_pos;
	para_t out_pos;
	para_t last_jump_case;
	para_t blocks;

	parser_next_token (parser);
	/* Check '('. */
	if (!parser_test_and_next (parser, TOKEN ('('),
		"expected '(' after switch.")) {
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

	start_pos = code_current_pos (code) + 1;
	insert_pos = start_pos;

	/* Emit a POP_STACK and JUMP_FORCE. */
	line = TOKEN_LINE (parser->token);
	if (!code_push_opcode (code, OPCODE (OP_POP_STACK, 0), line)) {
		return 0;
	}
	jump_pos = code_push_opcode (code, OPCODE (OP_JUMP_FORCE, 0), line) - 1;
	if (jump_pos == -1) {
		return 0;
	}

	if (!parser_statement (parser, code, UPPER_TYPE_SWITCH, start_pos)) {
		return 0;
	}

	blocks = 0;
	last_jump_case = -1;
	out_pos = code_current_pos (code) + 1;
	for (para_t i = start_pos; i < out_pos; i++) {
		opcode_t opcode;

		opcode = code_get_pos (code, i);
		switch (OPCODE_OP (opcode)) {
			case OP_ENTER_BLOCK:
				blocks++;
				break;
			case OP_LEAVE_BLOCK:
				blocks--;
				break;
			case OP_JUMP_CASE:
				/* Check if this label matchs. */
				if (OPCODE_PARA (opcode) != start_pos) {
					break;
				}
				for (para_t j = i + 1; j < out_pos; j++) {
					opcode_t opcode;

					opcode = code_get_pos (code, j);
					if (OPCODE_OP (opcode) != OP_PUSH_BLOCKS ||
						OPCODE_PARA (opcode) != start_pos) {
						continue;
					}

					if (!code_modify_opcode (code, j,
						OPCODE (OP_PUSH_BLOCKS, blocks), 0)) {
						return 0;
					}
					if (!parser_adjust_case (code, start_pos,
						insert_pos, i, j - i + 2)) {
						return 0;
					}
					if (last_jump_case != -1 &&
						!code_modify_opcode (code, last_jump_case,
						OPCODE (OP_JUMP_CASE, insert_pos), 0)) {
						return 0;
					}
					last_jump_case = insert_pos + j - i - 1;
					jump_pos += j - i + 2;
					insert_pos += j - i + 2;
					i = j + 1;
					break;
				}
				break;
			case OP_PUSH_BLOCKS:
				/* Check if this label matchs. */
				if (OPCODE_PARA (opcode) != start_pos) {
					break;
				}
				if (!code_modify_opcode (code, i,
					OPCODE (OP_PUSH_BLOCKS, blocks), 0)) {
					return 0;
				}
				/* The next must be a matching JUMP_DEFAULT. */ 
				parser_adjust_default (code, start_pos, insert_pos, i);
				if (last_jump_case != -1 &&
					!code_modify_opcode (code, last_jump_case,
					OPCODE (OP_JUMP_CASE, insert_pos), 0)) {
					return 0;
				}
				else {
					last_jump_case = -1;
				}
				i++;
				jump_pos += 2;
				insert_pos += 2;
				break;
			case OP_JUMP_BREAK:
				/* Check if this label matchs. */
				if (OPCODE_PARA (opcode) != start_pos) {
					break;
				}
				/* Modify jump position to current out_pos. */
				if (!code_modify_opcode (code, i,
					OPCODE (OP_JUMP_BREAK, out_pos), 0)) {
					return 0;
				}
				break;
			case OP_POP_BLOCKS:
				/* Check if this opcode matchs. */
				if (OPCODE_PARA (opcode) != start_pos) {
					break;
				}
				/* Modify this POP_BLOCKS. */
				if (!code_modify_opcode (code, i,
					OPCODE (OP_POP_BLOCKS, blocks), 0)) {
					return 0;
				}
				break;
			default:
				break;
		}
	}

	/* Modify the JUMP_FORCE. */
	if (!code_modify_opcode (code, jump_pos,
		OPCODE (OP_JUMP_FORCE, out_pos), 0)) {
		return 0;
	}

	/* Modify the last JUMP_CASE. */
	if (last_jump_case != -1 &&
		!code_modify_opcode (code, last_jump_case,
		OPCODE (OP_JUMP_CASE, jump_pos - 1), 0)) {
		return 0;
	}


	return 1;
}

/* if-statement:
 * if ( expression ) statement
 * if ( expression ) statement else statement */
static int
parser_if_statement (parser_t *parser, code_t *code,
					 upper_type_t ut, para_t upper_pos)
{
	uint32_t line;
	para_t false_pos;
	para_t out_pos;

	parser_next_token (parser);
	line = TOKEN_LINE (parser->token);
	/* Check '('. */
	if (!parser_test_and_next (parser, TOKEN ('('),
		"expected '(' after if.")) {
		return 0;
	}

	if (!parser_expression (parser, code)) {
		return 0;
	}

	/* Emit a JUMP_FALSE, the para is pending. */
	if ((false_pos = code_push_opcode (code,
		OPCODE (OP_JUMP_FALSE, 0), line) - 1) == -1) {
		return 0;
	}

	/* Check ')'. */
	if (!parser_test_and_next (parser, TOKEN (')'),
		"missing matching ')'.")) {
		return 0;
	}

	if (!parser_statement (parser, code, ut, upper_pos)) {
		return 0;
	}

	if (parser_check (parser, TOKEN_ELSE)) {
		para_t force_pos;

		parser_next_token (parser);
		/* Emit a JUMP_FORCE, the para is pending. */
		if ((force_pos = code_push_opcode (code,
			OPCODE (OP_JUMP_FORCE, 0), line) - 1) == -1) {
			return 0;
		}

		/* Modify last JUMP_FALSE. */
		if (!code_modify_opcode (code, false_pos,
			OPCODE (OP_JUMP_FALSE, force_pos + 1), line)) {
			return 0;
		}

		line = TOKEN_LINE (parser->token);
		if (!parser_statement (parser, code, ut, upper_pos)) {
			return 0;
		}

		/* Modify last JUMP_FORCE. */
		out_pos = code_current_pos (code) + 1;

		return code_modify_opcode (code, force_pos,
			OPCODE (OP_JUMP_FORCE, out_pos), line);
	}

	out_pos = code_current_pos (code) + 1;

	return code_modify_opcode (code, false_pos,
		OPCODE (OP_JUMP_FALSE, out_pos), line);
}

/* jump-statement:
 * continue ;
 * break ;
 * return expressionopt ; */
static int
parser_jump_statement (parser_t *parser, code_t *code,
					   upper_type_t ut, para_t upper_pos)
{
	uint32_t line;

	line = TOKEN_LINE (parser->token);
	/* Check upper type. */
	if (parser_check (parser, TOKEN_CONTINUE)) {
		parser_next_token (parser);
		if (ut != UPPER_TYPE_FOR && ut != UPPER_TYPE_DO &&
			ut != UPPER_TYPE_WHILE) {
			return parser_syntax_error (parser, "invalid jump statement.");
		}

		/* Emit POP_BLOCKS and JUMP_CONTINUE. */
		if (!code_push_opcode (code, OPCODE (OP_POP_BLOCKS, upper_pos), line) ||
			!code_push_opcode (code, OPCODE (OP_JUMP_CONTINUE, upper_pos),
			line)) {
			return 0;
		}
	}
	else if (parser_check (parser, TOKEN_BREAK)) {
		parser_next_token (parser);
		if (ut == UPPER_TYPE_PLAIN) {
			return parser_syntax_error (parser, "invalid jump statement.");
		}

		/* Emit POP_BLOCKS and JUMP_BREAK. */
		if (!code_push_opcode (code, OPCODE (OP_POP_BLOCKS, upper_pos), line) ||
			!code_push_opcode (code, OPCODE (OP_JUMP_BREAK, upper_pos),
			line)) {
			return 0;
		}
	}
	else if (parser_check (parser, TOKEN_RETURN)) {
		parser_next_token (parser);
		if (parser_check (parser, TOKEN (';'))) {
			/* Check return type. */
			if (FUNC_RET_TYPE (code) != OBJECT_TYPE_VOID) {
				parser_syntax_error (parser, "non-void func need return a value.");

				return 0;
			}

			/* Push a dummy object. */
			return parser_push_dummy_return (parser, code);
		}
		else {
			/* Check return type. */
			if (FUNC_RET_TYPE (code) == OBJECT_TYPE_VOID) {
				parser_syntax_error (parser, "void func can not return a value.");

				return 0;
			}

			line = TOKEN_LINE (parser->token);
			if (!parser_expression (parser, code)) {
				return 0;
			}

			return code_push_opcode (code, OPCODE (OP_RETURN, 0), line);
		}
	}
	else {
		return 0;
	}

	/* Skip ';'. */
	return parser_test_and_next (parser, TOKEN (';'), "missing ';'.");
}

/* iteration-statement:
 * while-statement
 * do-while-statement
 * for-statement */
static int
parser_iteration_statement (parser_t *parser, code_t *code)
{
	if (parser_check (parser, TOKEN_WHILE)) {
		return parser_while_statement (parser, code);
	}
	else if (parser_check (parser, TOKEN_DO)) {
		return parser_do_while_statement (parser, code);
	}
	else if (parser_check (parser, TOKEN_FOR)) {
		return parser_for_statement (parser, code);
	}

	return parser_syntax_error (parser, "unknown iteration expression");
}

/* selection-statement:
 * if-statement
 * switch-statement */
static int
parser_selection_statement (parser_t *parser, code_t *code,
							upper_type_t ut, para_t upper_pos)
{
	if (parser_check (parser, TOKEN_IF)) {
		return parser_if_statement (parser, code, ut, upper_pos);
	}

	return parser_switch_statement (parser, code);
}

/* expression-statement:
 * expressionopt ; */
static int
parser_expression_statement (parser_t *parser, code_t *code)
{
	uint32_t line;

	if (parser_check (parser, TOKEN (';'))) {
		/* Empty statement, do nothing. */
		parser_next_token (parser);

		return 1;
	}

	if (!parser_expression (parser, code)) {
		return 0;
	}

	/* Skip ';'. */
	if (!parser_test_and_next (parser, TOKEN (';'),
		"missing ';' in the end of the statement.")) {
		return 0;
	}

	/* Emit a POP_STACK to ignore left part. */
	line = TOKEN_LINE (parser->token);

	return code_push_opcode (code, OPCODE (OP_POP_STACK, 0), line);
}

/* labeled-statement:
 * case conditional-expression : statement
 * default : statement */
static int
parser_labeled_statement (parser_t *parser, code_t *code,
						  upper_type_t ut, para_t upper_pos)
{
	uint32_t line;
	para_t current;

	/* Check upper type. */
	if (ut != UPPER_TYPE_SWITCH) {
		return parser_syntax_error (parser, "unmatched switch label.");
	}

	line = TOKEN_LINE (parser->token);
	if (parser_check (parser, TOKEN_CASE)) {
		parser_next_token (parser);
		/* Emit a JUMP_CASE. */
		if (!code_push_opcode (code, OPCODE (OP_JUMP_CASE, upper_pos), line) ||
			!parser_conditional_expression (parser, code, 0)) {
			return 0;
		}

		/* Emit PUSH_BLOCKS and JUMP_FORCE. */
		line = TOKEN_LINE (parser->token);
		current = code_current_pos (code) + 3;
		if (!code_push_opcode (code, OPCODE (OP_PUSH_BLOCKS, upper_pos), line) ||
			!code_push_opcode (code, OPCODE (OP_JUMP_FORCE, current), line)) {
			return 0;
		}
	}
	else {
		parser_next_token (parser);
		/* Emit PUSH_BLOCKS and JUMP_DEFAULT. */
		line = TOKEN_LINE (parser->token);
		if (!code_push_opcode (code, OPCODE (OP_PUSH_BLOCKS, upper_pos), line)) {
			return 0;
		}
		current = code_current_pos (code) + 2;
		if (!code_push_opcode (code,
			OPCODE (OP_JUMP_DEFAULT, current), line)) {
			return 0;
		}
	}

	/* Skip ':'. */
	if (!parser_test_and_next (parser, TOKEN (':'),
		"missing matching ':' for label statement.")) { 
			return 0;
	}

	return parser_statement (parser, code, ut, upper_pos);
}

/* statement:
 * labeled-statement
 * compound-statement
 * expression-statement
 * selection-statement
 * iteration-statement
 * jump-statement */
static int
parser_statement (parser_t *parser, code_t *code,
				  upper_type_t ut, para_t upper_pos)
{
	switch (TOKEN_TYPE (parser->token)) {
		case TOKEN_CASE:
		case TOKEN_DEFAULT:
			return parser_labeled_statement (parser, code, ut, upper_pos);
		case TOKEN_IF:
		case TOKEN_SWITCH:
			return parser_selection_statement (parser, code, ut, upper_pos);
		case TOKEN_WHILE:
		case TOKEN_DO:
		case TOKEN_FOR:
			return parser_iteration_statement (parser, code);
		case TOKEN_CONTINUE:
		case TOKEN_BREAK:
		case TOKEN_RETURN:
			return parser_jump_statement (parser, code, ut, upper_pos);
		default:
			if (parser_check (parser, TOKEN ('{'))) {
				uint32_t line;

				/* Emit an ENTER_BLOCK. */
				line = TOKEN_LINE (parser->token);
				if (!code_push_opcode (code,
					OPCODE (OP_ENTER_BLOCK, 0), line)) {
					return 0;
				}

				if (!parser_compound_statement (parser, code, ut, upper_pos)) {
					return 0;
				}

				/* Emit an LEAVE_BLOCK. */
				line = TOKEN_LINE (parser->token);
				
				return code_push_opcode (code, 
										 OPCODE (OP_LEAVE_BLOCK, 0), line);
			}
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

	if (!parser_multiplicative_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_additive_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_shift_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_relational_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_equality_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_and_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_exclusive_or_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_inclusive_or_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_logical_and_expression (parser, code, skip)) {
		return 0;
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

	if (!parser_logical_or_expression (parser, code, skip)) {
		return 0;
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
		return parser_syntax_error (parser, "number of arguments exceeded.");
	}
	
	/* Emit a MAKE_VEC. */
	if (size > 0 &&
		!code_push_opcode (code, OPCODE (OP_MAKE_VEC, size), line)) {
		return 0;
	}

	return 1;
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
			"missing matching ']' for indexing.")) { 
			return 0;
		}

		/* Emit a LOAD_INDEX. */
		return code_push_opcode (code, OPCODE (OP_LOAD_INDEX, 0), line);
	}
	else if (parser_check (parser, TOKEN ('('))) {
		parser_next_token (parser);
		if (parser_check (parser, TOKEN (')'))) {
			/* Empty argument list, emit a CALL_FUNC, no need to Emit
			 * a MAKE_VEC. */
			parser_next_token (parser);

			return code_push_opcode (code, OPCODE (OP_CALL_FUNC, 0), line);
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
			return code_modify_opcode (code, -1,
				OPCODE (OP_VAR_POINC, OPCODE_PARA (last)), line);
		}
		else if (OPCODE_OP (last) == OP_LOAD_INDEX) {
			return code_modify_opcode (code, -1,
				OPCODE (OP_INDEX_POINC, OPCODE_PARA (last)), line);
		}
		else {
			return parser_syntax_error (parser, "lvalue requierd.");
		}
	}
	else if (parser_check (parser, TOKEN_DEC)){
		parser_next_token (parser);
		last = code_last_opcode (code);
		if (OPCODE_OP (last) == OP_LOAD_VAR) {
			return code_modify_opcode (code, -1,
				OPCODE (OP_VAR_PODEC, OPCODE_PARA (last)), line);
		}
		else if (OPCODE_OP (last) == OP_LOAD_INDEX) {
			return code_modify_opcode (code, -1,
				OPCODE (OP_INDEX_PODEC, OPCODE_PARA (last)), line);
		}
		else {
			return parser_syntax_error (parser, "lvalue requierd.");
		}
	}

	return parser_syntax_error (parser, "unknown expression postfix.");
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
			pos = code_push_varname (code,TOKEN_ID (parser->token),
									 OBJECT_TYPE_VOID, 0);
			parser_next_token (parser);
			if (pos == -1) {
				return 0;
			}
			/* Emit a LOAD_VAR. */
			return code_push_opcode (code, OPCODE (OP_LOAD_VAR, pos), line);
		case TOKEN_NULL:
			parser_next_token (parser);
			pos = parser_push_const (code,
									 OBJECT_TYPE_NULL, nullobject_new (NULL));
			if (pos == -1) {
				return 0;
			}
			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		case TOKEN_TRUE:
		case TOKEN_FALSE:
			pos = parser_push_const (code,
									 OBJECT_TYPE_BOOL,
									 boolobject_new (TOKEN_TYPE (parser->token) == TOKEN_TRUE,
									 NULL));
			if (pos == -1) {
				return 0;
			}
			parser_next_token (parser);
			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		case TOKEN_INTEGER:
		case TOKEN_HEXINT:
		case TOKEN_LINTEGER: {
			long val;

			val = strtol (TOKEN_ID (parser->token), NULL, 0);
			if (val > INT_MAX || val < INT_MIN ||
				parser_check (parser, TOKEN_LINTEGER)) {
				pos = parser_push_const (code,
										 OBJECT_TYPE_LONG,
										 longobject_new (val, NULL));
			}
			else {
				pos = parser_push_const (code,
										 OBJECT_TYPE_INT,
										 intobject_new ((int) val, NULL));
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
									 OBJECT_TYPE_DOUBLE,
									 doubleobject_new (val, NULL));
			if (pos == -1) {
				return 0;
			}

			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		}
		case TOKEN_CHARACTER: {
			char val;

			val = (char) *TOKEN_ID (parser->token);
			parser_next_token (parser);
			pos = parser_push_const (code,
									 OBJECT_TYPE_CHAR,
									 charobject_new (val, NULL));
			if (pos == -1) {
				return 0;
			}

			/* Emit a LOAD_CONST. */
			return code_push_opcode (code, OPCODE (OP_LOAD_CONST, pos), line);
		}
		case TOKEN_STRING:
			pos = parser_push_const (code,
									 OBJECT_TYPE_STR, 
									 strobject_new (TOKEN_ID (parser->token),
									 strlen (TOKEN_ID (parser->token)), 0,
									 NULL));
			parser_next_token (parser);
			if (pos == -1) {
				return 0;
			}
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

	return parser_syntax_error (parser, "invalid primary expression.");
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

	if (parser_check (parser, TOKEN ('[')) ||
		parser_check (parser, TOKEN ('(')) ||
		parser_check (parser, TOKEN_INC) ||
		parser_check (parser, TOKEN_DEC)) {
		return parser_expression_postfix_list (parser, code);
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
				para_t para;

				para = OPCODE_PARA (last);

				return type == TOKEN_INC?
					code_modify_opcode (code, -1, 
										OPCODE (OP_VAR_INC, para), line):
					code_modify_opcode (code, -1,
										OPCODE (OP_VAR_DEC, para), line);
			}
			else if (OPCODE_OP (last) == OP_LOAD_INDEX){
				para_t para;

				para = OPCODE_PARA (last);

				return type == TOKEN_INC?
					code_modify_opcode (code, -1,
										OPCODE (OP_INDEX_INC, para), line):
					code_modify_opcode (code, -1,
										OPCODE (OP_INDEX_DEC, para), line);
			}
			else {
				return parser_syntax_error (parser, "lvalue required.");
			}
		default:
			if (type == TOKEN ('+')) {
				/* Just skip '+'. */
				parser_next_token (parser);

				return parser_cast_expression (parser, code);
			}
			if (type == TOKEN ('-') || type == TOKEN ('~') ||
				type == TOKEN ('!')) {
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
		/* Oops, it's an unary-expression,
		 * and we skipped its leading parenthese. */
		if (type == -1) {
			return parser_unary_expression (parser, code, 1);
		}
		else if (type == OBJECT_TYPE_VOID) {
			return parser_syntax_error (parser,
										"can not cast any type to void.");
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
		return code_push_opcode (code,
								 OPCODE (OP_TYPE_CAST, (para_t) type), line);
	}

	return parser_unary_expression (parser, code, 0);
}

/* assignment-expression:
 * conditional-expression
 * unary-expression assignment-operator assignment-expression */
static int
parser_assignment_expression (parser_t *parser, code_t *code)
{
	para_t start_pos;

	/* The first part is always a cast-expression. */
	start_pos = code_current_pos (code) + 1;
	if (!parser_cast_expression (parser, code)) {
		return 0;
	}

	if (TOKEN_IS_CON (parser->token)) {
		return parser_conditional_expression (parser, code, 1);
	}
	else if (TOKEN_IS_ASSIGN (parser->token)) {
		opcode_t last;
		token_type_t type;
		op_t op;
		para_t assign_pos;
		para_t end_pos;
		uint32_t line;

		/* The last opcode must be a LOAD_VAR or LOAD_INDEX,
		 * otherwise it's not a lvalue. */
		line = TOKEN_LINE (parser->token);
		type = TOKEN_TYPE (parser->token);
		last = code_last_opcode (code);
		if (OPCODE_OP (last) != OP_LOAD_VAR &&
			OPCODE_OP (last) != OP_LOAD_INDEX) {
			return parser_syntax_error (parser, "lvalue required.");
		}

		/* Recursion needed. */
		parser_next_token (parser);
		assign_pos = code_current_pos (code) + 1;
		if (!parser_assignment_expression (parser, code)) {
			return 0;
		}

		end_pos = code_current_pos (code) + 1;
		if (!parser_adjust_assignment (code, start_pos, assign_pos, end_pos)) {
			return 0;
		}
		
		/* Need to remove that LOAD_* opcode and emit an assignment. */
		op = OPCODE_OP (last) == OP_LOAD_VAR?
			parser_get_var_assign_op (type):
			parser_get_index_assign_op (type);
		if (!op) {
			return parser_syntax_error (parser,
										"unknown assignment operation.");
		}

		return code_modify_opcode (code, -1,
								   OPCODE (op, OPCODE_PARA (last)), line);
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
			return parser_syntax_error (parser, "missing identifier name.");
		}

		var = TOKEN_ID (parser->token);
	}

	var_pos = code_push_varname (code, var, type, 0);
	if (var_pos == -1) {
		return 0;
	}

	if (id == NULL) {
		parser_next_token (parser);
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
			return parser_syntax_error (parser, "variable can not be void.");
		}
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
parser_block_item (parser_t *parser, code_t *code,
				   upper_type_t ut, para_t upper_pos)
{
	if (TOKEN_IS_TYPE (parser->token)) {
		return parser_declaration (parser, code, -1, NULL);
	}

	return parser_statement (parser, code, ut, upper_pos);
}

/* block-item-list:
 * block-item
 * block-item block-item-list */
static int
parser_block_item_list (parser_t *parser, code_t *code,
						upper_type_t ut, para_t upper_pos)
{
	while (!parser_check (parser, TOKEN_END) &&
		   !parser_check (parser, TOKEN ('}'))) {
		if (!parser_block_item (parser, code, ut, upper_pos)) {
			return 0;
		}
	}

	return 1;
}

/* compound-statement:
 * { block-item-listopt } */
static int
parser_compound_statement (parser_t *parser, code_t *code,
						   upper_type_t ut, para_t upper_pos)
{
	/* Skip '{'. */
	parser_next_token (parser);
	
	/* Check empty function body. */
	if (!parser_check (parser, TOKEN ('}'))) {
		if (!parser_block_item_list (parser, code, ut, upper_pos)) {
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
	type = parser_token_object_type (parser);

	if (type == -1) {
		return parser_syntax_error (parser, "unknown parameter type.");
	}
	else if (type == OBJECT_TYPE_VOID) {
		return parser_syntax_error (parser, "parameter can not be a void.");
	}

	parser_next_token (parser);
	if (!parser_check (parser, TOKEN_IDENTIFIER)) {
		return parser_syntax_error (parser, "missing identifier name.");
	}
	
	/* Insert a default value for this parameter, this is used while
	 * checking arguments. */
	/* Insert parameter local var and const. */
	if (code_push_varname (code, TOKEN_ID (parser->token), type, 1) == -1) {
		return 0;
	}

	parser_next_token (parser);

	/* Make opcodes and insert them. */
	return 1;
}

/* parameter-list:
 * parameter-declaration
 * parameter-declaration, parameter-list */
static int
parser_parameter_list (parser_t *parser, code_t *code)
{
	para_t pos;
	uint32_t line;

	line = TOKEN_LINE (parser->token);
	/* First parameter. */
	pos = 1;
	if (!parser_parameter_declaration (parser, code)) {
		return 0;
	}

	while (parser_check (parser, TOKEN (','))) {
		pos++;
		if (pos > MAX_PARA) {
			return parser_syntax_error (parser,
										"number of parameters exceeded.");
		}

		parser_next_token (parser);
		if (!parser_parameter_declaration (parser, code)) {
			return 0;
		}
	}

	return code_push_opcode (code, OPCODE (OP_BIND_ARGS, pos), line);
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
	opcode_t last;
	uint32_t line;

	line = TOKEN_LINE (parser->token);
	/* Make a new code for this function. */
	func_code = code_new (parser->path, id);
	if (func_code == NULL) {
		return 0;
	}

	code_set_func (func_code, line, ret_type);

	/* Push func name. */
	var_pos = code_push_varname (code, id, OBJECT_TYPE_FUNC, 0);
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

	if (!parser_test_and_next (parser, TOKEN (')'), "missing matching ')'.")) {
		return 0;
	}

	if (!parser_check (parser, TOKEN ('{'))) {
		return parser_syntax_error (parser,
									"missing '{' in function definition.");
	}

	if (!parser_compound_statement (parser, func_code, UPPER_TYPE_PLAIN, -1)) {
		return 0;
	}

	/* Check the last opcode, if it's not a RETURN, we need to push one. */
	last = code_last_opcode (func_code);
	if (OPCODE_OP (last) != OP_RETURN) {
		if (ret_type != OBJECT_TYPE_VOID) {
			return parser_syntax_error (parser,
										"non-void func must return a value.");
		}

		if (!parser_push_dummy_return (parser, func_code)) {
			return 0;
		}
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
	token_t *temp;

	type = parser_token_object_type (parser);
	if (type == -1) {
		return parser_syntax_error (parser, "unknown type.");
	}

	parser_next_token (parser);
	if (!parser_check (parser, TOKEN_IDENTIFIER)) {
		return parser_syntax_error (parser, "missing identifier name.");
	}

	id = TOKEN_ID (parser->token);
	temp = parser_next_no_free (parser);
	if (parser_check (parser, TOKEN ('('))) {
		/* Goes to function-definition. */
		if (!parser_function_definition (parser, code, type, id)) {
			return 0;
		}

		lex_token_free (temp);

		return 1;
	}

	/* Goes to declaration. */
	if (type == OBJECT_TYPE_VOID) {
		return parser_syntax_error (parser, "variable can not be a void.");
	}

	if (!parser_declaration (parser, code, type, id)) {
		return 0;
	}

	lex_token_free (temp);

	return 1;
}

/* translation-unit:
 * external-declaration
 * external-declaration translation-unit */
static int
parser_translation_unit (parser_t *parser, code_t *code)
{
	uint32_t line;

	while (!parser_check (parser, TOKEN_END)) {
		if (!parser_external_declaration (parser, code)) {
			return 0;
		}
	}

	/* Emit a END_PROGRAM. */
	line = TOKEN_LINE (parser->token);

	return code_push_opcode (code, OPCODE (OP_END_PROGRAM, 0), line);
}

static int
parser_check_source (const char *path)
{
	if (!misc_check_source_extension (path)) {
		error ("source file extension must be \".k\".");

		return 0;
	}

	if (!misc_check_file_access (path, 1, 0)) {
		error ("file doesn't exist or no access: %s.", path);

		return 0;
	}

	return 1;
}

static int
parser_check_binary (const char *path)
{
	char *f;
	size_t len;
	int res;

	len = strlen (path);
	f = (char *) pool_alloc (len);
	if (f == NULL) {
		return -1;
	}

	strcpy (f, path);
	f[len - 1] = 'b';
	if (!misc_check_file_access (f, 0, 0)) {
		pool_free ((void *) f);

		return 0;
	}
	if (!misc_check_file_access (f, 1, 1)) {
		pool_free ((void *) f);

		return -1;
	}
	
	res = misc_file_is_older (path, f);
	pool_free ((void *) f);

	return res;
}

code_t *
parser_load_file (const char *path)
{
	code_t *code;
	parser_t *parser;
	int bin_stat;
	FILE *f;

	if (!parser_check_source (path)) {
		return NULL;
	}

	bin_stat = parser_check_binary (path);
	if (bin_stat == -1) {
		error ("no access or can not stat binary: %s", path);

		return NULL;
	}
	else if (bin_stat == 1) {
		return code_load_binary (path, NULL);
	}

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

	if (parser->token != NULL) {
		lex_token_free (parser->token);
	}
	parser_free (parser);

	code_save_binary (code);

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

	if (parser->token != NULL) {
		lex_token_free (parser->token);
	}
	parser_free (parser);

	return code;
}
