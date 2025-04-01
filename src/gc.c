/*
 * gc.c
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

#include "gc.h"
#include "error.h"
#include "list.h"
#include "object.h"

#define GEN_NUM 3

#define GEN_HEAD(x) (&g_generation_list[(x)].head)
#define G1_HEAD (GEN_HEAD(0))

typedef struct collect_s
{
	list_t *reachable;
	list_t *unreachable;
	list_t *old;
} collect_t;

typedef struct generation_s
{
	list_t head;
	const int threshold;
	int count;
} generation_t;

/* Each thread track its own objects. */
static __thread generation_t g_generation_list[GEN_NUM] =
{
	{LIST_SINGLE (NULL), 500, 0},
	{LIST_SINGLE (NULL), 10, 0},
	{LIST_SINGLE (NULL), 10, 0}
};

void
gc_track (void *obj)
{
	gc_head_t *head;

	if (!CONTAINER_TYPE ((object_t *) obj)) {
		return;
	}
	head = (gc_head_t *) obj;
	UNUSED (list_append (LIST (GEN_HEAD(0)), LIST (head)));
	g_generation_list[0].count++;
	head->status = GC_STATUS_REACHABLE;
}

void
gc_untrack (void *obj)
{
	gc_head_t *head;

	head = (gc_head_t *) obj;
	if (head->status == GC_STATUS_UNTRACKED) {
		return;
	}

	UNUSED (list_remove (NULL, LIST (head)));
	if (g_generation_list[0].count > 0) {
		g_generation_list[0].count--;
	}
	head->status = GC_STATUS_UNTRACKED;
}

static void
gc_list_merge (int young, int old)
{
	list_merge (GEN_HEAD (old), GEN_HEAD (young));
	UNUSED (list_remove (GEN_HEAD (old), GEN_HEAD (young)));
	LIST_SET_SINGLE (GEN_HEAD (young));
}

static int
gc_update_ref_fun (list_t *list, void *data)
{
	gc_head_t *head;

	if (list == (list_t *) data) {
		return 0;
	}
	head = (gc_head_t *) list;
	head->gc_ref = OBJECT_REF ((object_t *) head);

	return 0;
}

static int
gc_object_sub_ref_fun (object_t *obj, void *data)
{
	gc_head_t *head;

	head = (gc_head_t *) obj;
	if (head->status == GC_STATUS_UNTRACKED) {
		return 0;
	}
	if (head->gc_ref > 0) {
		head->gc_ref--;
	}

	return 0;
}

static int
gc_sub_ref_fun (list_t *list, void *data)
{
	object_t *obj;

	if (list == (list_t *) data) {
		return 0;
	}
	obj = (object_t *) list;
	object_traverse (obj, gc_object_sub_ref_fun, NULL);

	return 0;
}

static int
gc_object_recover_fun (object_t *obj, void *data)
{
	gc_head_t *head;
	collect_t *collect;

	head = (gc_head_t *) obj;
	collect = (collect_t *) data;
	if (head->gc_ref <= 0) {
		head->gc_ref = 1;
	}
	else if (head->status == GC_STATUS_UNREACHABLE) {
		UNUSED (list_remove (collect->unreachable, LIST (head)));
		UNUSED (list_append (collect->reachable, LIST (head)));
		head->status = GC_STATUS_REACHABLE;
	}

	return 0;
}

static int
gc_get_unreachable_fun (list_t *list, void *data)
{
	gc_head_t *head;
	collect_t *collect;

	head = (gc_head_t *) list;
	collect = (collect_t *) data;
	if (list == collect->old) {
		return 0;
	}

	if (head->gc_ref > 0) {
		object_traverse ((object_t *) head, gc_object_recover_fun, data);
	}
	else {
		UNUSED (list_remove (NULL, LIST (head)));
		UNUSED (list_append (collect->unreachable, LIST (head)));
		head->status = GC_STATUS_UNREACHABLE;
	}

	return 0;
}

static int
gc_object_unref_fun (object_t *obj, void *data)
{
	gc_head_t *head;

	head = (gc_head_t *) obj;
	if (head->status == GC_STATUS_UNTRACKED) {
		return 0;
	}
	object_unref (obj);

	return 1;
}

static void
gc_free_unreachable (list_t *unreachable, list_t *old)
{
	object_t *obj;

	while (!LIST_IS_SINGLE (unreachable)) {
		obj = (object_t *) LIST_NEXT (unreachable);
		object_ref (obj);
		object_traverse (obj, gc_object_unref_fun, NULL);
		object_unref (obj);
		if (LIST_NEXT (unreachable) == LIST (obj)) {
			UNUSED (list_remove (NULL, LIST (obj)));
			UNUSED (list_append (old, LIST (obj)));
		}
	}
}

static void
gc_collect_gen (int gen)
{
	collect_t collect;
	list_t reachable;
	list_t unreachable;

	if (gen + 1 < GEN_NUM) {
		g_generation_list[gen + 1].count++;
	}

	for (int i = 0; i <= gen; i++) {
		g_generation_list[i].count = 0;
	}

	/* Merge generations. */
	for (int i = 0; i < gen; i++) {
		gc_list_merge (i, gen);
	}
	/* Update gc_ref. */
	list_foreach (GEN_HEAD (gen), gc_update_ref_fun, (void *) GEN_HEAD (gen));
	/* Find root objects. */
	list_foreach (GEN_HEAD (gen), gc_sub_ref_fun, (void *) GEN_HEAD (gen));
	/* Collect unreachable objects. */
	LIST_SET_SINGLE (&reachable);
	LIST_SET_SINGLE (&unreachable);
	collect.reachable = &reachable;
	collect.unreachable = &unreachable;
	collect.old = GEN_HEAD (gen);
	list_foreach (GEN_HEAD (gen), gc_get_unreachable_fun, (void *) &collect);
	list_merge (GEN_HEAD (gen), &reachable);
	UNUSED (list_remove (NULL, &reachable));
	gc_free_unreachable (&unreachable, GEN_HEAD (gen));
	/* Push young to old. */
	if (gen + 1 < GEN_NUM && !LIST_IS_SINGLE (GEN_HEAD (gen))) {
		gc_list_merge (gen, gen + 1);
	}
}

void
gc_collect ()
{
	for (int i = GEN_NUM - 1; i >= 0; i--) {
		if (g_generation_list[i].count > g_generation_list[i].threshold) {
			gc_collect_gen (i);
			break;
		}
	}
}

void
gc_init ()
{
	for (int i = 0; i < GEN_NUM; i++) {
		LIST_SET_SINGLE (GEN_HEAD (i));
	}
}