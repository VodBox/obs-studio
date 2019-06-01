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
#include "xml.h"
#include <ctype.h>

static int hex_string_to_int(const char *str)
{
	int res = 0;

	if (*str == '#')
		str = str + 1;

	for (size_t i = 0, l = strlen(str); i < l; i++) {
		res *= 16;

		if (*str >= '0' && *str <= '9')
			res += *str - '0';
		else
			res += *str - 'A' + 10;

		str = str + 1;
	}

	return res;
}

#define rep(dst, in, out, temp)              \
	temp = dst;                          \
	dst = string_replace(temp, in, out); \
	bfree(temp)

#define translate_int(in_key, in, out_key, out, off) \
	obs_data_set_int(out, out_key, obs_data_get_int(in, in_key) + off);
#define translate_string(in_key, in, out_key, out) \
	obs_data_set_string(out, out_key, obs_data_get_string(in, in_key))
#define translate_double(in_key, in, out_key, out) \
	obs_data_set_double(out, out_key, obs_data_get_double(in, in_key))
#define translate_bool(in_key, in, out_key, out) \
	obs_data_set_bool(out, out_key, obs_data_get_int(in, in_key) == 1)

static obs_data_t *parse_text(const char *config)
{
	obs_data_t *res = obs_data_create();
	obs_data_t *font = obs_data_create();

	const char *start = strstr(config, "*{");
	start = start + 1;

	char *r = string_replace(start, "&quot;", "\"");
	char *temp;
	rep(r, "\\", "/", temp);

	obs_data_t *data = obs_data_create_from_json(r);
	bfree(r);

	translate_string("text", data, "text", res);
	translate_string("fontStyle", data, "face", font);

	obs_data_set_int(font, "size", 200);

	const char *outline = obs_data_get_string(data, "outline");
	if (strcmp(outline, "thick") == 0) {
		obs_data_set_bool(res, "outline", true);
		obs_data_set_int(res, "outline_size", 20);
	} else if (strcmp(outline, "thicker") == 0) {
		obs_data_set_bool(res, "outline", true);
		obs_data_set_int(res, "outline_size", 40);
	} else if (strcmp(outline, "thinner") == 0) {
		obs_data_set_bool(res, "outline", true);
		obs_data_set_int(res, "outline_size", 5);
	} else if (strcmp(outline, "thin") == 0) {
		obs_data_set_bool(res, "outline", true);
		obs_data_set_int(res, "outline_size", 10);
	}

	const char *o_color = obs_data_get_string(data, "outlineColor");
	obs_data_set_int(res, "outline_color",
			 hex_string_to_int(o_color) + 4278190080);

	const char *color = obs_data_get_string(data, "color");
	obs_data_set_int(res, "color", hex_string_to_int(color) + 4278190080);

	translate_string("textAlign", data, "align", res);

	const char *valign = obs_data_get_string(data, "vertAlign");
	if (strcmp(valign, "middle") == 0)
		valign = "center";

	translate_int("alpha", data, "opacity", res, 0);

	obs_data_set_string(res, "valign", valign);

	obs_data_set_obj(res, "font", font);
	obs_data_release(font);

	obs_data_release(data);

	return res;
}

static obs_data_array_t *parse_playlist(const char *config)
{
	obs_data_array_t *out = obs_data_array_create();

	size_t pos = 0;
	while (true) {
		obs_data_t *file = obs_data_create();

		const char *end = strchr(config + pos, '*');

		char *path = bmalloc(end - config - pos + 1);
		strncpy(path, config + pos, end - config - pos);

		path[end - config - pos] = '\0';

		obs_data_set_string(file, "value", path);
		bfree(path);

		obs_data_array_push_back(out, file);
		obs_data_release(file);

		const char *next = strchr(config + pos, '|');
		if (next == NULL)
			break;

		next = next + 1;
		pos = next - config;
	}

	return out;
}

