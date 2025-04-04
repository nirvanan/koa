/*
 * thread_pthread.h
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

#ifndef THREAD_PTHREAD_H
#define THREAD_PTHREAD_H

#include <pthread.h>

#include "koa.h"
#include "error.h"

static long
_thread_create (void *(*func)(void *), void *arg)
{
	pthread_t th;
	int status;

	status = pthread_create (&th, NULL, func, arg);
	if (status != 0) {
		return 0;
	}

	return (long) th;
}

static int
_thread_join (long th)
{
	return pthread_join ((pthread_t) th, NULL);
}

static int
_thread_detach (long th) {
	return pthread_detach ((pthread_t) th);
}

static int
_thread_cancel (long th)
{
    return pthread_cancel ((pthread_t) th);
}

static void
_thread_init ()
{
#if defined(_AIX) && defined(__GNUC__)
	extern void pthread_init(void);
	pthread_init();
#endif
}


#endif /* THREAD_PTHREAD_H */
