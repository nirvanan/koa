/*
 * frame.c
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

#include "frame.h"
#include "pool.h"
#include "dict.h"
#include "object.h"
#include "strobject.h"
#include "vecobject.h"
#include "error.h"

#define FRAME_UPPER(x) ((frame_t *)LIST_NEXT(x))

frame_t *
frame_new (code_t *code, frame_t *current, sp_t top, int global)
{
	frame_t *frame;

	frame = (frame_t *) pool_calloc (1, sizeof (frame_t));
	if (frame == NULL) {
		error ("out of memory.");

		return NULL;
	}

	frame = (frame_t *) list_append (LIST (current), LIST (frame));
	frame->code = code;
	frame->bottom = top;
	frame_make_block (frame);
	frame->global = global? frame->global: FRAME_UPPER (frame)->global;
	frame->is_global = global;

	return frame;
}

static int
frame_block_cleanup_fun (list_t *list, void *data)
{
	block_t *block;

	UNUSED (data);
	block = (block_t *) list;
	dict_free (block->ns);

	return 1;
}

frame_t *
frame_free (frame_t *frame)
{
	frame_t *upper;

	upper = FRAME_UPPER (frame);
	list_cleanup (LIST (frame->current), frame_block_cleanup_fun, 1, NULL);
	pool_free ((void *) frame);

	return upper;
}

opcode_t
frame_next_opcode (frame_t *frame)
{
	return code_get_pos (frame->code, frame->esp++);
}

void
frame_jump (frame_t *frame, para_t pos)
{
	frame->esp = pos;
}

void
frame_traceback (frame_t *frame)
{
	printf ("\t%s in %s: line %d\n",
			code_get_name (frame->code),
			code_get_filename (frame->code),
			code_get_line (frame->code, frame->esp));

	if (FRAME_UPPER (frame) != NULL) {
		frame_traceback (FRAME_UPPER (frame));
	}
}


static uint64_t
frame_varname_hash_fun (void *data)
{
	return strobject_get_hash ((object_t *) data);
}

static int
frame_varname_test_fun (void *value, void *hd)
{
	return strobject_equal ((object_t *) value, (object_t *) hd);
}

int
frame_make_block (frame_t *frame)
{
	dict_t *ns;
	block_t *block;

	ns = dict_new (frame_varname_hash_fun, frame_varname_test_fun);
	if (ns == NULL) {
		return 0;
	}

	block = (block_t *) pool_calloc (1, sizeof (block_t));
	if (block == NULL) {
		dict_free (ns);
		error ("out of memory.");

		return 0;
	}

	block->ns = ns;
	frame->current = (block_t *) list_append (LIST (frame->current), LIST (block));

	return 1;
}

int
frame_store_local (frame_t *frame, object_t *name, object_t *value)
{
	dict_t *ns;

	ns = frame->current->ns;
	/* Check whether this var has alreay declared. */
	if (dict_get (ns, (void *) name) != NULL) {
		error ("try redefine variable.");

		return 0;
	}

	if (dict_set (ns, (void *) name, (void *) value) != (void *) value) {
		return 0;
	}

	object_ref (value);

	return 1;
}

object_t *
frame_store_var (frame_t *frame, object_t *name, object_t *value)
{
	block_t *block;
	object_t *prev;

	block = frame->current;
	while (block != NULL) {
		prev = (object_t *) dict_get (block->ns, (void *) name);
		if (prev != NULL) {
			break;
		}

		block = (block_t *) LIST_NEXT (LIST (block));
	}

	/* Lookup global. */
	if (prev == NULL && !frame->is_global) {
		prev = (object_t *) dict_get (frame->global, (void *) name);
	}

	if (prev != NULL) {
		if (OBJECT_TYPE (value) != OBJECT_TYPE (prev)) {
			/* Try cast. */
			object_t *casted;

			casted = object_cast (value, OBJECT_TYPE (prev));
			if (casted == NULL) {
				return NULL;
			}


			if (dict_set (block->ns, (void *) name, (void *) casted) != prev) {
				return NULL;
			}

			object_ref (casted);

			return prev;
		}

		if (dict_set (block->ns, (void *) name, (void *) value) != prev) {
			return NULL;
		}

		object_ref (value);

		return prev;
	}

	error ("variable undefined: %s.", strobject_c_str (name));

	return NULL;
}

object_t *
frame_get_var (frame_t *frame, object_t *name)
{
	block_t *block;
	void *var;

	block = frame->current;
	while (block != NULL) {
		if ((var = dict_get (block->ns, name)) != NULL) {
			return (object_t *) var;
		}

		block = (block_t *) LIST_NEXT (LIST (block));
	}

	/* Lookup global. */
	if (!frame->is_global) {
		if ((var = dict_get (frame->global, name)) != NULL) {
			return (object_t *) var;
		}
	}

	error ("variable undefined: %s.", strobject_c_str (name));

	return NULL;
}

int
frame_bind_args (frame_t *frame, object_t *args)
{
	vec_t *v;
	size_t size;

	v = vecobject_get_value (args);
	if (!code_check_args (frame->code, v)) {
		return 0;
	}

	size = vec_size (v);
	for (integer_value_t i = 0; i < (integer_value_t) size; i++) {
		object_t *arg;
		object_t *name;

		arg = (object_t *) vec_pos (v, i);
		name = code_get_varname (frame->code, (para_t) i);
		if (!frame_store_local (frame, name, arg)) {
			return 0;
		}
	}

	return 1;
}