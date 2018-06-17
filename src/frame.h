/*
 * frame.h
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

#ifndef FRAME_H
#define FRAME_H

#include "koa.h"
#include "list.h"
#include "dict.h"
#include "code.h"
#include "stack.h"

typedef struct block_s
{
	list_t link;
	dict_t *ns;
} block_t;

typedef struct frame_s
{
	list_t link;
	block_t *current;
	dict_t *global; /* Link to the first block. */
	int is_global;
	code_t *code;
	para_t esp;
	sp_t bottom;
} frame_t;

frame_t *
frame_new (code_t *code, frame_t *current, sp_t top, int global);

frame_t *
frame_free (frame_t *frame);

opcode_t
frame_next_opcode (frame_t *frame);

void
frame_jump (frame_t *frame, para_t pos);

void
frame_traceback (frame_t *frame);

int
frame_make_block (frame_t *frame);

int
frame_store_local (frame_t *frame, object_t *name, object_t *value);

object_t *
frame_store_var (frame_t *frame, object_t *name, object_t *value);

object_t *
frame_get_var (frame_t *frame, object_t *name);

int
frame_bind_args (frame_t *frame, object_t *args);

#endif /* FRAME_H */

