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
#include "base64.h"

static bool source_name_exists(const char *name, obs_data_array_t *sources)
{
	int i = 0;
	obs_data_t *source = obs_data_array_item(sources, i++);
	while (source != NULL) {
		if (strcmp(obs_data_get_string(source, "name"), name) == 0) {
			obs_data_release(source);
			return true;
		}
		obs_data_release(source);
		source = obs_data_array_item(sources, i++);
	}
	return false;
}

#define translate_int(in_key, in, out_key, out, off) \
	obs_data_set_int(out, out_key, obs_data_get_int(in, in_key) + off);
#define translate_string(in_key, in, out_key, out) \
	obs_data_set_string(out, out_key, obs_data_get_string(in, in_key))
#define translate_double(in_key, in, out_key, out) \
	obs_data_set_double(out, out_key, obs_data_get_double(in, in_key))
#define translate_bool(in_key, in, out_key, out) \
	obs_data_set_bool(out, out_key, obs_data_get_int(in, in_key) == 1)

static obs_data_t *translate_scene_item(obs_data_t *in, obs_data_t *source)
{
	obs_data_t *item = obs_data_create();

	translate_string("name", source, "name", item);

	translate_int("crop.top", in, "crop_top", item, 0);
	translate_int("crop.bottom", in, "crop_bottom", item, 0);
	translate_int("crop.left", in, "crop_left", item, 0);
	translate_int("crop.right", in, "crop_right", item, 0);

	obs_data_t *pos = obs_data_create();
	translate_int("x", in, "x", pos, 0);
	translate_int("y", in, "y", pos, 0);

	obs_data_t *bounds = obs_data_create();
	translate_int("cx", in, "x", bounds, 0);
	translate_int("cy", in, "y", bounds, 0);

	obs_data_set_obj(item, "pos", pos);
	obs_data_set_obj(item, "bounds", bounds);

	obs_data_set_int(item, "bounds_type", 2);
	obs_data_set_bool(item, "visible", true);

	obs_data_release(pos);
	obs_data_release(bounds);

	return item;
}

static void create_string_obj(const char *data, obs_data_array_t *arr);

