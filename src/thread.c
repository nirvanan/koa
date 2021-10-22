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

#include <string.h>
#include <stdlib.h>

#include "thread.h"
#include "pool.h"
#include "str.h"
#include "dict.h"
#include "object.h"
#include "longobject.h"
#include "uint64object.h"
#include "strobject.h"
#include "vecobject.h"
#include "dictobject.h"
#include "interpreter.h"
#include "error.h"
#include "gc.h"
#include "builtin.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* If we have pthread, we use it. Otherwise we use platform-specified thread utilities. */
#ifdef HAVE_PTHREAD_H
#include "thread_pthread.h"
#else
#endif

typedef struct thread_context_s
{
	code_t *code;
	object_t *args;
	char *ret_binary;
	size_t ret_len;
	allocator_t *allocator;
} thread_context_t;

static __thread object_t *g_thread_context; /* Store all child thread context, long->uint64. */

static void *
thread_func (void *arg)
{
	thread_context_t *context;
	object_t *ret_value;
	object_t *ret_binary;
	str_t *ret_str;

	context = (thread_context_t *) arg;
	pool_set_allocator (context->allocator);

	/* Init gc. */
	gc_init ();
	/* Init object caches. */
	object_init ();
	/* Init builtin. */
	builtin_init ();

	g_thread_context = dictobject_new (NULL);

	interpreter_execute_thread (context->code, context->args, &ret_value);

	/* Dump the returned object to binary. */
	ret_binary = NULL;
	if (ret_value != NULL) {
		ret_binary = object_binary (ret_value);
	}
	if (ret_binary != NULL) {
		ret_str = strobject_get_value (ret_binary);
		context->ret_len = str_len (ret_str);
		context->ret_binary = (char *) malloc (context->ret_len);
		if (context->ret_binary == NULL) {
			fatal_error ("out of memory.");
		}
		memcpy (context->ret_binary, str_c_str (ret_str), context->ret_len);
		object_free (ret_binary);
		object_unref (ret_value);
	}

	pool_free_all ();

	return (void *) context;
}

static int
thread_gc_untrack_fun (object_t *obj, void *data)
{
	if (CONTAINER_TYPE (obj)) {
		gc_untrack ((void *) obj);
	}

	return 0;
}

long
thread_create (code_t *code, object_t *args)
{
	thread_context_t *context;
	long th;
	object_t *th_obj;
	object_t *context_obj;

	context = (thread_context_t *) calloc (1, sizeof (thread_context_t));
	if (context == NULL) {
		fatal_error ("out of memory.");
	}

	if (!code_check_args (code, vecobject_get_value (args))) {
		free ((void *) context);

		return 0L;
	}

	context->code = code;
	/* Make a new allocator, it is passed to the child thread. */
	context->allocator = pool_make_new_allocator ();
	if (context->allocator == NULL) {
		free ((void *) context);

		return 0L;
	}

	/* Temporarily set the allocator to the new one. */
	pool_set_second_allocator (context->allocator);

	/* The args object passed to the child thread should be allocated
	 * from the new allocator. */
	context->args = object_copy (args);
	if (context->args == NULL) {
		free ((void *) context->allocator);
		free ((void *) context);
		pool_set_second_allocator (NULL);

		return 0L;
	}

	/* Untrack those arguments. */
	gc_untrack (context->args);
	object_traverse (context->args, thread_gc_untrack_fun, NULL);

	/* Recover allocator. */
	pool_set_second_allocator (NULL);

	/* Spawn child thread and store context. */
	th = _thread_create (thread_func, (void *) context);
	if (th == 0) {
		free ((void *) context->allocator);
		free ((void *) context);

		return 0L;
	}
	th_obj = longobject_new (th, NULL);
	context_obj = uint64object_new ((uint64_t) context, NULL);
	UNUSED (object_ipindex (g_thread_context, th_obj, context_obj));

	return th;
}

object_t *
thread_join (long th)
{
	object_t *th_obj;
	object_t *context_obj;
	thread_context_t *context;

	/* Wait for child thread to exit. */
	th_obj = longobject_new (th, NULL);
	context_obj = object_index (g_thread_context, th_obj);
	context = (thread_context_t *) uint64object_get_value (context_obj);

	return context->args;
}

void
thread_cancel (long th)
{
	_thread_cancel (th);
}

void
thread_init ()
{
	g_thread_context = dictobject_new (NULL);
	_thread_init ();
}
