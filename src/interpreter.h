/*
 * interpreter.h
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

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "koa.h"
#include "frame.h"

int
interpreter_play (code_t *code, int global, frame_t *frame);

void
interpreter_execute (const char *path);

void
interpreter_execute_thread (code_t *code, object_t *args, object_t **ret_value);

void
interpreter_traceback ();

void
interpreter_print_stack ();

int
interpreter_started ();

void
interpreter_set_exception (const char *exception);

void
interpreter_set_cmdline (frame_t *frame, code_t *code);

void
interpreter_init ();

#endif /* INTERPRETER_H */