static obs_data_t *translate_source(obs_data_t *in, obs_data_array_t *sources)
{
	const char *id = obs_data_get_string(in, "class");
	const char *name = obs_data_get_string(in, "name");

	if (strcmp(id, "GlobalSource") == 0) {
		int i = 0;
		obs_data_t *source = obs_data_array_item(sources, i++);
		while (source != NULL) {
			if (strcmp(obs_data_get_string(source, "name"), name) ==
			    0) {

				obs_data_set_bool(source, "preexist", true);
				return source;
			}
			obs_data_release(source);
			source = obs_data_array_item(sources, i++);
		}
	}

	obs_data_t *out = obs_data_create();

	obs_data_t *in_settings = obs_data_get_obj(in, "data");
	obs_data_t *settings = obs_data_create();

	int i = 0;
	char new_name[512];
	strcpy(new_name, name);
	while (source_name_exists(new_name, sources)) {
		sprintf(new_name, "%s %d", name, i++);
	}

	obs_data_set_string(out, "name", new_name);

	if (strcmp(id, "TextSource") == 0) {
#ifdef _WIN32
		obs_data_set_string(out, "id", "text_gdiplus");
#else
		obs_data_set_string(out, "id", "text_ft2_source");
#endif

		obs_data_t *font = obs_data_create();

		translate_string("text", in_settings, "text", settings);
		translate_int("color", in_settings, "color", settings,
			      (int)16777216 + 4278190080);
		translate_int("backgroundColor", in_settings, "bk_color",
			      settings, 16777216 + 4278190080);
		translate_int("backgroundOpacity", in_settings, "bk_opacity",
			      settings, 0);
		translate_bool("vertical", in_settings, "vertical", settings);
		translate_int("textOpacity", in_settings, "opacity", settings,
			      0);
		translate_bool("useOutline", in_settings, "outline", settings);
		translate_int("outlineColor", in_settings, "outline_color",
			      settings, 16777216 + 4278190080);
		translate_int("outlineOpacity", in_settings, "outline_opacity",
			      settings, 0);
		translate_bool("useTextExtents", in_settings, "extents",
			       settings);
		translate_int("extentsWidth", in_settings, "extents_cx",
			      settings, 0);
		translate_int("extentsHeight", in_settings, "extents_cy",
			      settings, 0);
		translate_bool("mode", in_settings, "read_from_file", settings);
		translate_bool("wrap", in_settings, "extents_wrap", settings);

		const char *str = obs_data_get_string(in_settings, "file");
		char *rep = string_replace(str, "\\\\", "/");
		obs_data_set_string(settings, "file", rep);
		bfree(rep);

		long long in_align = obs_data_get_int(in_settings, "align");
		const char *align =
			in_align == 0 ? "left"
				      : (in_align == 1 ? "center" : "right");

		obs_data_set_string(settings, "align", align);

		bool bold = obs_data_get_int(in_settings, "bold") == 1;
		bool italic = obs_data_get_int(in_settings, "italic") == 1;

		int flags = bold ? OBS_FONT_BOLD : 0;
		flags |= italic ? OBS_FONT_ITALIC : 0;
		flags |= obs_data_get_int(in_settings, "underline") == 1
				 ? OBS_FONT_UNDERLINE
				 : 0;

		obs_data_set_int(font, "flags", flags);

		translate_int("fontSize", in_settings, "size", font, 0);
		translate_string("font", in_settings, "face", font);

		if (bold && italic) {
			obs_data_set_string(font, "style", "Bold Italic");
		} else if (bold) {
			obs_data_set_string(font, "style", "Bold");
		} else if (italic) {
			obs_data_set_string(font, "style", "Italic");
		} else {
			obs_data_set_string(font, "style", "Regular");
		}

		obs_data_set_obj(settings, "font", font);
		obs_data_release(font);
	} else if (strcmp(id, "MonitorCaptureSource") == 0) {
#if defined(_WIN32)
		obs_data_set_string(out, "id", "monitor_capture");

		translate_int("monitor", in_settings, "monitor", settings, 0);
		translate_bool("captureMouse", in_settings, "capture_cursor",
			       settings);
#elif defined(__APPLE__)
		obs_data_set_string(out, "id", "display_capture");

		translate_bool("captureMouse", in_settings, "show_cursor",
			       settings);
#else
		obs_data_set_string(out, "id", "xshm_input");

		translate_bool("captureMouse", in_settings, "show_cursor",
			       settings);
#endif
	} else if (strcmp(id, "BitmapImageSource") == 0) {
		obs_data_set_string(out, "id", "image_source");

		const char *str = obs_data_get_string(in_settings, "path");
		char *rep = string_replace(str, "\\\\", "/");
		obs_data_set_string(settings, "file", rep);
		bfree(rep);
	} else if (strcmp(id, "BitmapTransitionSource") == 0) {
		obs_data_set_string(out, "id", "slideshow");

		obs_data_array_t *files =
			obs_data_get_array(in_settings, "bitmap");

		if (files == NULL) {
			files = obs_data_array_create();
			const char *str =
				obs_data_get_string(in_settings, "bitmap");

			create_string_obj(str, files);
		}

		obs_data_set_array(settings, "files", files);

		obs_data_array_release(files);
	} else if (strcmp(id, "WindowCaptureSource") == 0) {
#if defined(_WIN32)
		obs_data_set_string(out, "id", "window_capture");

		const char *win = obs_data_get_string(in_settings, "window");
		const char *winClass =
			obs_data_get_string(in_settings, "windowClass");

		char selection[1024];
		sprintf(selection, "%s:%s:", win, winClass);

		obs_data_set_string(settings, "window", selection);
		obs_data_set_int(settings, "priority", 0);
#elif defined(__APPLE__)
		obs_data_set_string(out, "id", "window_capture");
#else
		obs_data_set_string(out, "id", "xcomposite_input");
#endif
	} else if (strcmp(id, "CLRBrowserSource") == 0) {
		obs_data_set_string(out, "id", "browser_source");

		const char *browser_settings =
			obs_data_get_string(in_settings, "sourceSettings");

		char *browser_dec = c_base64_decode(browser_settings);

		obs_data_t *browser = obs_data_create_from_json(browser_dec);

		free(browser_dec);

		translate_string("CSS", browser, "css", settings);
		translate_int("Height", browser, "height", settings, 0);
		translate_int("Width", browser, "width", settings, 0);
		translate_string("Url", browser, "url", settings);

		obs_data_release(browser);
	} else if (strcmp(id, "DeviceCapture") == 0) {
#if defined(_WIN32)
		obs_data_set_string(out, "id", "dshow_input");

		const char *device_id =
			obs_data_get_string(in_settings, "deviceID");
		const char *device_name =
			obs_data_get_string(in_settings, "deviceName");

		char des[1024];

		sprintf(des, "%s:%s", device_name, device_id);

		obs_data_set_string(settings, "video_device_id", des);

		long long w = obs_data_get_int(in_settings, "resolutionWidth");
		long long h = obs_data_get_int(in_settings, "resolutionHeight");

		char resolution[512];
		sprintf(resolution, "%lldx%lld", w, h);

		obs_data_set_string(settings, "resolution", resolution);
#elif defined(__APPLE__)
		obs_data_set_string(out, "id", "av_capture_input");
#else
		obs_data_set_string(out, "id", "v4l2_input");
#endif
	} else if (strcmp(id, "GraphicsCapture") == 0) {
#if defined(_WIN32)
		bool hotkey = obs_data_get_int(in_settings, "useHotkey") == 1;

		if (hotkey) {
			obs_data_set_string(settings, "capture_mode", "hotkey");
		} else {
			obs_data_set_string(settings, "capture_mode", "window");
		}

		char window[512];
		strcpy(window, ":");
		strcat(window, obs_data_get_string(in_settings, "windowClass"));
		strcat(window, ":");
		strcat(window, obs_data_get_string(in_settings, "executable"));

		obs_data_set_string(settings, "window", window);

		translate_bool("captureMouse", in_settings, "capture_cursor",
			       settings);
#elif defined(__APPLE__)
		obs_data_set_string(out, "id", "syphon-input");
#else
		obs_data_release(in_settings);
		obs_data_release(settings);
		obs_data_release(out);
		return NULL;
#endif
	}

	obs_data_set_obj(out, "settings", settings);
	obs_data_release(settings);
	obs_data_release(in_settings);
	return out;
}

