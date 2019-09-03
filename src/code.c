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
#include "object.h"
#include "longobject.h"
#include "strobject.h"
#include "vecobject.h"
#include "funcobject.h"

#define BINARY_HEADER "KOABIN"
#define BINARY_HEADER_LEN 6

static const char *g_code_names[] =
{
	"NULL",
	"OP_LOAD_CONST",
	"OP_STORE_LOCAL",
	"OP_STORE_VAR",
	"OP_STORE_EXCEPTION",
	"OP_LOAD_VAR",
	"OP_TYPE_CAST",
	"OP_VAR_INC",
	"OP_VAR_DEC",
	"OP_VAR_POINC",
	"OP_VAR_PODEC",
	"OP_NEGATIVE",
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
	"OP_BIND_ARGS",
	"OP_CON_SEL",
	"OP_LOGIC_OR",
	"OP_LOGIC_AND",
	"OP_BIT_OR",
	"OP_BIT_XOR",
	"OP_BIT_AND",
	"OP_EQUAL",
	"OP_NOT_EQUAL",
	"OP_LESS_THAN",
	"OP_LARGER_THAN",
	"OP_LESS_EQUAL",
	"OP_LARGER_EQUAL",
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
	"OP_PUSH_BLOCKS",
	"OP_POP_BLOCKS",
	"OP_JUMP_CASE",
	"OP_JUMP_DEFAULT",
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
		fatal_error ("out of memory.");
	}

	code->name = str_new (name, strlen (name));
	if (code->name == NULL) {
		code_free (code);
		fatal_error ("out of memory.");
	}

	/* Allocate all data segments. */
	code->opcodes = vec_new (0);
	code->lineinfo = vec_new (0);
	code->types = vec_new (0);
	code->consts = vec_new (0);
	code->varnames = vec_new (0);
	if (code->opcodes == NULL || code->lineinfo == NULL ||
		code->types == NULL || code->consts == NULL ||
		code->varnames == NULL) {
		code_free (code);

		return NULL;
	}

	return code;
}