static void parse_media_types(obs_data_t *attr, obs_data_t *source,
			      obs_data_t *settings)
{
	const char *playlist = obs_data_get_string(attr, "FilePlaylist");

	if (strcmp(playlist, "") != 0) {
		obs_data_set_string(source, "id", "vlc_source");

		obs_data_array_t *files = parse_playlist(playlist);

		obs_data_set_array(settings, "playlist", files);
		obs_data_array_release(files);

		long long end_op = obs_data_get_int(attr, "OpWhenFinished");

		if (end_op == 2)
			obs_data_set_bool(settings, "loop", true);
	} else {
		const char *url = obs_data_get_string(attr, "item");
		const char *sep = strstr(url, "://");

		if (sep != NULL) {
			char *protocol = bmalloc(sep - url + 1);
			strncpy(protocol, url, sep - url);

			if (strcmp(protocol, "smlndi") == 0) {
				obs_data_set_string(source, "id", "ndi_source");
			} else {
				obs_data_set_string(source, "id",
						    "ffmpeg_source");

				const char *info = strstr(url, "\\");
				char *dest;

				if (info != NULL) {
					dest = bmalloc(info - url + 1);
					strncpy(dest, url, info - url);
				} else {
					dest = bmalloc(strlen(url) + 1);
					strcpy(dest, url);
				}

				obs_data_set_string(settings, "input", dest);

				bfree(dest);

				obs_data_set_bool(settings, "is_local_file",
						  false);
			}

			bfree(protocol);
		} else {
			obs_data_set_string(source, "id", "ffmpeg_source");

			char *rep = string_replace(url, "\\", "/");
			obs_data_set_string(settings, "local_file", rep);
			bfree(rep);

			obs_data_set_bool(settings, "is_local_file", true);
		}
	}
}

static obs_data_t *parse_slideshow(const char *config)
{
	obs_data_t *out = obs_data_create();

	const char *start_array = strstr(config, "images&quot;:[");
	if (start_array == NULL)
		return out;

	start_array = (start_array + 13);

	const char *end_array = strchr(start_array, ']');
	if (end_array == NULL)
		return out;

	obs_data_array_t *files = obs_data_array_create();

	char *arr = bmalloc(end_array - start_array + 1);
	strncpy(arr, start_array, end_array - start_array + 1);
	arr[(end_array - start_array) + 1] = '\0';

	char *temp;
	rep(arr, "\\\\", "/", temp);
	rep(arr, "&quot;", "\"", temp);

	temp = arr + 2;
	char *end_pos = strchr(temp, '"');
	while (true) {
		char path[512];
		strncpy(path, temp, end_pos - temp);
		path[end_pos - temp] = '\0';

		obs_data_t *file = obs_data_create();
		obs_data_set_string(file, "value", path);
		obs_data_array_insert(files, obs_data_array_count(files), file);

		obs_data_release(file);
		if (strcmp(end_pos, "\"]") == 0)
			break;

		temp = end_pos + 3;
		end_pos = strchr(temp, '"');
	}

	bfree(arr);

	end_array = end_array + 2;

	char *options = bmalloc(strlen(end_array) + 2);
	options[0] = '{';
	strcpy(options + 1, end_array);
	options[strlen(end_array) + 2] = '\0';

	rep(options, "&quot;", "\"", temp);
	rep(options, "\\", "/", temp);

	obs_data_t *opt = obs_data_create_from_json(options);
	bfree(options);

	obs_data_set_bool(out, "randomize", obs_data_get_bool(opt, "random"));

	obs_data_set_int(out, "slide_time",
			 obs_data_get_int(opt, "delay") * 1000 + 700);

	obs_data_set_array(out, "files", files);

	obs_data_array_release(files);
	obs_data_release(opt);

	return out;
}

static bool source_name_exists(const char *name, obs_data_array_t *sources)
{
	int i = 0;
	obs_data_t *source = obs_data_array_item(sources, i++);
	while (source != NULL) {
		const char *s_name = obs_data_get_string(source, "name");
		obs_data_release(source);

		if (strcmp(s_name, name) == 0) {
			return true;
		}

		source = obs_data_array_item(sources, i++);
	}
	return false;
}

static obs_data_t *get_source_with_id(const char *src_id,
				      obs_data_array_t *sources)
{
	int i = 0;
	obs_data_t *source = obs_data_array_item(sources, i++);
	while (source != NULL) {
		const char *id = obs_data_get_string(source, "src_id");
		if (strcmp(id, src_id) == 0)
			return source;

		obs_data_release(source);
		source = obs_data_array_item(sources, i++);
	}

	return NULL;
}

