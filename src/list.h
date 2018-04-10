/*
 * list.h
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

#ifndef LIST_H
#define LIST_H

#include "koa.h"

#define LIST_NEXT(x) (((list_t *)x)->next)

#define LIDT_PREV(x) (((list_t *)x)->prev)

typedef int (*list_del_f) (void *data, void *udata);

typedef void (*list_for_f) (void *data, void *udata);

typedef struct list_s
{
	struct list_s *prev;
	struct list_s *next;
} list_t;

list_t *
list_append (list_t *list, list_t *n);

list_t *
list_remove (list_t *list, list_t *n);

int
list_find (list_t *list, void *data);

list_t *
list_cleanup (list_t *list, list_del_f df, void *udata);

void
list_foreach (list_t *list, list_for_f ff, void *udata);

#endif /* LIST_H */
