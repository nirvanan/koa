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
#include <stdlib.h>

#include "builtin.h"
#include "pool.h"
#include "error.h"
#include "vec.h"
#include "vecobject.h"
#include "dictobject.h"
#include "strobject.h"
#include "funcobject.h"
#include "exceptionobject.h"

#define MAX_ARGS 256

#define ARG(x, y) ((object_t*)vec_pos(vecobject_get_value((x)),vec_size(vecobject_get_value((x)))-1-(integer_value_t)y))
#define ARG_SIZE(x) (vec_size(vecobject_get_value((x))))
#define DUMMY (object_get_default(OBJECT_TYPE_VOID,NULL))

static object_t *g_builtin;

static object_t *
_builtin_print (object_t *args)
{
	object_t *arg;

	arg = ARG (args, 0);
	object_print (arg);
	printf ("\n");

	return DUMMY;
}

static object_t *
_builtin_hash (object_t *args)
{
	object_t *arg;

	arg = ARG (args, 0);

	return object_hash (arg);
}

static object_t *
_builtin_len (object_t *args)
{
	object_t *arg;

	arg = ARG (args, 0);

	return object_len (arg);
}

static object_t *
_builtin_append (object_t *args)
{
	object_t *vec;

	/* The first argument must be vec. */
	vec = ARG (args, 0);
	if (vec == NULL || !OBJECT_IS_VEC (vec)) {
		error ("the first argument of append must be vec.");

		return NULL;
	}
	if (ARG (args, 1) == NULL) {
		error ("no element to append.");

		return NULL;
	}

	for (int i = 1; ARG (args, i) != NULL; i++) {
		if (vecobject_append (vec, ARG (args, i)) == 0) {
			return NULL;
		}
	}

	return DUMMY;
}

static object_t *
_builtin_remove (object_t *args)
{
	object_t *container;
	object_t *target;

	container = ARG (args, 0);
	target = ARG (args, 1);
	if (OBJECT_IS_VEC (container)) {
		integer_value_t pos;

		if (!INTEGER_TYPE (target)) {
			error ("vec pos must be an integer type.");

			return NULL;
		}
		pos = object_get_integer (target);

		if (!vecobject_remove (container, pos)) {
			return NULL;
		}

		return DUMMY;
	}
	else if (OBJECT_IS_DICT (container)) {
		if (!NUMBERICAL_TYPE (target) && OBJECT_TYPE (target) != OBJECT_TYPE_STR) {
			error ("dict index must be a number or str.");

			return NULL;
		}

		if (!dictobject_remove (container, target)) {
			return NULL;
		}

		return DUMMY;
	}

	error ("the first argument of remove must be a dict or a vec.");

	return NULL;
}

static object_t *
_builtin_exit (object_t *args)
{
	object_t *exit_obj;
	int exit_value;

	exit_obj = ARG (args, 0);
	exit_value = 0;
	if (INTEGER_TYPE (exit_obj)) {
		integer_value_t integer_value = object_get_integer (exit_obj);
		exit_value = (int) integer_value;
	}
	else if (FLOATING_TYPE (exit_obj)){
		floating_value_t floating_value = object_get_floating (exit_obj);
		exit_value = (int) floating_value;
	}
	else {
		error ("the argument of exit should be numberical.");

		return NULL;
	}

	exit (exit_value);

	/* Bite me. */
	return DUMMY;
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
	{2, "hash", _builtin_hash, 0, 1, {OBJECT_TYPE_ALL}},
	{3, "len", _builtin_len, 0, 1, {OBJECT_TYPE_ALL}},
	{4, "append", _builtin_append, 1, 0, {}},
	{5, "remove", _builtin_remove, 0, 2, {OBJECT_TYPE_ALL, OBJECT_TYPE_ALL}},
	{6, "exit", _builtin_exit, 0, 1, {OBJECT_TYPE_ALL}},
	{0, NULL, NULL, 0, 0, {}}
};

object_t *
builtin_find (object_t *name)
{
	return object_index (g_builtin, name);
}

object_t *
builtin_execute (builtin_t *builtin, object_t *args)
{
	builtin_slot_t *slot;

	if (builtin->slot <= 0 || (size_t) builtin->slot > sizeof (g_builtin_slot_list)) {
		fatal_error ("slot out of bound.");
	}

	slot = &g_builtin_slot_list[builtin->slot - 1];
	/* Check args. */
	if (!slot->var_args) {
		vec_t *v;
		size_t size;

		v = vecobject_get_value (args);
		size = vec_size (v);
		if (size != slot->args) {
			error ("wrong number of arguments, required: %d, passed: %d.", slot->args, size);

			return NULL; 
		}
		for (size_t i = 0; i < size; i++) {
			object_t *arg;

			arg = vec_pos (v, (integer_value_t) i);
			if (OBJECT_TYPE (arg) != slot->types[i] && slot->types[i] != OBJECT_TYPE_ALL) {
				 error ("wrong argument type at position %d.", i + 1);

				 return NULL;
			}
		}
	}

	return slot->fun (args);
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
builtin_free (builtin_t *builtin)
{
	pool_free ((void *) builtin);
}

const char *
builtin_get_name (builtin_t *builtin)
{
	return g_builtin_slot_list[builtin->slot - 1].name;
}

int
builtin_no_arg (builtin_t *builtin)
{
	return g_builtin_slot_list[builtin->slot - 1].args == 0 &&
		!g_builtin_slot_list[builtin->slot - 1].var_args;
}

object_t *
builtin_binary (builtin_t *builtin)
{
	return strobject_new (BINARY (builtin->slot), sizeof (int), 1, NULL);
}

builtin_t *
builtin_load_binary (FILE *f)
{
	int id;
	builtin_t *builtin;

	if (fread (&id, sizeof (int), 1, f) != 1) {
		error ("failed to load builtin binary.");

		return NULL;
	}

	builtin = (builtin_t *) pool_calloc (1, sizeof (builtin_t));
	if (builtin == NULL) {
		return NULL;
	}

	builtin->slot = id;

	return builtin;
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
	
	object_ref (g_builtin);
	gc_untrack (g_builtin);
}
