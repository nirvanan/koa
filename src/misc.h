/*
 * misc.h
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

#ifndef MISC_H
#define MISC_H

#include "koa.h"

int
misc_check_source_extension (const char *filename);

int
misc_check_file_access (const char *path, int read, int write);

int
misc_file_is_older (const char *s, const char *b);

const char *
misc_get_package_full ();

const char *
misc_get_bugreport ();

void
misc_print_usage ();

void
misc_print_opt_error (const char *opt);

void
misc_print_version ();

#endif /* MISC_H */
