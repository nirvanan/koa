/*
 * main.c
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
#include "pool.h"
#include "object.h"
#include "intobject.h"
#include "strobject.h"
#include "str.h"

int main(int argc, char *argv[])
{
	/* Init pool utility. */
	pool_init ();
	/* Init object caches. */
	object_init ();

	while (1) {
		void *str1 = pool_alloc (200);

		pool_free (str1);
	}

	return 0;
}
