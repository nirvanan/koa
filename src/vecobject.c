/*
 * vecobject.c
 * This file is part of koa
 *
 * Copyright (C) 2018 - Gordon Li
 *
 * This program is free software: you can redivecibute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is divecibuted in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vecobject.h"
#include "pool.h"
#include "hash.h"
#include "error.h"

/* Object ops. */
static void vecobject_op_free (object_t *obj);
static object_t *vecobject_op_add (object_t *obj1, object_t *obj2);
static object_t *vecobject_op_index (object_t *obj1, object_t *obj2);

static object_opset_t g_object_ops =
{
	NULL, /* Logic Not. */
	vecobject_op_free, /* Free. */
	NULL, /* Dump. */
	NULL, /* Negative. */
	NULL, /* Call. */
	vecobject_op_add, /* Addition. */
	NULL, /* Subvecaction. */
	NULL, /* Multiplication. */
	NULL, /* Division. */
	NULL, /* Mod. */
	NULL, /* Bitwise and. */
	NULL, /* Bitwise or. */
	NULL, /* Bitwise xor. */
	NULL, /* Logic and. */
	NULL, /* Logic or. */
	NULL, /* Left shift. */
	NULL, /* Right shift. */
	NULL, /* Equality. */
	NULL, /* Comparation. */
	vecobject_op_index /* Index. */
};

/* Free. */
void
vecobject_op_free (object_t *obj)
{
	vec_free (vecobject_get_value (obj));
}

/* Addition. */
static object_t *
vecobject_op_add (object_t *obj1, object_t *obj2)
{
	vec_t *v1;
	vec_t *v2;
	vec_t *cated;

	v1 = vecobject_get_value (obj1);
	v2 = vecobject_get_value (obj2);
	cated = vec_concat (v1, v2);
	if (cated == NULL) {
		return NULL;
	}

	return vecobject_vec_new (cated, NULL);
}

/* Index. */
static object_t *
vecobject_op_index (object_t *obj1, object_t *obj2)
{
	vec_t *v;
	integer_value_t pos;

	if (!INTEGER_TYPE (obj2)) {
		error ("vec index must be an integer.");

		return NULL;
	}

	v = vecobject_get_value (obj1);
	pos = object_get_integer (obj2);
	if (pos >= vec_size (v) || pos < 0) {
		error ("vec index out of bound.");

		return NULL;
	}

	return vec_pos (v, pos);
}

object_t *
vecobject_new (size_t len, void *udata)
{
	vecobject_t *obj;

	obj = (vecobject_t *) pool_alloc (sizeof (vecobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	obj->head.ref = 0;
	obj->head.type = OBJECT_TYPE_VEC;
	obj->head.ops = &g_object_ops;
	obj->head.udata = udata;
	obj->val = vec_new (len);
	if (obj->val == NULL) {
		pool_free ((void *) obj);

		return NULL;
	}

	return (object_t *) obj;
}

object_t *
vecobject_vec_new (vec_t *val, void *udata)
{
	vecobject_t *obj;

	obj = (vecobject_t *) pool_alloc (sizeof (vecobject_t));
	if (obj == NULL) {
		error ("out of memory.");

		return NULL;
	}

	obj->head.ref = 0;
	obj->head.type = OBJECT_TYPE_VEC;
	obj->head.ops = &g_object_ops;
	obj->head.udata = udata;
	obj->val = val;

	return (object_t *) obj;
}

vec_t *
vecobject_get_value (object_t *obj)
{
	vecobject_t *ob;

	ob = (vecobject_t *) obj;

	return ob->val;
}

