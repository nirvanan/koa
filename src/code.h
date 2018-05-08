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

#include "koa.h"
#include "vec.h"
#include "str.h"

/* Code is a static structure, it can represent a function, a code block,
 * or a module. */
typedef struct code_s {
	vec_t *opcodes; /* All op codes in this block. */
	vec_t *lineinfo; /* Line numbers of all codes. */
	vec_t *consts; /* All consts appears in this block. */
	vec_t *varnames; /* The names of local variables (args included). */
	vec_t *blocks; /* All nested code blocks. */
	str_t *name; /* Reference name of this block (or function name). */
	str_t *filename; /* File name of this block. */
	int args; /* Number of arguments. */
	int fun; /* Is this code representing a function? */
	int lineno; /* The first line number of this block. */
	int depth; /* The depth of current block. */
} code_t;

code_t *
code_new (const char *filename, const char *name);

#endif /* CODE_H */
