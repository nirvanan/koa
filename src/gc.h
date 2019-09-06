/*
 * gc.h
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

#ifndef GC_H
#define GC_H

#include "koa.h"
#include "list.h"

#define GC_REF(x) ((x)->gc_ref)
#define GC_STATUS(x) ((x)->status)
#define GC_NULL {LIST_NULL,GC_STATUS_UNTRACKED,0}
#define GC_INIT(x) GC_REF((x))=0;\
	GC_STATUS((x))=GC_STATUS_UNTRACKED;\
	LIST_CLEAR(&(x)->link)

typedef enum gc_status_e
{
	GC_STATUS_UNTRACKED,
	GC_STATUS_REACHABLE,
	GC_STATUS_UNREACHABLE
} gc_status_t;

typedef struct gc_head_s
{
	list_t link;
	int gc_ref;
	gc_status_t status;
} gc_head_t;

void
gc_track (void *obj);

void
gc_untrack (void *obj);

void
gc_collect ();

#endif /* GC_H */