static void parse_items(obs_data_array_t *in_items, obs_data_array_t *items,
			obs_data_array_t *sources)
{
	int i = 0;
	obs_data_t *item = obs_data_array_item(in_items, i++);
	while (item != NULL) {
		obs_data_t *attr = obs_data_get_obj(item, "attr");
		const char *src_id = obs_data_get_string(attr, "srcid");

		obs_data_t *source = obs_data_create();
		obs_data_t *settings = obs_data_create();
		const char *name;

		obs_data_t *s = get_source_with_id(src_id, sources);
		if (s != NULL) {
			obs_data_release(source);

			source = s;
			name = obs_data_get_string(s, "name");

			goto skip;
		}

		name = obs_data_get_string(attr, "cname");
		if (strcmp(name, "") == 0 || *name == '\0')
			name = obs_data_get_string(attr, "name");

		int x = 0;
		while (source_name_exists(name, sources)) {
			char new_name[512];
			sprintf(new_name, "%s %d", name, x);

			name = new_name;
		}

		long long vol = obs_data_get_int(attr, "volume");

		long long type = obs_data_get_int(attr, "type");

		/** type=1     means Media of some kind (Video Playlist, RTSP,
		               RTMP, NDI or Media File).
		    type=2     means either a DShow or WASAPI source.
		    type=4     means an Image source.
		    type=5     means either a Display or Window Capture.
		    type=7     means a Game Capture.
		    type=8     means rendered with a browser, which includes:
		                   Web Page, Image Slideshow, Text.
		    type=11    means another Scene. **/

		if (type == 5) {
			const char *opt = obs_data_get_string(attr, "item");

			char *path = string_replace(opt, "&lt;", "<");
			char *temp;
			rep(path, "&gt;", ">", temp);
			rep(path, "&quot;", "\"", temp);

			obs_data_t *options = parse_xml(path);
			bfree(path);

			obs_data_t *o_attr = obs_data_get_obj(options, "attr");
			const char *display =
				obs_data_get_string(o_attr, "desktop");

			if (strcmp(display, "") != 0 || *display == '\0') {
#if defined(_WIN32)
				obs_data_set_string(source, "id",
						    "monitor_capture");
				translate_bool("ScrCapShowMouse", attr,
					       "capture_cursor", settings);
#elif defined(__APPLE__)
				obs_data_set_string(source, "id",
						    "display_capture");
				translate_bool("ScrCapShowMouse", attr,
					       "show_cursor", settings);
#else
				obs_data_set_string(source, "id", "xshm_input");
				translate_bool("ScrCapShowMouse", attr,
					       "show_cursor", settings);
#endif
			} else {
#if defined(_WIN32)
				obs_data_set_string(source, "id",
						    "window_capture");

				const char *exec =
					obs_data_get_string(o_attr, "module");
				const char *window =
					obs_data_get_string(o_attr, "window");
				const char *class =
					obs_data_get_string(o_attr, "class");

				if (strcmp(class, "") == 0 || *class == '\0')
					class = "class";

				char res[1024];

				sprintf(res, "%s:%s:%s", window, class,
					strrchr(exec, '\\') + 1);

				obs_data_set_string(settings, "window", res);
				obs_data_set_int(settings, "priority", 2);
#elif defined(__APPLE__)
				obs_data_set_string(source, "id",
						    "window_capture");
#else
				obs_data_set_string(source, "id",
						    "xcomposite_input");
#endif
			}
			obs_data_release(options);
			obs_data_release(o_attr);
		} else if (type == 7) {
#if defined(_WIN32)
			const char *opt = obs_data_get_string(attr, "item");

			char *path = string_replace(opt, "&lt;", "<");
			char *temp;
			rep(path, "&gt;", ">", temp);
			rep(path, "&quot;", "\"", temp);

			obs_data_t *options = parse_xml(path);
			bfree(path);

			obs_data_t *o_attr = obs_data_get_obj(options, "attr");

			obs_data_set_string(source, "id", "game_capture");

			const char *name =
				obs_data_get_string(o_attr, "wndname");
			const char *exec =
				obs_data_get_string(o_attr, "imagename");

			char res[1024];

			sprintf(res, "%s::%s", name, exec);

			obs_data_set_string(settings, "window", res);
			obs_data_set_string(settings, "capture_mode", "window");

			obs_data_release(options);
			obs_data_release(o_attr);
#elif defined(__APPLE__)
			obs_data_set_string(source, "id", "syphon_capture");
#endif
		} else if (type == 4) {
			obs_data_set_string(source, "id", "image_source");

			const char *path = obs_data_get_string(attr, "item");
			char *rep = string_replace(path, "\\", "/");
			obs_data_set_string(settings, "path", rep);
			bfree(rep);
		} else if (type == 1) {
			parse_media_types(attr, source, settings);
		} else if (type == 2) {
			const char *audio =
				obs_data_get_string(attr, "itemaudio");

			if (strcmp(audio, "") == 0) {
#if defined(_WIN32)
				obs_data_set_string(source, "id",
						    "dshow_input");
#elif defined(__APPLE__)
				obs_data_set_string(source, "id",
						    "av_capture_input");
#else
				obs_data_set_string(source, "id", "v4l2_input");
#endif
			} else {
#if defined(_WIN32)
				obs_data_set_string(source, "id",
						    "wasapi_input_capture");

				const char *dev = strstr(audio, "\\wave:") + 6;

				char res[1024];
				sprintf(res, "{0.0.1.00000000}.%s", dev);

				size_t i = 0;
				while (*(res + i) != '\0') {
					*(res + i) = (char)tolower(*(res + i));
					i++;
				}

				obs_data_set_string(settings, "device_id", res);
#elif defined(__APPLE__)
				obs_data_set_string(source, "id",
						    "coreaudio_input_capture");
#else
				obs_data_set_string(source, "id",
						    "pulse_input_capture");
#endif
			}
		} else if (type == 8) {
			const char *plugin = obs_data_get_string(attr, "item");
			if (strncmp(plugin, "html:plugin:imageslideshowplg*",
				    30) == 0) {

				obs_data_set_string(source, "id", "slideshow");

				obs_data_release(settings);
				settings = parse_slideshow(plugin);
			} else if (strncmp(plugin, "html:plugin:titleplg*",
					   21) == 0) {

#ifdef _WIN32
				obs_data_set_string(source, "id",
						    "text_gdiplus");
#else
				obs_data_set_string(source, "id",
						    "text_ft2_source");
#endif

				obs_data_release(settings);
				settings = parse_text(plugin);
			} else if (strncmp(plugin, "html", 4) != 0) {
				obs_data_set_string(source, "id",
						    "browser_source");

				const char *end = strchr(plugin, '*');
				char *url = bmalloc(end - plugin + 1);
				strncpy(url, plugin, end - plugin);
				*(url + (end - plugin)) = '\0';

				obs_data_set_string(settings, "url", url);

				bfree(url);
			}
		} else if (type == 11) {
			const char *id = obs_data_get_string(attr, "item");
			obs_data_t *s = get_source_with_id(id, sources);

			name = obs_data_get_string(s, "name");
			obs_data_release(s);

			goto skip;
		}

		translate_string("srcid", attr, "src_id", source);
		obs_data_set_string(source, "name", name);
		obs_data_set_double(source, "volume", (double)vol / 100.0);

		obs_data_set_obj(source, "settings", settings);
		obs_data_array_insert(sources, obs_data_array_count(sources),
				      source);

	skip:
		obs_data_release(settings);
		obs_data_release(source);

		struct obs_video_info ovi;
		obs_get_video_info(&ovi);

		int width = ovi.base_width;
		int height = ovi.base_height;

		obs_data_t *out_item = obs_data_create();

		obs_data_set_int(out_item, "bounds_type", 2);

		double pos_left = obs_data_get_double(attr, "pos_left");
		double pos_right = obs_data_get_double(attr, "pos_right");
		double pos_top = obs_data_get_double(attr, "pos_top");
		double pos_bottom = obs_data_get_double(attr, "pos_bottom");

		obs_data_t *pos = obs_data_create();
		obs_data_set_double(pos, "x", pos_left * width);
		obs_data_set_double(pos, "y", pos_top * height);

		obs_data_t *bounds = obs_data_create();
		obs_data_set_double(bounds, "x",
				    (pos_right - pos_left) * width);
		obs_data_set_double(bounds, "y",
				    (pos_bottom - pos_top) * height);

		obs_data_set_obj(out_item, "pos", pos);
		obs_data_set_obj(out_item, "bounds", bounds);
		obs_data_set_string(out_item, "name", name);

		translate_bool("visible", attr, "visible", out_item);

		obs_data_array_insert(items, obs_data_array_count(items),
				      out_item);

		obs_data_release(out_item);
		obs_data_release(pos);
		obs_data_release(bounds);

		obs_data_release(attr);
		obs_data_release(item);
		item = obs_data_array_item(in_items, i++);
	}
}

