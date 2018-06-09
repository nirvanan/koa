/*
 * code.h
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

#ifndef CODE_H
#define CODE_H

#include <stdint.h>

#include "koa.h"
#include "vec.h"
#include "str.h"
#include "object.h"

#define MAX_PARA (0x00ffffff)
#define PARA_BITS 24
#define PARA_MASK MAX_PARA

#define OPCODE(o,p) (((o)<<PARA_BITS)|(p))

#define OPCODE_OP(x) ((x)>>PARA_BITS)

#define OPCODE_PARA(x) ((x)&PARA_MASK)

#define OPCODE_IS_JUMP(x) (OPCODE_OP(x)==OP_JUMP_FALSE||\
	OPCODE_OP(x)==OP_JUMP_FORCE||\
	OPCODE_OP(x)==OP_JUMP_CONTINUE||\
	OPCODE_OP(x)==OP_JUMP_BREAK||\
	OPCODE_OP(x)==OP_JUMP_CASE||\
	OPCODE_OP(x)==OP_JUMP_DEFAULT)

#define FUNC_RET_TYPE(x) ((x)->ret_type)

typedef int32_t para_t;

typedef uint32_t opcode_t;

typedef enum op_e 
{
	OP_LOAD_CONST = 0x01,
	OP_STORE_LOCAL,
	OP_STORE_VAR,
	OP_LOAD_VAR,
	OP_FUNC_RETURN,
	OP_TYPE_CAST,
	OP_VAR_INC,
	OP_VAR_DEC,
	OP_VAR_POINC,
	OP_VAR_PODEC,
	OP_VALUE_NEG,
	OP_BIT_NOT,
	OP_LOGIC_NOT,
	OP_POP_STACK,
	OP_LOAD_INDEX,
	OP_STORE_INDEX,
	OP_INDEX_INC,
	OP_INDEX_DEC,
	OP_INDEX_POINC,
	OP_INDEX_PODEC,
	OP_MAKE_VEC,
	OP_CALL_FUNC,
	OP_BIND_ARGS,
	OP_CON_SEL,
	OP_LOGIC_OR,
	OP_LOGIC_AND,
	OP_BIT_OR,
	OP_BIT_XOR,
	OP_BIT_AND,
	OP_EQUAL,
	OP_NOT_EQUAL,
	OP_LESS_THAN,
	OP_LARGE_THAN,
	OP_LESS_EQUAL,
	OP_LARGE_EQUAL,
	OP_LEFT_SHIFT,
	OP_RIGHT_SHIFT,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_VAR_IPMUL,
	OP_VAR_IPDIV,
	OP_VAR_IPMOD,
	OP_VAR_IPADD,
	OP_VAR_IPSUB,
	OP_VAR_IPLS,
	OP_VAR_IPRS,
	OP_VAR_IPAND,
	OP_VAR_IPXOR,
	OP_VAR_IPOR,
	OP_INDEX_IPMUL,
	OP_INDEX_IPDIV,
	OP_INDEX_IPMOD,
	OP_INDEX_IPADD,
	OP_INDEX_IPSUB,
	OP_INDEX_IPLS,
	OP_INDEX_IPRS,
	OP_INDEX_IPAND,
	OP_INDEX_IPXOR,
	OP_INDEX_IPOR,
	OP_JUMP_FALSE,
	OP_JUMP_FORCE,
	OP_ENTER_BLOCK,
	OP_LEAVE_BLOCK,
	OP_JUMP_CONTINUE,
	OP_JUMP_BREAK,
	OP_RETURN,
	OP_PUSH_BLOCKS,
	OP_POP_BLOCKS,
	OP_JUMP_CASE,
	OP_JUMP_DEFAULT,
	OP_JUMP_TRUE,
	OP_END_PROGRAM
} op_t;

/* Code is a static structure, it can represent a function, or a module. */
typedef struct code_s {
	vec_t *opcodes; /* All op codes in this block. */
	vec_t *lineinfo; /* Line numbers of all codes. */
	vec_t *types; /* Type of local variables. */
	vec_t *consts; /* All consts appears in this block. */
	vec_t *varnames; /* The names of local variables (args included). */
	str_t *name; /* Reference name of this block (or function name). */
	str_t *filename; /* File name of this block. */
	int fun; /* Is this code representing a function? */
	int lineno; /* The first line number of this block. */
	object_type_t ret_type; /* Return type of function. */
} code_t;

typedef enum upper_type_e
{
	UPPER_TYPE_PLAIN,
	UPPER_TYPE_FOR,
	UPPER_TYPE_DO,
	UPPER_TYPE_WHILE,
	UPPER_TYPE_SWITCH
} upper_type_t;

code_t *
code_new (const char *filename, const char *name);

void
code_set_func (code_t *code, uint32_t line, object_type_t ret_type);

void
code_free (code_t *code);

int
code_insert_opcode (code_t *code, para_t pos,
					opcode_t opcode, uint32_t line);

para_t
code_push_opcode (code_t *code, opcode_t opcode, uint32_t line);

void
code_switch_opcode (code_t *code, para_t f, para_t s);

para_t
code_push_const (code_t *code, object_t *var, int *exist);

para_t
code_push_varname (code_t *code, const char *var, object_type_t type);

opcode_t
code_last_opcode (code_t *code);

int
code_modify_opcode (code_t *code, para_t pos,
					opcode_t opcode, uint32_t line);

para_t
code_current_pos (code_t *code);

opcode_t
code_get_pos (code_t *code, para_t pos);

uint32_t
code_get_line (code_t *code, para_t pos);

int
code_remove_pos (code_t *code, para_t pos);

const char *
code_get_filename (code_t *code);

const char *
code_get_name (code_t *code);

void
code_print (code_t *code);

object_t *
code_binary (code_t *code);

int
code_save_binary (code_t *code);

code_t *
code_load_binary (const char *path, FILE *f);

#endif /* CODE_H */
