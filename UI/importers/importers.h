/******************************************************************************
    Copyright (C) 2019 by Dillon Pentz <dillon@vodbox.io>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "stdbool.h"
#include <util/darray.h>
#include <util/platform.h>
#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif

enum obs_importer_responses {
	IMPORTER_SUCCESS,
	IMPORTER_FILE_NOT_FOUND,
	IMPORTER_FILE_NOT_RECOGNISED,
	IMPORTER_FILE_WONT_OPEN,
	IMPORTER_ERROR_DURING_CONVERSION,
	IMPORTER_UNKNOWN_ERROR,
	IMPORTER_NOT_FOUND
};

typedef DARRAY(char *) obs_importer_files;

struct obs_importer {
	const char *prog;
	int (*import_scenes)(const char *path, const char *name,
			     obs_data_t **res);
	bool (*check)(const char *path);
	char *(*name)(const char *path);
	obs_importer_files (*find_files)();
};

void importers_init();
void importers_free();

const char *detect_program(const char *path);
char *get_sc_name(const char *path, const char *prog);

int import_sc_from_prog(const char *path, const char *name, const char *program,
			obs_data_t **res);
int import_sc(const char *path, const char *name, obs_data_t **res);

obs_importer_files importers_find_files();

void translate_os_studio(obs_data_t *data);

static inline char *get_filename_from_path(const char *path)
{
	char *dir = (char *)bmalloc(sizeof(char) * 1024);
#ifdef _WIN32
	const char *slashPos = strrchr(path, '\\');
	if (slashPos == NULL || slashPos < strrchr(path, '/'))
		slashPos = strrchr(path, '/');
#else
	const char *slashPos = strrchr(path, '/');
#endif
	const char *pos = os_get_path_extension(path);
	strncpy(dir, slashPos + 1, pos - slashPos - 1);
	dir[pos - slashPos - 1] = '\0';

	return dir;
}

static inline char *get_folder_from_path(const char *path)
{
	char *dir = (char *)bmalloc(sizeof(char) * 1024);
#ifdef _WIN32
	const char *slashPos = strrchr(path, '\\');
	if (slashPos == NULL || slashPos < strrchr(path, '/'))
		slashPos = strrchr(path, '/');
#else
	const char *slashPos = strrchr(path, '/');
#endif
	strncpy(dir, path, slashPos - path + 1);
	dir[slashPos - path + 1] = '\0';

	return dir;
}

static inline char *read_line(char **src)
{
	char *pos = strchr(*src, '\r');

	if (pos == NULL)
		pos = strchr(*src, EOF);

	if (pos == NULL)
		pos = strchr(*src, '\0');

	if (pos == NULL)
		return NULL;

	size_t len = pos - *src;

	char *res = (char *)bmalloc(len + sizeof(char));
	strncpy(res, *src, len);
	res[len] = '\0';

	*src = *src + len + sizeof(char) * 2;

	return res;
}

static inline char *string_replace(const char *in, const char *search,
				   const char *rep)
{
	char *res = (char *)bmalloc(
		(strlen(in) + strlen(rep) - strlen(search) + 1) * sizeof(char));

	strcpy(res, in);

	char *pos = strstr(res, search);

	while (pos != NULL) {
		char *new_res = (char *)bmalloc(
			(strlen(res) + strlen(rep) - strlen(search) + 1) *
			sizeof(char));

		strncpy(new_res, res, (pos - res) / sizeof(char));
		strcpy(new_res + (pos - res), rep);
		strcpy(new_res + (pos - res) + strlen(rep) * sizeof(char),
		       pos + strlen(search) * sizeof(char));

		bfree(res);

		pos = strstr(new_res + (pos - res) + strlen(rep) * sizeof(char),
			     search);
		res = new_res;
	}

	return res;
}

#ifdef __cplusplus
}
#endif