void
code_set_func (code_t *code, uint32_t line, object_type_t ret_type)
{
	code->func = 1;
	code->lineno = line;
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
	if (code->types != NULL) {
		vec_foreach (code->types, code_vec_free_fun);
		vec_free (code->types);
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
		fatal_error ("out of memory.");
	}

	*op = opcode;

	li = pool_alloc (sizeof (uint32_t));
	if (li == NULL) {
		pool_free ((void *) op);
		fatal_error ("out of memory.");
	}

	*li = line;

	if (!vec_insert (code->opcodes, (integer_value_t) pos, (void *) op)) {
		pool_free ((void *) op);

		return -1;
	}
	if (!vec_insert (code->lineinfo, (integer_value_t) pos, (void *) li)) {
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

void
code_switch_opcode (code_t *code, para_t f, para_t s)
{
	void *t;

	t = vec_set (code->opcodes, (integer_value_t) f,
				 vec_pos (code->opcodes, (integer_value_t) s));
	t = vec_set (code->opcodes, (integer_value_t) s, t);
	t = vec_set (code->lineinfo, (integer_value_t) f,
				 vec_pos (code->lineinfo, (integer_value_t) s));
	t = vec_set (code->lineinfo, (integer_value_t) s, t);
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
	if ((pos = vec_find (code->consts,
		(void *) var, code_var_find_fun)) != -1) {
		*exist = 1;

		return pos;
	}

	if (!vec_push_back (code->consts, (void *) var)) {
		return -1;
	}

	*exist = 0;
	object_ref (var);

	/* Return the index of this new var. */
	return vec_size (code->consts) - 1;
}

para_t
code_push_varname (code_t *code, const char *var, object_type_t type, int para)
{
	object_t *name;
	size_t pos;
	object_type_t *var_type;

	/* Check var list size. */
	if (vec_size (code->varnames) >= MAX_PARA) {
		error ("number of vars exceeded.");

		return -1;
	}

	name = strobject_new (var, strlen (var), 0, NULL);
	if (name == NULL) {
		return -1;
	}

	/* Check whether there is already a var. */
	if (type == OBJECT_TYPE_VOID && (pos = vec_find (code->varnames,
		(void *) name, code_var_find_fun)) != -1) {
		object_free (name);

		return pos;
	}

	if (!vec_push_back (code->varnames, (void *) name)) {
		object_free (name);

		return -1;
	}

	var_type = (object_type_t *) pool_alloc (sizeof (object_type_t));
	if (var_type == NULL) {
		return -1;
	}

	*var_type = type;
	if (!vec_push_back (code->types, (void *) var_type)) {
		pool_free ((void *) var_type);

		return -1;
	}

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

uint32_t
code_get_line (code_t *code, para_t pos)
{
	uint32_t *line;

	line = (uint32_t *) vec_pos (code->lineinfo, (integer_value_t) pos);
	if (line == NULL) {
		return 0;
	}

	return *line;
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
		printf ("%lld\t%s\n", i, str_c_str (strobject_get_value (dump)));
		object_free (dump);
	}
}

void
code_print (code_t *code)
{
	size_t size;

	if (code->func) {
		printf ("parameters: %d\n", code->args);
	}

	/* Print consts. */
	printf ("consts:\n");
	code_print_object_vec (code->consts);

	/* Print varnames. */
	printf ("varnames:\n");
	code_print_object_vec (code->varnames);

	/* Print opcodes. */
	size = vec_size (code->opcodes);
	printf ("opcodes:\nPos\tLine\tOP\t\t\tPara\n");
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		opcode_t *opcode;
		uint32_t *line;

		opcode = (opcode_t *) vec_pos (code->opcodes, i);
		line = (uint32_t *) vec_pos (code->lineinfo, i);
		printf ("%lld\t%u\t%-16s\t%d\n",
				i, *line, g_code_names[OPCODE_OP (*opcode)],
				OPCODE_PARA (*opcode));
	}

	/* Print function codes. */
	size = vec_size (code->consts);
	printf ("\n\n");
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *obj;

		obj = (object_t *) vec_pos (code->consts, i);
		if (OBJECT_IS_FUNC (obj)) {
			code_t *func;

			func = funcobject_get_value (obj);
			printf ("func %s:\n", code_get_name (func));
			code_print (func);
		}
	}
}

static object_t *
code_vec_to_binary (vec_t *vec, size_t el)
{
	size_t size;
	object_t *temp;

	size = vec_size (vec);
	temp = strobject_new (BINARY (size), sizeof (size_t), 1, NULL);
	if (temp == NULL) {
		return NULL;
	}

	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *res;
		object_t *obj;

		obj = strobject_new (vec_pos (vec, i), el, 1, NULL);
		if (obj == NULL) {
			object_free (temp);

			return NULL;
		}

		res = object_add (temp, obj);
		object_free (temp);
		object_free (obj);
		if (res == NULL) {
			return NULL;
		}
		
		temp = res;
	}
	
	return temp;
}

static object_t *
code_object_to_binary (vec_t *vec)
{
	object_t *obj;
	object_t *res;

	obj = vecobject_vec_new (vec, NULL);
	if (obj == NULL) {
		return NULL;
	}

	res = object_binary (obj);
	/* Data can not be freed. */
	pool_free ((void *) obj);

	return res;
}

static object_t *
code_str_to_binary (str_t *str)
{
	object_t *obj;
	object_t *res;

	obj = strobject_str_new (str, NULL);
	if (obj == NULL) {
		return NULL;
	}

	res = object_binary (obj);
	/* Data can not be freed. */
	pool_free ((void *) obj);

	return res;
}

static object_t *
code_binary_concat (object_t *obj1, object_t *obj2)
{
	object_t *res;

	res = object_add (obj1, obj2);
	object_free (obj1);
	object_free (obj2);

	return res;
}

