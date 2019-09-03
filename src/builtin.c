/*
 * builtin.c
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

#include "builtin.h"
#include "pool.h"
#include "error.h"
#include "dictobject.h"
#include "strobject.h"
#include "funcobject.h"

#define MAX_ARGS 10

static object_t *g_builtin;

object_t *
_builtin_print (object_t *args)
{
	return NULL;
}

typedef struct builtin_slot_s
{
	int id;
	const char *name;
	builtin_f fun;
	int var_args;
	int args;
	object_type_t types[MAX_ARGS];
} builtin_slot_t;

static builtin_slot_t g_builtin_slot_list[] =
{
	{1, "print", _builtin_print, 0, 1, {OBJECT_TYPE_ALL}},
	{0, NULL, NULL, 0, 0, {}}
};

object_t *
builtin_find (object_t *name)
{
	return NULL;
}

object_t *
builtin_execute (builtin_t *builtin)
{
	return NULL;
}

static object_t *
builtin_make (builtin_slot_t *slot)
{
	builtin_t *builtin;

	builtin = (builtin_t *) pool_calloc (1, sizeof (builtin_t));
	if (builtin == NULL) {
		return NULL;
	}

	builtin->slot = slot->id;

	return funcobject_builtin_new (builtin, NULL);
}

void
builtin_init ()
{
	builtin_slot_t *slot;

	/* Insert all slots. */
	g_builtin = dictobject_new (NULL);
	if (g_builtin == NULL) {
		fatal_error ("failed to allocate the slot dict.");
	}

	slot = &g_builtin_slot_list[0];
	while (slot->name != NULL) {
		object_t *word;
		object_t *func;
		object_t *suc;

		word = strobject_new (slot->name, strlen (slot->name), 0, NULL);
		if (word == NULL) {
			fatal_error ("failed to generate the reserved word dict.");
		}
		func = builtin_make (slot);
		if (func == NULL) {
			fatal_error ("failed to init builtin %s.", slot->name);
		}
		suc = object_ipindex (g_builtin, word, func);
		if (suc == NULL) {
			fatal_error ("failed to generate the reserved word dict.");
		}

		slot++;
	}
	object_print (g_builtin);
}
