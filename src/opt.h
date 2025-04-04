/*
 * opt.h
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

#ifndef OPT_H
#define OPT_H

#include "koa.h"

#define MAX_PATH_LENGTH 1000

typedef struct opt_s {
    int help;
    int print;
    int version;
    char path[MAX_PATH_LENGTH + 1];
} opt_t;

opt_t *
opt_parse_opts (int args, char *argv[]);

#endif /* OPT_H */