object_t *
code_binary (code_t *code)
{
	object_t *cur;
	object_t *temp;

	/* Dump opcodes. */
	cur = code_vec_to_binary (code->opcodes, sizeof (opcode_t));
	if (cur == NULL) {
		return NULL;
	}

	/* Dump lineinfo. */
	temp = code_vec_to_binary (code->lineinfo, sizeof (uint32_t));
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump types. */
	temp = code_vec_to_binary (code->types, sizeof (object_type_t));
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump consts. */
	temp = code_object_to_binary (code->consts);
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump varnames. */
	temp = code_object_to_binary (code->varnames);
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump name. */
	temp = code_str_to_binary (code->name);
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump filename. */
	temp = code_str_to_binary (code->filename);
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump func. */
	temp = strobject_new (BINARY (code->func), sizeof (int), 1, NULL);
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump lineno. */
	temp = strobject_new (BINARY (code->lineno), sizeof (int), 1, NULL);
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump args. */
	temp = strobject_new (BINARY (code->args), sizeof (int), 1, NULL);
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	/* Dump ret_type. */
	temp = strobject_new (BINARY (code->ret_type), sizeof (object_type_t), 1, NULL);
	if (temp == NULL) {
		object_free (cur);

		return NULL;
	}
	cur = code_binary_concat (cur, temp);
	if (cur == NULL) {
		return cur;
	}

	return cur;
}

static int
code_write_header (FILE *b)
{
	size_t len;

	len = strlen (BINARY_HEADER);

	return fwrite (BINARY_HEADER, sizeof (char), len, b) == len;
}

int
code_save_binary (code_t *code)
{
	char *path;
	size_t len;
	FILE *b;
	object_t *obj;
	str_t *str;

	len = str_len (code->filename);
	path = (char *) pool_alloc (len + 1);
	if (path == NULL) {
		fatal_error ("out of memory.");
	}

	/* Binary is saved as ".b". */
	strcpy (path, str_c_str (code->filename));
	path[len - 1] = 'b';
	b = fopen (path, "wb");
	if (b == NULL) {
		error ("can't open binary file: %s.", path);
		pool_free ((void *) path);

		return 0;
	}

	if (!code_write_header (b)) {
		pool_free ((void *) path);
		UNUSED (fclose (b));
	}

	obj = code_binary (code);
	if (obj == NULL) {
		UNUSED (fclose (b));
		error ("saving binary header failed: %s.", path);
		pool_free ((void *) path);

		return 0;
	}

	str = strobject_get_value (obj);
	len = str_len (str);
	if (fwrite ((const void *) str_c_str (str), sizeof (char), len, b) != len) {
		UNUSED (fclose (b));
		object_free (obj);
		error ("saving binary failed: %s.", path);
		pool_free ((void *) path);

		return 0;
	}

	pool_free ((void *) path);
	UNUSED (fclose (b));
	object_free (obj);

	return 1;
}

static vec_t *
code_binary_to_vec (FILE *f, size_t el)
{
	size_t size;
	vec_t *vec;

	vec = vec_new (0);
	if (vec == NULL) {
		return NULL;
	}

	if (fread (&size, sizeof (size_t), 1, f) != 1) {
		vec_free (vec);
		error ("read binary failed.");

		return NULL;
	}

	for (int i = 0; i < size; i++) {
		void *data;

		data = pool_alloc (el);
		if (fread (data, el, 1, f) != 1 ||
			!vec_push_back (vec, data)) {
			pool_free (data);
			vec_free (vec);

			return NULL;
		}
	}

	return vec;
}

static vec_t *
code_binary_to_object (FILE *f)
{
	vec_t *vec;
	object_t *obj;

	obj = object_load_binary (f);
	if (obj == NULL) {
		return NULL;
	}
	vec = vecobject_get_value (obj);
	pool_free ((void *) obj);

	return vec;
}

