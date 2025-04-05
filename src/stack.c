/*
 * stack.c
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

#include "stack.h"
#include "pool.h"
#include "error.h"

#define STACK_INIT_SIZE 32

st_t *
stack_new ()
{
	st_t *stack;

	stack = (st_t *) pool_calloc (1, sizeof (st_t));
	if (stack == NULL) {
		fatal_error ("out of memory.");
	}

	stack->top = STACK_INIT_SIZE;
	stack->cap = STACK_INIT_SIZE;
	stack->v = (void **) pool_calloc (STACK_INIT_SIZE, sizeof (void *));

	return stack;
}

void
stack_free (st_t *stack)
{
	pool_free ((void *) stack->v);
	pool_free ((void *) stack);
}

int
stack_push (st_t *stack, void *data)
{
	/* Double the stack if it is full. */
	if (stack->sp >= stack->top) {
		void *new_v;

		new_v = (void **) pool_calloc (stack->cap * 2, sizeof (void *));
		if (new_v == NULL) {
			fatal_error ("out of memory.");
		}
		memcpy (new_v, stack->v, stack->cap * sizeof (void *));
		stack->top = stack->top * 2;
		stack->cap = stack->cap * 2;
		pool_free ((void *) stack->v);
		stack->v = new_v;
	}

	stack->v[stack->sp++] = data;

	return 1;
}

void *
stack_pop (st_t *stack)
{
	void *ret;

	if (stack->sp <= 0) {
		return NULL;
	}

	ret = stack->v[--stack->sp];

	return ret;
}

void *
stack_top (st_t *stack)
{
	if (stack->sp <= 0) {
		return NULL;
	}

	return stack->v[stack->sp - 1];
}

void
stack_foreach (st_t *stack, stack_foreach_f fun)
{
	for (integer_value_t pos = stack->sp - 1; pos >= 0; pos--) {
		fun (stack->v[pos]);
	}
}

sp_t
stack_get_sp (st_t *stack)
{
	return stack->sp;
}

void *
stack_set (st_t *stack, integer_value_t pos, void *data)
{
	return stack->v[pos] = data;
}
