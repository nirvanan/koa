/*
 * code.c
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
#include <string.h>

#include "code.h"
#include "error.h"
#include "pool.h"
#include "longobject.h"
#include "strobject.h"

static const char *g_code_names[] =
{
	"NULL",
	"OP_LOAD_CONST",
	"OP_STORE_LOCAL",
	"OP_STORE_VAR",
	"OP_LOAD_VAR",
	"OP_FUNC_RETURN",
	"OP_TYPE_CAST",
	"OP_VAR_INC",
	"OP_VAR_DEC",
	"OP_VAR_POINC",
	"OP_VAR_PODEC",
	"OP_VALUE_NEG",
	"OP_BIT_NOT",
	"OP_LOGIC_NOT",
	"OP_POP_STACK",
	"OP_LOAD_INDEX",
	"OP_STORE_INDEX",
	"OP_INDEX_INC",
	"OP_INDEX_DEC",
	"OP_INDEX_POINC",
	"OP_INDEX_PODEC",
	"OP_MAKE_VEC",
	"OP_CALL_FUNC",
	"OP_CON_SEL",
	"OP_LOGIC_OR",
	"OP_LOGIC_AND",
	"OP_BIT_OR",
	"OP_BIT_XOR",
	"OP_BIT_AND",
	"OP_EQUAL",
	"OP_NOT_EQUAL",
	"OP_LESS_THAN",
	"OP_LARGE_THAN",
	"OP_LESS_EQUAL",
	"OP_LARGE_EQUAL",
	"OP_LEFT_SHIFT",
	"OP_RIGHT_SHIFT",
	"OP_ADD",
	"OP_SUB",
	"OP_MUL",
	"OP_DIV",
	"OP_MOD",
	"OP_VAR_IPMUL",
	"OP_VAR_IPDIV",
	"OP_VAR_IPMOD",
	"OP_VAR_IPADD",
	"OP_VAR_IPSUB",
	"OP_VAR_IPLS",
	"OP_VAR_IPRS",
	"OP_VAR_IPAND",
	"OP_VAR_IPXOR",
	"OP_VAR_IPOR",
	"OP_INDEX_IPMUL",
	"OP_INDEX_IPDIV",
	"OP_INDEX_IPMOD",
	"OP_INDEX_IPADD",
	"OP_INDEX_IPSUB",
	"OP_INDEX_IPLS",
	"OP_INDEX_IPRS",
	"OP_INDEX_IPAND",
	"OP_INDEX_IPXOR",
	"OP_INDEX_IPOR",
	"OP_JUMP_FALSE",
	"OP_JUMP_FORCE",
	"OP_ENTER_BLOCK",
	"OP_LEAVE_BLOCK",
	"OP_JUMP_CONTINUE",
	"OP_JUMP_BREAK",
	"OP_RETURN",
	"OP_CASE_BLOCK",
	"OP_JUMP_CASE",
	"OP_JUMP_TRUE",
	"OP_END_PROGRAM"
};

code_t *
code_new (const char *filename, const char *name)
{
	code_t *code;

	code = (code_t *) pool_calloc (1, sizeof (code_t));
	if (code == NULL) {
		error ("failed to alloc code.");

		return NULL;
	}

	code->filename = str_new (filename, strlen (filename));
	if (code->filename == NULL) {
		code_free (code);
		error ("out of memory.");

		return NULL;
	}

	code->name = str_new (name, strlen (name));
	if (code->name == NULL) {
		code_free (code);
		error ("out of memory.");

		return NULL;
	}

	/* Allocate all data segments. */
	code->opcodes = vec_new (0);
	code->lineinfo = vec_new (0);
	code->consts = vec_new (0);
	code->varnames = vec_new (0);
	if (code->opcodes == NULL || code->lineinfo == NULL ||
		code->consts == NULL || code->varnames == NULL) {
		code_free (code);
		error ("out of memory.");

		return NULL;
	}

	return code;
}

void
code_set_fun (code_t *code, object_type_t ret_type)
{
	code->fun = 1;
	code->ret_type = ret_type;
}

static int
code_vec_free_fun (void *data)
{
	pool_free (data);

	return 0;
}

static int
code_vec_unref_fun (void *data)
{
	object_unref ((object_t *) data);

	return 0;
}

void
code_free (code_t *code)
{
	if (code->opcodes != NULL) {
		vec_foreach (code->opcodes, code_vec_free_fun);
		vec_free (code->opcodes);
	}
	if (code->lineinfo != NULL) {
		vec_foreach (code->lineinfo, code_vec_free_fun);
		vec_free (code->lineinfo);
	}
	if (code->consts != NULL) {
		vec_foreach (code->consts, code_vec_unref_fun);
		vec_free (code->consts);
	}
	if (code->varnames != NULL) {
		vec_foreach (code->varnames, code_vec_unref_fun);
		vec_free (code->varnames);
	}
	if (code->filename != NULL) {
		str_free (code->filename);
	}
	if (code->name != NULL) {
		str_free (code->name);
	}

	pool_free ((void *) code);
}