static str_t *
code_binary_to_str (FILE *f)
{
	str_t *str;
	object_t *obj;

	obj = object_load_binary (f);
	if (obj == NULL) {
		return NULL;
	}
	str = strobject_get_value (obj);
	pool_free ((void *) obj);

	return str;
}

static int
code_load_header (FILE *f)
{
	char header[BINARY_HEADER_LEN];

	if (fread ((void *) header, BINARY_HEADER_LEN, 1, f) != 1) {
		error ("read binary header failed.");

		return 0;
	}

	if (memcmp (header, BINARY_HEADER, BINARY_HEADER_LEN) != 0) {
		error ("invalid binary header.");

		return 0;
	}

	return 1;
}

code_t *
code_load_binary (const char *path, FILE *f)
{
	code_t *code;
	FILE *b;

	if (f == NULL) {
		char *bin_path;
		size_t len;

		len = strlen (path);
		bin_path = (char *) pool_alloc (len + 1);
		if (bin_path == NULL) {
			fatal_error ("out of memory.");
		}

		strcpy (bin_path, path);
		bin_path[len - 1] = 'b';
		b = fopen (bin_path, "rb");
		pool_free ((void *) bin_path);
		if (b == NULL) {
			error ("failed to open binary: %s", path);

			return NULL;
		}
	}
	else {
		b = f;
	}

	code = (code_t *) pool_calloc (1, sizeof (code_t));
	if (code == NULL) {
		if (f == NULL) {
			UNUSED (fclose (b));
		}
		fatal_error ("out of memory.");
	}

	if (f == NULL && !code_load_header (b)) {
		UNUSED (fclose (b));

		return NULL;
	}

	code->opcodes = code_binary_to_vec (b, sizeof (opcode_t));
	code->lineinfo = code_binary_to_vec (b, sizeof (uint32_t));
	code->types = code_binary_to_vec (b, sizeof (object_type_t));
	code->consts = code_binary_to_object (b);
	code->varnames = code_binary_to_object (b);
	code->name = code_binary_to_str (b);
	code->filename = code_binary_to_str (b);
	if (code->opcodes == NULL || code->lineinfo == NULL ||
		code->types == NULL || code->consts == NULL ||
		code->varnames == NULL || code->name == NULL ||
		code->filename == NULL) {
		code_free (code);
		if (f == NULL) {
			UNUSED (fclose (b));
		}

		return NULL;
	}

	if (fread (&code->func, sizeof (int), 1, b) != 1 ||
		fread (&code->lineno, sizeof (int), 1, b) != 1 ||
		fread (&code->args, sizeof (int), 1, b) != 1 ||
		fread (&code->ret_type, sizeof (object_type_t), 1, b) != 1) {
		code_free (code);
		if (f == NULL) {
			UNUSED (fclose (b));
		}

		return NULL;
	}

	if (f == NULL) {
		UNUSED (fclose (b));
	}

	return code;
}

object_t *
code_get_const (code_t *code, para_t pos)
{
	return (object_t *) vec_pos (code->consts, (integer_value_t) pos);
}

object_t *
code_get_varname (code_t *code, para_t pos)
{
	return (object_t *) vec_pos (code->varnames, (integer_value_t) pos);
}

int
code_check_args (code_t *code, vec_t *args)
{
	size_t size;

	size = vec_size (args);
	if (size != code->args) {
		error ("wrong number of arguments, required: %d, passed: %d.", code->args, size);

		return 0;
	}
	for (size_t i = 0; i < size; i++) {
		object_t *arg;
		object_type_t *type;

		arg = (object_t *) vec_pos (args, (integer_value_t) i);
		type = (object_type_t *) vec_pos (code->types, (integer_value_t) i);
		if (OBJECT_TYPE (arg) != *type) {
			error ("wrong argument type at position %d.", i + 1);

			return 0;
		}
	}

	return 1;
}

object_type_t
code_get_vartype (code_t *code, para_t pos)
{
	return *(object_type_t *) vec_pos (code->types, pos);
}
