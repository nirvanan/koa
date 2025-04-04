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
#include "pool.h"

list_t *
list_append (list_t *list, list_t *n)
{
	if (list == NULL || list->prev == NULL) {
		n->next = list;
		n->prev = NULL;
		if (list != NULL) {
			n->prev = list->prev;
			list->prev = n;
		}

		return n;
	}

	n->next = list;
	n->prev = list->prev;
	list->prev->next = n;
	list->prev = n;

	return list;
}

list_t *
list_remove (list_t *list, list_t *n)
{
	list_t *head;

	if (n->next != NULL) {
		n->next->prev = n->prev;
	}
	if (n->prev != NULL) {
		n->prev->next = n->next;
	}

	head = list == n? n->next: list;

	n->next = NULL;
	n->prev = NULL;

	return head;
}

void
list_merge (list_t *a, list_t *b)
{
	a->next->prev = b->prev;
	b->prev->next = a->next;
	a->next = b;
	b->prev = a;
}

int
list_find (list_t *list, void *data)
{
	list_t *l;

	if (list != NULL && (void *) list == data) {
		return 1;
	}
	l = list == NULL? NULL: list->next;
	while (l != NULL && l != list) {
		if ((void *) l == data) {
			return 1;
		}

		l = l->next;
	}

	return 0;
}

list_t *
list_cleanup (list_t *list, list_del_f df, int need_free, void *udata)
{
	list_t *l;
	list_t *head;
	list_t *next;

	head = list;
	l = list;
	while (l != NULL) {
		next = l->next;
		if (df (l, udata) > 0) {
			head = list_remove (head, l);
			if (need_free) {
				pool_free ((void *) l);
			}
		}

		l = next;
	}

	return head;
}

void
list_foreach (list_t *list, list_for_f ff, void *udata)
{
	list_t *l;
	list_t *next;

	l = NULL;
	if (list != NULL) {
		l = list->next;
		if (ff (list, udata) > 0) {
			return;
		}
	}
	while (l != NULL && l != list) {
		next = l->next;
		if (ff (l, udata) > 0) {
			return;
		}
		l = next;
	}
}

size_t
list_len (list_t *list)
{
	list_t *l;
	size_t size;

	size = 0;
	l = list;
	while (l != NULL) {
		size++;
		l = l->next;
	}

	return size;
}