para_t
code_insert_opcode (code_t *code, para_t pos, opcode_t opcode, uint32_t line)
{
	opcode_t *op;
	uint32_t *li;

	/* Check const list size. */
	if (vec_size (code->opcodes) >= MAX_PARA) {
		error ("number of opcodes exceeded.");

		return -1;
	}

	op = (opcode_t *) pool_alloc (sizeof (opcode_t));
	if (op == NULL) {
		error ("out of memory.");

		return -1;
	}

	*op = opcode;

	li = pool_alloc (sizeof (uint32_t));
	if (li == NULL) {
		pool_free ((void *) op);
		error ("out of memory.");

		return -1;
	}

	*li = line;

	if (!vec_insert (code->opcodes, (integer_value_t) pos, (void *) op)) {
		pool_free ((void *) op);

		return -1;
	}
	if (!vec_insert (code->lineinfo, (integer_value_t) pos, (void *) li)) {
		UNUSED (vec_remove (code->opcodes, (integer_value_t) pos));
		pool_free ((void *) op);
		pool_free ((void *) li);

		return -1;
	}

	return (para_t) vec_size (code->opcodes);
}

para_t
code_push_opcode (code_t *code, opcode_t opcode, uint32_t line)
{
	return code_insert_opcode (code,
		(para_t) vec_size (code->opcodes), opcode, line);
}


static int
code_var_find_fun (void *a, void *b)
{
	object_t *res;
	int eq;

	res = object_equal ((object_t *) a, (object_t *) b);
	if (res == 0) {
		return 0;
	}

	eq = object_get_integer (res);
	object_free (res);

	return eq != 0;
}

para_t
code_push_const (code_t *code, object_t *var, int *exist)
{
	size_t pos;

	/* Check const list size. */
	if (vec_size (code->consts) >= MAX_PARA) {
		error ("number of consts exceeded.");

		return -1;
	}

	/* Check whether there is already a var. */
	if ((pos = vec_find (code->consts, (void *) var, code_var_find_fun)) != -1) {
		*exist = 1;

		return pos;
	}

	if (!vec_push_back (code->consts, (void *) var)) {
		return -1;
	}

	*exist = 0;

	/* Return the index of this new var. */
	return vec_size (code->consts) - 1;
}

para_t
code_push_varname (code_t *code, const char *var, int para)
{
	object_t *name;
	size_t pos;

	/* Check var list size. */
	if (vec_size (code->varnames) >= MAX_PARA) {
		error ("number of vars exceeded.");

		return -1;
	}

	name = strobject_new (var, NULL);
	if (name == NULL) {
		return -1;
	}

	/* Check whether there is already a var. */
	if ((pos = vec_find (code->varnames, (void *) name, code_var_find_fun)) != -1) {
		object_free (name);

		return pos;
	}

	if (!vec_push_back (code->varnames, (void *) name)) {
		object_free (name);

		return -1;
	}

	/* Is this var a parameter? */
	if (para) {
		code->args++;
	}

	object_ref (name);

	/* Return the index of this new var. */
	return vec_size (code->varnames) - 1;
}

opcode_t
code_last_opcode (code_t *code)
{
	if (!vec_size (code->opcodes)) {
		return (opcode_t) 0;
	}

	return *((opcode_t *) vec_last (code->opcodes));
}

int
code_modify_opcode (code_t *code, para_t pos,
					opcode_t opcode, uint32_t line)
{
	size_t size;
	para_t p;
	opcode_t *prev_opcode;
	uint32_t *prev_line;

	if (!(size = vec_size (code->opcodes))) {
		return 0;
	}

	p = pos;
	if (p == -1) {
		p = size - 1;
	}

	prev_opcode = (opcode_t *) vec_pos (code->opcodes, (integer_value_t) p);
	*prev_opcode = opcode;
	if (line != 0) {
		prev_line = (uint32_t *) vec_pos (code->lineinfo, (integer_value_t) p);
		*prev_line = line;
	}
	
	return 1;
}

para_t
code_current_pos (code_t *code)
{
	return (para_t) vec_size (code->opcodes) - 1;
}

opcode_t
code_get_pos (code_t *code, para_t pos)
{
	opcode_t *opcode;

	opcode = (opcode_t *) vec_pos (code->opcodes, (integer_value_t) pos);

	if (opcode == NULL) {
		return (opcode_t) 0;
	}

	return *opcode;
}

int
code_remove_pos (code_t *code, para_t pos)
{
	return vec_remove (code->opcodes, (integer_value_t) pos) &&
		vec_remove (code->lineinfo, (integer_value_t) pos);
}

const char *
code_get_filename (code_t *code)
{
	return str_c_str (code->filename);
}

const char *
code_get_name (code_t *code)
{
	return str_c_str (code->name);
}

static void
code_print_object_vec (vec_t *vec)
{
	size_t size;
	object_t *obj;
	object_t *dump;

	size = vec_size (vec);
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		obj = (object_t *) vec_pos (vec, i);
		dump = object_dump (obj);
		printf ("%ld\t%s\n", i, str_c_str (strobject_get_value (dump)));
		object_free (dump);
	}
}

void
code_print (code_t *code)
{
	size_t size;

	/* Print consts. */
	printf ("consts:\n");
	code_print_object_vec (code->consts);

	/* Print varnames. */
	printf ("varnames:\n");
	code_print_object_vec (code->varnames);

	/* Print opcodes. */
	size = vec_size (code->opcodes);
	printf ("opcodes:\n");
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		opcode_t *opcode;
		uint32_t *line;

		opcode = (opcode_t *) vec_pos (code->opcodes, i);
		line = (uint32_t *) vec_pos (code->lineinfo, i);
		printf ("%u\t%s\t\t%d\n",
				*line, g_code_names[OPCODE_OP (*opcode)],
				OPCODE_PARA (*opcode));
	}
}
