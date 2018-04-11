/*
 * object.c
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

#include <stdio.h>

#include "object.h"
#include "pool.h"
#include "error.h"

/* Note that the 'null' object is shared everywhere. */
static object_t *g_null_object;

static object_opset_t g_object_ops =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/* This object is known as 'null'. */
object_t *
object_new (void *udata)
{
	object_t *ob;

	if (g_null_object != NULL) {
		return g_null_object;
	}
	ob = (object_t *) pool_alloc (sizeof (object_t));
	ob->head.ref = 0;
	ob->head.type = OBJECT_TYPE_NONE;
	ob->head.ops = &g_object_ops;
	ob->head.udata = udata;

	return ob;
}

void
object_ref (object_t *ob)
{
	ob->head.ref++;
}

void
object_unref (object_t *ob)
{
	ob->head.ref--;
	if (ob->head.ref <= 0) {
		object_free (ob);
	}
}

void
object_free (object_t *ob)
{
	void_una_op_f free_fun;

	/* Refed objects should not be freed. */
	if (ob->head.ref > 0) {
		return;
	}

	/* Call object specifiled cleanup routine. */
	free_fun = ((object_opset_t *) ob->head.ops)->free;
	if (free_fun != NULL) {
		free_fun (ob);
	}

	pool_free (ob);
}

void
object_init ()
{
	g_null_object = object_new (NULL);
	object_ref (g_null_object);
}
