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

#include "importers.h"
#include <util/platform.h>
#include <util/darray.h>
#include "stdlib.h"
#include "string.h"

extern struct obs_importer studio_importer;
extern struct obs_importer classic_importer;
extern struct obs_importer sl_importer;
extern struct obs_importer xsplit_importer;

DARRAY(struct obs_importer) importers;

void importers_init()
{
	if (importers.array != NULL)
		da_free(importers);

	da_init(importers);
	da_insert(importers, 0, &classic_importer);
	da_insert(importers, 0, &sl_importer);
	da_insert(importers, 0, &xsplit_importer);
	da_insert(importers, 0, &studio_importer);
}

void importers_free()
{
	da_free(importers);
}

int import_sc_from_prog(const char *path, const char *name, const char *program,
			obs_data_t **res)
{
	if (path == NULL || !os_file_exists(path)) {
		return IMPORTER_FILE_NOT_FOUND;
	}

	for (size_t i = 0; i < importers.num; i++) {
		if (strcmp(program, importers.array[i].prog) == 0)
			return importers.array[i].import_scenes(path, name,
								res);
	}

	return IMPORTER_UNKNOWN_ERROR;
}

int import_sc(const char *path, const char *name, obs_data_t **res)
{
	if (path == NULL || !os_file_exists(path)) {
		return IMPORTER_FILE_NOT_FOUND;
	}

	const char *prog = detect_program(path);

	if (prog == NULL) {
		return IMPORTER_FILE_NOT_RECOGNISED;
	}

	return import_sc_from_prog(path, name, prog, res);
}

const char *detect_program(const char *path)
{
	if (path == NULL || !os_file_exists(path)) {
		return "";
	}

	for (size_t i = 0; i < importers.num; i++) {
		if (importers.array[i].check(path)) {
			return importers.array[i].prog;
		}
	}

	return "";
}

char *get_sc_name(const char *path, const char *prog)
{
	for (size_t i = 0; i < importers.num; i++) {
		if (strcmp(importers.array[i].prog, prog) == 0) {
			return importers.array[i].name(path);
		}
	}

	return NULL;
}

obs_importer_files importers_find_files()
{
	obs_importer_files f;
	da_init(f);

	for (size_t i = 0; i < importers.num; i++) {
		if (importers.array[i].find_files) {
			obs_importer_files f2 = importers.array[i].find_files();
			if (f2.num != 0)
				da_insert_da(f, 0, f2);
			da_free(f2);
		}
	}

	return f;
}
