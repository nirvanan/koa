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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "misc.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef PACKAGE_STRING
#define PACKAGE_STRING "koa 0.01"
#endif

#ifndef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT "nirvanan@live.cn"
#endif

static char *g_usage_str = "\
Usage: koa [OPTION]... [INPUT-FILE]\n\n\
  -v, --version\t\toutput version information\n\
  -p, --print\t\tprint op codes of input-file\n\
  -h, --help\t\toutput this usage information\n\n\
Copyright (C) 2018 Gordin Li.\n\
This is free software; see the source for copying conditions.\n\
please send bug report to <%s>.\n\
";

static char *g_version_str = "\
%s\n\
Copyright (C) 2018 Gordin Li.\n\
This is free software; see the source for copying conditions.\n\
please send bug report to <%s>.\n\
";

int
misc_check_source_extension (const char *filename)
{
	size_t len;

	len = strlen (filename);
	if (len < 2 || filename[len - 2] != '.' || filename[len - 1] != KOA_SOURCE_EXTENSION) {
		return 0;
	}

	return 1;
}

int
misc_check_file_access (const char *path, int read, int write)
{
	int mode;

	mode = F_OK;
	if (read) {
		mode |= R_OK;
	}
	if (write) {
		mode |= W_OK;
	}

	return access (path, mode) != -1;
}

int
misc_file_is_older (const char *s, const char *b)
{
	struct stat s_stat;
	struct stat b_stat;

	if (stat (s, &s_stat) || stat (b, &b_stat)) {
		return -1;
	}
#if defined(__APPLE__) && defined(__MACH__)
	return b_stat.st_mtimespec.tv_sec > s_stat.st_mtimespec.tv_sec ||
		(b_stat.st_mtimespec.tv_sec == s_stat.st_mtimespec.tv_sec &&
		 b_stat.st_mtimespec.tv_nsec > s_stat.st_mtimespec.tv_nsec);
#else
	return b_stat.st_mtim.tv_sec > s_stat.st_mtim.tv_sec ||
		(b_stat.st_mtim.tv_sec == s_stat.st_mtim.tv_sec &&
		 b_stat.st_mtim.tv_nsec > s_stat.st_mtim.tv_nsec);
#endif
}

const char *
misc_get_package_full ()
{
	return PACKAGE_STRING;
}

const char *
misc_get_bugreport ()
{
	return PACKAGE_BUGREPORT;
}

void
misc_print_usage (int status)
{
	FILE *out;

	out = status? stderr: stdout;
	fprintf (out, g_usage_str, PACKAGE_BUGREPORT);
}

void
misc_print_opt_error (const char *opt)
{
	fprintf (stderr, "koa: invalid option %s\n", opt);
	misc_print_usage (1);
}

void
misc_print_version ()
{
	fprintf (stdout, g_version_str, PACKAGE_STRING, PACKAGE_BUGREPORT);
}