static void parse_scenes(obs_data_t *res, obs_data_array_t *scenes)
{
	obs_data_array_t *sources = obs_data_array_create();

	const char *first = NULL;

	int i = 0;
	obs_data_t *scene = obs_data_array_item(scenes, i++);
	while (scene != NULL) {
		const char *type = obs_data_get_string(scene, "type");

		if (strcmp(type, "placement") == 0) {
			obs_data_t *out = obs_data_create();

			obs_data_t *attr = obs_data_get_obj(scene, "attr");

			translate_string("name", attr, "name", out);
			translate_string("id", attr, "src_id", out);

			if (first == NULL)
				first = obs_data_get_string(attr, "name");

			obs_data_release(attr);

			obs_data_set_string(out, "id", "scene");

			obs_data_array_insert(
				sources, obs_data_array_count(sources), out);

			obs_data_release(out);
		}

		obs_data_release(scene);
		scene = obs_data_array_item(scenes, i++);
	}

	i = 0;
	scene = obs_data_array_item(scenes, i++);
	while (scene != NULL) {
		obs_data_t *out = obs_data_array_item(sources, i - 1);

		obs_data_t *settings = obs_data_create();

		obs_data_array_t *in_items =
			obs_data_get_array(scene, "children");
		obs_data_array_t *items = obs_data_array_create();

		parse_items(in_items, items, sources);

		obs_data_set_array(settings, "items", items);
		obs_data_set_int(settings, "id_counter",
				 obs_data_array_count(items));
		obs_data_set_obj(out, "settings", settings);

		obs_data_release(settings);
		obs_data_array_release(items);
		obs_data_array_release(in_items);

		obs_data_release(out);
		obs_data_release(scene);
		scene = obs_data_array_item(scenes, i++);
	}

	obs_data_set_array(res, "sources", sources);
	if (first != NULL)
		obs_data_set_string(res, "current_scene", first);
	obs_data_array_release(sources);
}

