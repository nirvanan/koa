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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "misc.h"

int
misc_check_source_extension (const char *filename)
{
	size_t len;

	len = strlen (filename);
	if (len < 2 || filename[len - 2] != '.' || filename[len - 1] != 'k') {
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

	return b_stat.st_mtim.tv_sec > s_stat.st_mtim.tv_sec ||
		(b_stat.st_mtim.tv_sec == s_stat.st_mtim.tv_sec &&
		 b_stat.st_mtim.tv_nsec > s_stat.st_mtim.tv_nsec);
}
