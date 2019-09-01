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

#include "stack.h"
#include "pool.h"
#include "error.h"

stack_t *
stack_new ()
{
	vec_t *v;
	stack_t *stack;

	v = vec_new (0);
	if (v == NULL) {
		return NULL;
	}

	stack = (stack_t *) pool_calloc (1, sizeof (stack_t));
	if (stack == NULL) {
		vec_free (v);
		error ("out of memory.");

		return NULL;
	}

	stack->v = v;

	return stack;
}

void
stack_free (stack_t *stack)
{
	vec_free (stack->v);
	pool_free ((void *) stack);
}

int
stack_push (stack_t *stack, void *data)
{
	if (vec_push_back (stack->v, data)) {
		stack->sp++;

		return 1;
	}

	return 0;
}

void *
stack_pop (stack_t *stack)
{
	void *ret;

	ret = vec_last (stack->v);
	if (vec_pop_back (stack->v)) {
		stack->sp--;

		return ret;
	}

	return NULL;
}

void *
stack_top (stack_t *stack)
{
	return vec_last (stack->v);
}

void
stack_foreach (stack_t *stack, stack_foreach_f fun)
{
	for (integer_value_t pos = stack->sp - 1; pos >= 0; pos--) {
		fun (vec_pos (stack->v, pos));
	}
}
