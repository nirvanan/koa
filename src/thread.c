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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PTHREAD_H
#include "thread_pthread.h"
#endif

typedef struct thread_context_s
{
	code_t *code;
	object_t *args;
	object_t *ret_value;
	list_t *pool_list; /* The pool list allocated in child thread. */
} thread_context_t;

static void *
thread_func (void *arg)
{
	thread_context_t *context;
	object_t *ret_value;

	context = (thread_context_t *) arg;
	interpreter_execute_thread (context->code, context->args, &ret_value);
	context->ret_value = ret_value;

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
	// __thread_init ();
}