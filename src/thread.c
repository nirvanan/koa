/*
 * thread.c
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

#include "thread.h"
#include "pool.h"
#include "vec.h"
#include "vecobject.h"
#include "interpreter.h"

typedef struct thread_param_s
{
	code_t *code;
	object_t *args;
} thread_param_t;

static void *
thread_func (void *arg)
{
	thread_param_t *param;
	object_t *ret_value;

	param = (thread_param_t *) arg;
	interpreter_execute_thread (param->code, param->args, &ret_value);

	return (void *) ret_value;	
}

long
thread_create (code_t *code, object_t *args)
{
	vec_t *v;

	v = vecobject_get_value (args);
	if (!code_check_args (code, v)) {
		return 0L;
	}
	return 0L;
}

object_t *
thread_join (long tr)
{
	return NULL;
}

void
thread_cancel (long tr)
{

}

void
thread_init ()
{

}