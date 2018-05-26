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

#define OPCODE(o,p) (((o)<<PARA_BITS)&(p))

#define OPCODE_OP(x) ((x)>>PARA_BITS)

#define OPCODE_PARA(x) ((x)&PARA_MASK)

typedef int32_t para_t;

typedef uint32_t opcode_t;

typedef enum op_e {
	OP_LOAD_CONST = 0x01,
	OP_STORE_LOCAL,
	OP_STORE_VAR,
	OP_LOAD_VAR,
	OP_FUNC_RETURN,
	OP_TYPE_CAST,
	OP_VAR_INC,
	OP_VAR_DEC,
	OP_VALUE_NEG,
	OP_BIT_NOT,
	OP_LOGIC_NOT,
	OP_POP_STACK,
	OP_LOAD_INDEX,
	OP_INDEX_INC,
	OP_INDEX_DEC,
	OP_MAKE_VEC,
	OP_CALL_FUNC
} op_t;

/* Code is a static structure, it can represent a function, or a module. */
typedef struct code_s {
	vec_t *opcodes; /* All op codes in this block. */
	vec_t *lineinfo; /* Line numbers of all codes. */
	vec_t *consts; /* All consts appears in this block. */
	vec_t *varnames; /* The names of local variables (args included). */
	str_t *name; /* Reference name of this block (or function name). */
	str_t *filename; /* File name of this block. */
	int args; /* Number of arguments. */
	int fun; /* Is this code representing a function? */
	int lineno; /* The first line number of this block. */
	object_type_t ret_type;
} code_t;

code_t *
code_new (const char *filename, const char *name);

void
code_set_fun (code_t *code, object_type_t ret_type);

void
code_free (code_t *code);

integer_value_t
code_push_opcode (code_t *code, opcode_t opcode, uint32_t line);

para_t
code_push_const (code_t *code, object_t *var, int *exist);

para_t
code_push_varname (code_t *code, const char *var, int para);

opcode_t
code_last_opcode (code_t *code);

int
code_modify_opcode (code_t *code, integer_value_t pos,
					opcode_t opcode, uint32_t line);

#endif /* CODE_H */