#undef translate_int
#undef translate_string
#undef translate_double
#undef translate_bool

static void translate_sc(obs_data_t *in, obs_data_t *out)
{
	obs_data_array_t *out_sources = obs_data_array_create();

	obs_data_array_t *global = obs_data_get_array(in, "globals");
	if (global) {
		int i = 0;
		obs_data_t *source = obs_data_array_item(global, i++);
		while (source != NULL) {
			obs_data_t *out_source =
				translate_source(source, out_sources);

			obs_data_array_insert(out_sources,
					      obs_data_array_count(out_sources),
					      out_source);

			obs_data_release(out_source);
			obs_data_release(source);
			source = obs_data_array_item(global, i++);
		}

		obs_data_array_release(global);
	}

	obs_data_array_t *scenes = obs_data_get_array(in, "scenes");
	int i = 0;
	obs_data_t *in_scene = obs_data_array_item(scenes, i++);

	obs_data_set_string(out, "current_scene",
			    obs_data_get_string(in_scene, "name"));

	while (in_scene != NULL) {
		obs_data_t *scene = obs_data_create();
		obs_data_set_string(scene, "id", "scene");
		obs_data_set_string(scene, "name",
				    obs_data_get_string(in_scene, "name"));

		obs_data_t *settings = obs_data_create();
		obs_data_array_t *items = obs_data_array_create();

		int x = 0;
		obs_data_array_t *sources =
			obs_data_get_array(in_scene, "sources");
		obs_data_t *source = obs_data_array_item(sources, x++);

		while (source != NULL) {
			obs_data_t *out_source =
				translate_source(source, out_sources);

			obs_data_t *out_item =
				translate_scene_item(source, out_source);

			obs_data_set_int(out_item, "id", x);

			obs_data_array_insert(items, 0, out_item);

			if (!obs_data_get_bool(out_source, "preexist"))
				obs_data_array_insert(
					out_sources,
					obs_data_array_count(out_sources),
					out_source);

			obs_data_release(out_item);
			obs_data_release(out_source);
			obs_data_release(source);
			source = obs_data_array_item(sources, x++);
		}

		obs_data_set_array(settings, "items", items);
		obs_data_set_int(settings, "id_counter",
				 obs_data_array_count(items));

		obs_data_set_obj(scene, "settings", settings);

		obs_data_array_insert(out_sources,
				      obs_data_array_count(out_sources), scene);

		obs_data_release(settings);
		obs_data_array_release(items);
		obs_data_release(in_scene);
		obs_data_array_release(sources);
		obs_data_release(scene);
		in_scene = obs_data_array_item(scenes, i++);
	}

	obs_data_array_release(scenes);

	obs_data_set_array(out, "sources", out_sources);
	obs_data_array_release(out_sources);
}

