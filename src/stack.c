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

st_t *
stack_new ()
{
	vec_t *v;
	st_t *stack;

	v = vec_new (0);
	if (v == NULL) {
		return NULL;
	}

	stack = (st_t *) pool_calloc (1, sizeof (st_t));
	if (stack == NULL) {
		vec_free (v);
		fatal_error ("out of memory.");
	}

	stack->v = v;

	return stack;
}

void
stack_free (st_t *stack)
{
	vec_free (stack->v);
	pool_free ((void *) stack);
}

int
stack_push (st_t *stack, void *data)
{
	if (vec_push_back (stack->v, data)) {
		stack->sp++;

		return 1;
	}

	return 0;
}

void *
stack_pop (st_t *stack)
{
	void *ret;

	if (stack->sp <= 0) {
		return NULL;
	}

	ret = vec_last (stack->v);
	if (vec_pop_back (stack->v)) {
		stack->sp--;

		return ret;
	}

	return NULL;
}

void *
stack_top (st_t *stack)
{
	return vec_last (stack->v);
}

void
stack_foreach (st_t *stack, stack_foreach_f fun)
{
	for (integer_value_t pos = stack->sp - 1; pos >= 0; pos--) {
		fun (vec_pos (stack->v, pos));
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
	return vec_set (stack->v, pos, data);
}
