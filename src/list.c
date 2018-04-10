/*
 * list.c
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

#include <stddef.h>

#include "list.h"

list_t *
list_append (list_t *list, list_t *n)
{
	n->next = list;
	n->prev = NULL;
	if (list != NULL) {
		list->prev = n;
	}

	return n;
}

list_t *
list_remove (list_t *list, list_t *n)
{
	if (n->next != NULL) {
		n->next->prev = n->prev;
	}
	if (n->prev != NULL) {
		n->prev->next = n->next;
	}
	if (list == n) {
		return n->next;
	}

	return list;
}

int
list_find (list_t *list, void *data)
{
	list_t *l;
   
	l = list;
	while (l) {
		if ((void *) l == data) {
			return 1;
		}

		l = l->next;
	}

	return 0;
}

list_t *
list_cleanup (list_t *list, list_del_f df, void *udata)
{
	list_t *l;
	list_t *head;

	l = list;
	head = list;
	while (l != NULL) {
		if (df (l, udata)) {
			head = list_remove (head, l);
		}
	}

	return head;
}

void
list_foreach (list_t *list, list_for_f ff, void *udata)
{
	list_t *l;

	l = list;
	while (l != NULL) {
		ff (l, udata);
		l = l->next;
	}
}

list_t *
list_next (list_t *list)
{
	return list->next;
}

list_t *
list_prev (list_t *list)
{
	return list->prev;
}
