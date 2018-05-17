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
#include "object.h"

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
	code->blocks = vec_new (0);
	if (code->opcodes == NULL || code->lineinfo == NULL ||
		code->consts == NULL || code->varnames == NULL ||
		code->blocks == NULL) {
		code_free (code);
		error ("out of memory.");

		return NULL;
	}

	return code;
}

void
code_free (code_t *code)
{
	size_t size;

	if (code->opcodes != NULL) {
		vec_free (code->opcodes);
	}
	if (code->lineinfo != NULL) {
		vec_free (code->lineinfo);
	}
	if (code->consts != NULL) {
		size = vec_size (code->consts);
		for (size_t i = 0; i < size; i++) {
			object_unref ((object_t *) vec_pos (code->consts, i));
		}
		vec_free (code->consts);
	}
	if (code->varnames != NULL) {
		size = vec_size (code->varnames);
		for (size_t i = 0; i < size; i++) {
			object_unref ((object_t *) vec_pos (code->varnames, i));
		}
		vec_free (code->varnames);
	}
	if (code->blocks != NULL) {
		size = vec_size (code->blocks);
		for (size_t i = 0; i < size; i++) {
			code_free ((code_t *) vec_pos (code->blocks, i));
		}
		vec_free (code->blocks);
	}

	pool_free ((void *) code);
}
