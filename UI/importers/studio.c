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

void translate_os_studio(obs_data_t *res)
{
	obs_data_array_t *sources = obs_data_get_array(res, "sources");

	for (size_t i = 0; i < obs_data_array_count(sources); i++) {
		obs_data_t *source = obs_data_array_item(sources, i);
		obs_data_t *settings = obs_data_get_obj(source, "settings");

		const char *id = obs_data_get_string(source, "id");

#define direct_translation(before, after)                 \
	if (strcmp(id, before) == 0) {                    \
		obs_data_set_string(source, "id", after); \
	}

#define clear_translation(before, after)                  \
	if (strcmp(id, before) == 0) {                    \
		obs_data_set_string(source, "id", after); \
		obs_data_t *s = obs_data_create();        \
		obs_data_set_obj(source, "settings", s);  \
		obs_data_release(s);                      \
	}

#ifdef __APPLE__
		direct_translation("text_gdiplus", "text_ft2_source");

		clear_translation("game_capture", "syphon-input");

		clear_translation("wasapi_input_capture",
				  "coreaudio_input_capture");
		clear_translation("wasapi_output_capture",
				  "coreaudio_output_capture");
		clear_translation("pulse_input_capture",
				  "coreaudio_input_capture");
		clear_translation("pulse_output_capture",
				  "coreaudio_output_capture");

		clear_translation("jack_output_capture",
				  "coreaudio_output_capture");
		clear_translation("alsa_input_capture",
				  "coreaudio_input_capture");

		clear_translation("dshow_input", "av_capture_input");
		clear_translation("v4l2_input", "av_capture_input");

		clear_translation("xcomposite_input", "window_capture");

		if (strcmp(id, "monitor_capture") == 0) {
			if (obs_data_has_user_value(settings,
						    "capture_cursor")) {
				bool cursor = obs_data_get_bool(
					settings, "capture_cursor");

				obs_data_set_bool(settings, "show_cursor",
						  cursor);
			}
		}

		direct_translation("xshm_input", "monitor_capture");
#elif defined(_WIN32)
		direct_translation("text_ft2_source", "text_gdiplus");

		clear_translation("syphon-input", "game_capture");

		clear_translation("coreaudio_input_capture",
				  "wasapi_input_capture");
		clear_translation("coreaudio_output_capture",
				  "wasapi_output_capture");
		clear_translation("pulse_input_capture",
				  "wasapi_input_capture");
		clear_translation("pulse_output_capture",
				  "wasapi_output_capture");

		clear_translation("jack_output_capture",
				  "wasapi_output_capture");
		clear_translation("alsa_input_capture", "wasapi_input_capture");

		clear_translation("av_capture_input", "dshow_input");
		clear_translation("v4l2_input", "dshow_input");

		clear_translation("xcomposite_input", "window_capture");

		if (strcmp(id, "monitor_capture") == 0) {
			if (obs_data_has_user_value(settings, "show_cursor")) {
				bool cursor = obs_data_get_bool(settings,
								"show_cursor");

				obs_data_set_bool(settings, "capture_cursor",
						  cursor);
			}
		}

		if (strcmp(id, "xshm_input") == 0) {
			bool cursor =
				obs_data_get_bool(settings, "show_cursor");
			obs_data_set_bool(settings, "capture_cursor", cursor);

			obs_data_set_string(source, "id", "monitor_capture");
		}
#else
		direct_translation("text_gdiplus", "text_ft2_source");

		clear_translation("coreaudio_input_capture",
				  "pulse_input_capture");
		clear_translation("coreaudio_output_capture",
				  "pulse_output_capture");
		clear_translation("wasapi_input_capture",
				  "pulse_input_capture");
		clear_translation("wasapi_output_capture",
				  "pulse_output_capture");

		clear_translation("av_capture_input", "v4l2_input");
		clear_translation("dshow_input", "v4l2_input");

		clear_translation("window_capture", "xcomposite_input");

		if (strcmp(id, "monitor_capture") == 0) {
			obs_data_set_string(source, "id", "xshm_input");

			bool cursor =
				obs_data_get_bool(settings, "show_cursor");

			if (!cursor)
				obs_data_get_bool(settings, "capture_cursor");

			obs_data_set_bool(settings, "show_cursor", cursor);
		}
#endif

		obs_data_release(source);
		obs_data_release(settings);
	}

	obs_data_array_release(sources);
}

static bool studio_check(const char *path)
{
	obs_data_t *collection = obs_data_create_from_json_file(path);
	if (collection == NULL)
		return false;

	obs_data_array_t *sources = obs_data_get_array(collection, "sources");
	if (sources == NULL)
		goto fail;

	obs_data_array_release(sources);

	const char *name = obs_data_get_string(collection, "name");
	if (name == NULL)
		goto fail;

	const char *cur_scene =
		obs_data_get_string(collection, "current_scene");
	if (cur_scene == NULL)
		goto fail;

	obs_data_release(collection);
	return true;
fail:
	obs_data_release(collection);
	return false;
}

static char *studio_name(const char *path)
{
	obs_data_t *d = obs_data_create_from_json_file(path);
	const char *name = obs_data_get_string(d, "name");

	char *res = bmalloc(strlen(name) + 1);
	strcpy(res, name);

	obs_data_release(d);

	return res;
}

static int studio_import(const char *path, const char *name, obs_data_t **res)
{
	if (!os_file_exists(path))
		return IMPORTER_FILE_NOT_FOUND;

	if (!studio_check(path))
		return IMPORTER_FILE_NOT_RECOGNISED;

	if (*res != NULL)
		obs_data_release(*res);

	obs_data_t *d = obs_data_create_from_json_file(path);
	translate_os_studio(d);
	*res = d;

	if (d != NULL) {
		if (name != NULL)
			obs_data_set_string(*res, "name", name);

		return IMPORTER_SUCCESS;
	} else
		return IMPORTER_UNKNOWN_ERROR;
}

struct obs_importer studio_importer = {
	.prog = "OBSStudio",
	.import_scenes = studio_import,
	.name = studio_name,
	.check = studio_check,
};
