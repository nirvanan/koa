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
		vec_foreach (code->opcodes, code_vec_unref_fun);
		vec_free (code->opcodes);
	}
	if (code->lineinfo != NULL) {
		vec_foreach (code->lineinfo, code_vec_unref_fun);
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

int
code_push_opcode (code_t *code, opcode_t opcode, uint32_t line)
{
	object_t *op;
	object_t *li;

	op = longobject_new ((long) opcode, NULL);
	if (op == NULL) {
		return 0;
	}
	li = longobject_new ((long) line, NULL);
	if (li == NULL) {
		object_free (op);

		return 0;
	}

	if (!vec_push_back (code->opcodes, (void *) op)) {
		object_free (op);
		object_free (li);

		return 0;
	}
	if (!vec_push_back (code->lineinfo, (void *) li)) {
		UNUSED (vec_pop_back (code->opcodes));
		object_free (op);
		object_free (li);

		return 0;
	}

	object_ref (op);
	object_ref (li);

	return 1;
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

para_offset_t
code_push_const (code_t *code, object_t **var)
{
	size_t pos;

	/* Check const list size. */
	if (vec_size (code->consts) >= MAX_PARA) {
		error ("number of consts exceeded.");

		return -1;
	}

	/* Check whether there is already a var. */
	if ((pos = vec_find (code->consts, (void *) *var, code_var_find_fun)) != -1) {
		/* Free it. */
		object_free (*var);
		*var = NULL;

		return pos;
	}

	if (!vec_push_back (code->consts, (void *) *var)) {
		return -1;
	}

	object_ref (*var);

	/* Return the index of this new var. */
	return vec_size (code->consts) - 1;
}

para_offset_t
code_push_varname (code_t *code, const char *var)
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

	object_ref (name);

	/* Return the index of this new var. */
	return vec_size (code->varnames) - 1;
}

