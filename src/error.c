/*
 * error.c
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

#include <stdlib.h>
#include <stdio.h>

#include "error.h"

void
fatal_error (const char *error)
{
	if (error != NULL) {
		fprintf (stderr, "*** FATAL ***: %s\n", error);
	}

	exit (EXIT_FAILURE);
}

void
error (const char *error)
{
	if (error != NULL) {
		fprintf (stderr, "*** ERROR ***: %s\n", error);
	}
}

void
warning (const char *error)
{
	if (error != NULL) {
		fprintf (stderr, "*** WARNING ***: %s\n", error);
	}
}

void
message (const char *error)
{
	if (error != NULL) {
		fprintf (stdout, "*** WARNING ***: %s\n", error);
	}
}
