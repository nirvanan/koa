/*
 * stack.h
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

#ifndef STACK_H
#define STACK_H

#include "koa.h"
#include "vec.h"

typedef integer_value_t sp_t;

typedef struct stack_s
{
	vec_t *v;
	sp_t sp;
} stack_t;

stack_t *
stack_new ();

void
stack_free (stack_t *stack);

int
stack_push (stack_t *stack, void *data);

void *
stack_pop (stack_t *stack);

void *
stack_top (stack_t *stack);

#endif /* STACK_H */