#undef translate_int
#undef translate_string
#undef translate_double
#undef translate_bool

#undef rep

int xsplit_import(const char *path, const char *name, obs_data_t **res)
{
	if (name == NULL)
		name = "XSplit Import";

	if (*res == NULL) {
		obs_data_t *d = obs_data_create();
		*res = d;
	}

	char *file_data = os_quick_read_utf8_file(path);

	if (!file_data)
		return IMPORTER_FILE_WONT_OPEN;

	obs_data_t *parsed = parse_xml(file_data);

	obs_data_set_string(*res, "name", name);

	obs_data_array_t *scenes = obs_data_get_array(parsed, "children");
	parse_scenes(*res, scenes);
	obs_data_array_release(scenes);

	obs_data_release(parsed);

	bfree(file_data);

	return IMPORTER_SUCCESS;
fail:
	return IMPORTER_ERROR_DURING_CONVERSION;
}

static bool xsplit_check(const char *path)
{
	bool check = false;

	char *file_data = os_quick_read_utf8_file(path);

	if (!file_data)
		return false;

	char *pos = file_data;

	char *line = read_line(&pos);
	while (line != NULL) {
		if (strncmp(line, "<?xml", 5) == 0) {
			bfree(line);
			line = read_line(&pos);
		} else {
			if (strncmp(line, "<configuration", 14) == 0) {
				check = true;
			}
			bfree(line);
			break;
		}
	}

	bfree(file_data);

	return check;
}

static obs_importer_files xsplit_find_files()
{
	obs_importer_files res;
	da_init(res);
#ifdef _WIN32
	char dst[512];
	int found = os_get_program_data_path(
		dst, 512, "SplitMediaLabs\\XSplit\\Presentation2.0\\");
	if (found == -1)
		return res;

	os_dir_t *dir = os_opendir(dst);
	struct os_dirent *ent;
	while ((ent = os_readdir(dir)) != NULL) {
		if (ent->directory || *ent->d_name == '.')
			continue;

		if (strcmp(ent->d_name, "Placements.bpres") == 0) {
			char *str = bmalloc(512);
			strcpy(str, dst);
			strcat(str, ent->d_name);
			da_insert(res, 0, &str);

			break;
		}
	}
	os_closedir(dir);
#endif
	return res;
}

static char *xsplit_name(const char *path)
{
	const char *name = "XSplit Import";
	char *res = bmalloc(strlen(name) + 1);
	strcpy(res, name);

	UNUSED_PARAMETER(path);
	return res;
}

struct obs_importer xsplit_importer = {
	.prog = "XSplitBroadcaster",
	.import_scenes = xsplit_import,
	.check = xsplit_check,
	.name = xsplit_name,
	.find_files = xsplit_find_files,
};

//struct obs_importer gamecaster_importer = {
//	.prog = "XSplit Gamecaster",
//	.import_scenes = gamecaster_import,
//	.check = gamecaster_check,
//};