static void create_string(const char *name, obs_data_t *root, const char *data)
{
	char *str = string_replace(data, "\\\\", "/");
	obs_data_set_string(root, name, str);
	bfree(str);
}

static void create_string_obj(const char *data, obs_data_array_t *arr)
{
	obs_data_t *obj = obs_data_create();
	create_string("value", obj, data);

	obs_data_array_push_back(arr, obj);

	obs_data_release(obj);
}

static void create_double(const char *name, obs_data_t *root, const char *data)
{
	double d = atof(data);
	obs_data_set_double(root, name, d);
}

static void create_int(const char *name, obs_data_t *root, const char *data)
{
	int i = atoi(data);
	obs_data_set_int(root, name, i);
}

static void create_data_item(obs_data_t *root, char **line)
{
	char *end_pos = strchr(*line, ':');

	if (end_pos == NULL)
		return;

	char *start_pos = *line;
	while (*start_pos == ' ')
		start_pos = start_pos + 1;

	size_t len = end_pos - start_pos;

	char *name = (char *)bmalloc(len);
	strncpy(name, start_pos, len - 1);
	name[len - 1] = '\0';

	char first = *(end_pos + 2);

	if ((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') ||
	    first == '\\' || first == '/') {
		if (strcmp(obs_data_get_string(root, name), "") != 0 ||
		    obs_data_get_array(root, name) != NULL) {

			obs_data_array_t *arr = obs_data_get_array(root, name);
			if (arr == NULL) {
				arr = obs_data_array_create();
				const char *str =
					obs_data_get_string(root, name);

				create_string_obj(str, arr);

				obs_data_set_array(root, name, arr);
			}

			create_string_obj(end_pos + 2, arr);

			obs_data_array_release(arr);
		} else {
			create_string(name, root, end_pos + 2);
		}
	} else if (first == '"') {
		size_t l = strlen(end_pos + 3);

		char *str = bmalloc(l);
		strncpy(str, end_pos + 3, l - 1);
		str[l - 1] = '\0';

		if (strcmp(obs_data_get_string(root, name), "") != 0 ||
		    obs_data_get_array(root, name) != NULL) {

			obs_data_array_t *arr = obs_data_get_array(root, name);
			if (arr == NULL) {
				arr = obs_data_array_create();
				const char *str =
					obs_data_get_string(root, name);

				create_string_obj(str, arr);

				obs_data_set_array(root, name, arr);
			}

			create_string_obj(str, arr);

			obs_data_array_release(arr);
		} else {
			create_string(name, root, str);
		}

		bfree(str);
	} else if (strchr(end_pos + 2, '.') != NULL) {
		create_double(name, root, end_pos + 2);
	} else {
		create_int(name, root, end_pos + 2);
	}

	bfree(name);
}

static obs_data_t *create_object(obs_data_t *root, char **line, char **src);

static obs_data_array_t *create_sources(obs_data_t *root, char **line,
					char **src)
{
	obs_data_array_t *res = obs_data_array_create();

	bfree(*line);
	*line = read_line(src);
	size_t l_len = strlen(*line) - 1;
	while (*line != NULL && strcmp(*line + l_len, "}") != 0) {
		const char *end_pos = strchr(*line, ':');

		if (end_pos == NULL)
			return NULL;

		const char *start_pos = *line;
		while (*start_pos == ' ')
			start_pos = start_pos + 1;

		size_t len = end_pos - start_pos;

		char *name = (char *)bmalloc(len);
		strncpy(name, start_pos, len - 1);
		name[len - 1] = '\0';

		obs_data_t *source = create_object(NULL, line, src);
		obs_data_set_string(source, "name", name);
		obs_data_array_insert(res, obs_data_array_count(res), source);
		obs_data_release(source);

		bfree(name);
		bfree(*line);
		*line = read_line(src);
		l_len = strlen(*line) - 1;
	}

	if (root)
		obs_data_set_array(root, "sources", res);

	return res;
}

static obs_data_t *create_object(obs_data_t *root, char **line, char **src)
{
	const char *end_pos = strchr(*line, ':');

	if (end_pos == NULL)
		return NULL;

	const char *start_pos = *line;
	while (*start_pos == ' ')
		start_pos = start_pos + 1;

	size_t len = end_pos - start_pos;

	char *name = (char *)bmalloc(len);
	strncpy(name, start_pos, len - 1);
	name[len - 1] = '\0';

	obs_data_t *res = obs_data_create();

	bfree(*line);
	*line = read_line(src);
	size_t l_len = strlen(*line) - 1;
	while (*line != NULL && strcmp(*line + l_len, "}") != 0) {
		start_pos = *line;
		while (*start_pos == ' ')
			start_pos = start_pos + 1;

		if (strncmp(start_pos, "sources", 7) == 0)
			obs_data_array_release(create_sources(res, line, src));
		else if (strcmp(*line + l_len, "{") == 0)
			obs_data_release(create_object(res, line, src));
		else
			create_data_item(res, line);

		bfree(*line);
		*line = read_line(src);
		l_len = strlen(*line) - 1;
	}

	if (root)
		obs_data_set_obj(root, name, res);

	bfree(name);
	return res;
}

static char *classic_name(const char *path)
{
	return get_filename_from_path(path);
}

static int classic_import(const char *path, const char *name, obs_data_t **res)
{
	char *file_data = os_quick_read_utf8_file(path);
	if (!file_data)
		return IMPORTER_FILE_WONT_OPEN;

	if (*res == NULL) {
		obs_data_t *d = obs_data_create();
		*res = d;
	}

	char *sc_name = get_filename_from_path(path);

	if (name == NULL)
		name = sc_name;

	obs_data_set_string(*res, "name", sc_name);
	bfree(sc_name);

	obs_data_t *root = obs_data_create();

	char *src = file_data;
	char *line = read_line(&src);
	while (line != NULL && *line != '\0') {
		char *key = strcmp(line, "global sources : {") != 0 ? "scenes"
								    : "globals";

		obs_data_array_t *arr = create_sources(root, &line, &src);

		obs_data_set_array(root, key, arr);

		obs_data_array_release(arr);

		bfree(line);
		line = read_line(&src);
	}

	if (line != NULL)
		bfree(line);

	translate_sc(root, *res);

	obs_data_release(root);
	bfree(file_data);
	return IMPORTER_SUCCESS;
fail:
	return IMPORTER_ERROR_DURING_CONVERSION;
}

static bool classic_check(const char *path)
{
	bool check = false;

	char *file_data = os_quick_read_utf8_file(path);

	if (!file_data)
		return false;

	if (strncmp(file_data, "scenes : {\r\n", 12) == 0)
		check = true;

	bfree(file_data);
	return check;
}

static obs_importer_files classic_find_files()
{
	obs_importer_files res;
	da_init(res);
#ifdef _WIN32
	char dst[512];
	int found = os_get_config_path(dst, 512, "OBS\\sceneCollection\\");
	if (found == -1)
		return res;

	os_dir_t *dir = os_opendir(dst);
	struct os_dirent *ent;
	while ((ent = os_readdir(dir)) != NULL) {
		if (ent->directory || *ent->d_name == '.')
			continue;

		char *pos = strstr(ent->d_name, ".xconfig");
		char *end_pos = (ent->d_name + strlen(ent->d_name)) - 8;
		if (pos != NULL && pos == end_pos) {
			char *str = bmalloc(512);
			strcpy(str, dst);
			strcat(str, ent->d_name);
			da_insert(res, 0, &str);
		}
	}
	os_closedir(dir);
#endif
	return res;
}

struct obs_importer classic_importer = {
	.prog = "OBSClassic",
	.import_scenes = classic_import,
	.check = classic_check,
	.name = classic_name,
	.find_files = classic_find_files,
};
