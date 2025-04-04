/*
 * opt.c
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
#include <string.h>

#include "opt.h"
#include "misc.h"
#include "error.h"

#define OPT_IS(x, y) ((strlen(x)==strlen(y))&&strncmp(x,y,strlen(x))==0)

/* Keep opts static. */
static opt_t g_opts;

typedef struct opt_config_s {
    const char *opt;
    const char *long_opt;
    int *flag; /* Flag to set. */
    int quit; /* If set, no more opts will be parsed. */
    int check_path; /* Need path specified if opt is given. */
} opt_config_t;

static opt_config_t g_all_opts[] = {
    {"-p", "--print", &g_opts.print, 0, 1},
    {"-h", "--help", &g_opts.help, 1, 0},
    {"-v", "--version", &g_opts.version, 1, 0},
    {NULL, NULL, NULL, 0}
};

static int
opt_check_path ()
{
    opt_config_t *cu;

    cu = &g_all_opts[0];
    while (cu->opt != NULL) {
        if (*cu->flag && cu->check_path) {
            if (g_opts.path[0] == '\0') {
                error ("koa: need input-file when %s or %s option specified.", cu->opt, cu->long_opt);
                misc_print_usage (1);

                return 0;
            }
        }

        cu++;
    }

    return 1;
}

opt_t *
opt_parse_opts (int args, char *argv[])
{
    int cur;

    cur = 1;
    while (cur < args) {
        opt_config_t *cu;
        int hit;

        cu = &g_all_opts[0];
        hit = 0;
        while (cu->opt != NULL) {
            if (OPT_IS (argv[cur], cu->opt) || OPT_IS (argv[cur], cu->long_opt)) {
                *cu->flag = 1;
                hit = 1;
                if (cu->quit) {
                    return &g_opts;
                }

                break;
            }

            cu++;
        }

        /* The last opt is considered as code path. */
        if (!hit && cur == args - 1 && argv[cur][0] != '-') {
            strncpy (g_opts.path, argv[cur], MAX_PATH_LENGTH);
            return &g_opts;
        }

        /* Meet an invalid opt, show an error message and print usage. */
        if (!hit) {
            error ("koa: invalid option %s", argv[cur]);
            misc_print_usage (1);

            return NULL;
        }

        cur++;
    }

    if (!opt_check_path ()) {
        return NULL;
    }

    return &g_opts;
}