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

#include <string.h>

#include "code.h"
#include "error.h"
#include "pool.h"
#include "longobject.h"
#include "strobject.h"

code_t *
code_new (const char *filename, const char *name)
{
	code_t *code;

	code = (code_t *) pool_alloc (sizeof (code_t));
	if (code == NULL) {
		error ("failed to alloc code.");

		return NULL;
	}

	code->filename = str_new (filename, strlen (filename));
	if (code->filename == NULL) {
		pool_free ((void *) code);
		error ("out of memory.");

		return NULL;
	}

	code->name = str_new (name, strlen (name));
	if (code->name == NULL) {
		pool_free ((void *) code->filename);
		pool_free ((void *) code);
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

	pool_free ((void *) code);
}

integer_value_t
code_push_opcode (code_t *code, opcode_t opcode, uint32_t line)
{
	opcode_t *op;
	uint32_t *li;

	op = (opcode_t *) pool_alloc (sizeof (opcode_t));
	if (op == NULL) {
		error ("out of memory.");

		return 0;
	}
	*op = opcode;

	li = pool_alloc (sizeof (uint32_t));
	if (li == NULL) {
		pool_free ((void *) op);
		error ("out of memory.");

		return 0;
	}
	*li = line;

	if (!vec_push_back (code->opcodes, (void *) op)) {
		pool_free ((void *) op);

		return 0;
	}
	if (!vec_push_back (code->lineinfo, (void *) li)) {
		UNUSED (vec_pop_back (code->opcodes));
		pool_free ((void *) op);
		pool_free ((void *) li);

		return 0;
	}

	return (integer_value_t) vec_size (code->opcodes);
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
code_modify_opcode (code_t *code, integer_value_t pos,
					opcode_t opcode, uint32_t line)
{
	size_t size;
	integer_value_t p;
	opcode_t *prev_opcode;
	uint32_t *prev_line;

	if (!(size = vec_size (code->opcodes))) {
		return 0;
	}

	p = pos;
	if (p == -1) {
		p = size;
	}

	prev_opcode = (opcode_t *) vec_pos (code->opcodes, p);
	*prev_opcode = opcode;
	prev_line = (opcode_t *) vec_pos (code->lineinfo, p);
	*prev_line = line;
	
	return 1;
}

integer_value_t
code_current_pos (code_t *code)
{
	return vec_size (code->opcodes) - 1;
}

int
code_remove_pos (code_t *code,integer_value_t pos)
{
	return vec_remove (code->opcodes, pos);
}

