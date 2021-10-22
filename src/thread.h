/*
 * thread.h
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

#ifndef THREAD_H
#define THREAD_H

#include "koa.h"
#include "code.h"
#include "object.h"

long
thread_create (code_t *code, object_t *args);

object_t *
thread_join (long tr);

void
thread_cancel (long tr);

void
thread_set_main_thread ();

int
thread_is_main_thread ();

void
thread_init ();

#endif /* THREAD_H */
