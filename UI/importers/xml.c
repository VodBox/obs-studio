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

#include "xml.h"

static void translate_value(obs_data_t *attr, char *name, char *value)
{
	char *pos = value;
	bool is_int = true;

	while ((size_t)(pos - value) < strlen(value)) {
		if (*pos < '0' || *pos > '9') {
			is_int = false;
			break;
		}
		pos = pos + 1;
	}

	if (is_int) {
		obs_data_set_int(attr, name, atoi(value));
		return;
	}

	pos = value;

	bool is_double = true;
	bool dot = false;

	while ((size_t)(pos - value) < strlen(value)) {
		if (*pos < '0' || *pos > '9') {
			if (*pos == '.' && !dot)
				dot = true;
			else {
				is_double = false;
				break;
			}
		}
		pos = pos + 1;
	}

	if (is_double) {
		obs_data_set_double(attr, name, atof(value));
		return;
	}

	obs_data_set_string(attr, name, value);
}

static void parse_attr(char **orig, obs_data_t *attr)
{
	char *line = *orig;
	while (strncmp(line, ">", 1) != 0 && strncmp(line, "/>", 2) != 0) {
		char *name;
		char *equals = strchr(line, '=');

		name = bmalloc(equals - line + 1);
		strncpy(name, line, equals - line);
		name[equals - line] = '\0';

		line = equals + 1;

		char *quote = strchr(line + 1, '"');
		char *value;

		value = bmalloc(quote - line);
		strncpy(value, line + 1, quote - line);
		value[quote - line - 1] = '\0';

		line = quote + 1;

		if (*line == ' ')
			line = line + 1;

		translate_value(attr, name, value);

		bfree(value);
		bfree(name);
	}

	*orig = line;
}

static obs_data_t *parse_line(char **orig, char **src)
{
	char *line = *orig;

	obs_data_t *res = obs_data_create();
	obs_data_t *attr = obs_data_create();
	obs_data_array_t *children = obs_data_array_create();

	while (*line == ' ' || *line == '\t')
		line = (line + 1);

	if (*line != '<' || *(line + 1) == '/')
		goto fail;

	line = (line + 1);

	char *space = strchr(line, ' ');
	char *end = strchr(line, '>');

	char *node_type;

	if (space != NULL) {
		node_type = bmalloc(space - line + 1);
		strncpy(node_type, line, space - line);
		node_type[space - line] = '\0';

		line = space + 1;
	} else {
		node_type = bmalloc(end - line + 1);
		strncpy(node_type, line, end - line);
		node_type[end - line] = '\0';

		line = end + 1;
	}

	obs_data_set_string(res, "type", node_type);
	bfree(node_type);

	if (strcmp(line, "") != 0 && strcmp(line, "/>") != 0 &&
	    strncmp(line, ">", 1))
		parse_attr(&line, attr);

	obs_data_set_obj(res, "attr", attr);

	if (strcmp(line, "/>") != 0 && strstr(line, "</") == NULL) {
		bfree(*orig);
		*orig = read_line(src);

		obs_data_t *child = parse_line(orig, src);

		while (child) {
			obs_data_array_insert(children,
					      obs_data_array_count(children),
					      child);

			obs_data_release(child);

			bfree(*orig);
			*orig = read_line(src);

			if (*orig != NULL)
				child = parse_line(orig, src);
			else
				child = NULL;
		}
	}

	obs_data_set_array(res, "children", children);

	obs_data_release(attr);
	obs_data_array_release(children);

	return res;
fail:
	obs_data_release(attr);
	obs_data_array_release(children);
	obs_data_release(res);

	return NULL;
}

obs_data_t *parse_xml(char *doc)
{
	char *line = read_line(&doc);
	if (strncmp(line, "<?xml", 5) == 0) {
		bfree(line);
		line = read_line(&doc);
	}
	obs_data_t *l = parse_line(&line, &doc);
	bfree(line);

	return l;
}
