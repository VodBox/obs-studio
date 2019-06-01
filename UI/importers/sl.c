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

static char *translate_key(const char *sl_key)
{
	char *res = bmalloc(32);

	if (strncmp(sl_key, "Numpad", 6) == 0 && strlen(sl_key) == 7) {
		strcpy(res, "OBS_KEY_NUM");
		return strcat(res, (sl_key + 6));
	} else if (strncmp(sl_key, "Key", 3) == 0) {
		strcpy(res, "OBS_KEY_");
		return strcat(res, (sl_key + 3));
	} else if (strncmp(sl_key, "Digit", 5) == 0) {
		strcpy(res, "OBS_KEY_");
		return strcat(res, (sl_key + 5));
	} else if (strncmp(sl_key, "F", 1) == 0 && strlen(sl_key) < 4) {
		strcpy(res, "OBS_KEY_");
		return strcat(res, sl_key);
	}

#define add_translation(str, out)               \
	if (strcmp(sl_key, str) == 0) {         \
		bfree(res);                     \
		res = bmalloc(strlen(out) + 1); \
		strcpy(res, out);               \
		return res;                     \
	}

	add_translation("Backquote", "OBS_KEY_ASCIITILDE");
	add_translation("Backspace", "OBS_KEY_BACKSPACE");
	add_translation("Tab", "OBS_KEY_TAB");
	add_translation("Space", "OBS_KEY_SPACE");
	add_translation("Period", "OBS_KEY_PERIOD");
	add_translation("Slash", "OBS_KEY_SLASH");
	add_translation("Backslash", "OBS_KEY_BACKSLASH");
	add_translation("Minus", "OBS_KEY_MINUS");
	add_translation("Comma", "OBS_KEY_COMMA");
	add_translation("Plus", "OBS_KEY_PLUS");
	add_translation("Quote", "OBS_KEY_APOSTROPHE");
	add_translation("Semicolon", "OBS_KEY_SEMICOLON");
	add_translation("NumpadSubtract", "OBS_KEY_NUMMINUS");
	add_translation("NumpadAdd", "OBS_KEY_NUMPLUS");
	add_translation("NumpadDecimal", "OBS_KEY_NUMPERIOD");
	add_translation("NumpadDivide", "OBS_KEY_NUMSLASH");
	add_translation("NumpadMultiply", "OBS_KEY_NUMASTERISK");
	add_translation("Enter", "OBS_KEY_RETURN");
	add_translation("CapsLock", "OBS_KEY_CAPSLOCK");
	add_translation("NumLock", "OBS_KEY_NUMLOCK");
	add_translation("ScrollLock", "OBS_KEY_SCROLLLOCK");
	add_translation("Pause", "OBS_KEY_PAUSE");
	add_translation("Insert", "OBS_KEY_INSERT");
	add_translation("Home", "OBS_KEY_HOME");
	add_translation("End", "OBS_KEY_END");
	add_translation("Escape", "OBS_KEY_ESCAPE");
	add_translation("Delete", "OBS_KEY_DELETE");
	add_translation("ArrowUp", "OBS_KEY_UP");
	add_translation("ArrowDown", "OBS_KEY_DOWN");
	add_translation("ArrowLeft", "OBS_KEY_LEFT");
	add_translation("ArrowRight", "OBS_KEY_RIGHT");
	add_translation("PageUp", "OBS_KEY_PAGEUP");
	add_translation("PageDown", "OBS_KEY_PAGEDOWN");
	add_translation("BracketLeft", "OBS_KEY_BRACKETLEFT");
	add_translation("BracketRight", "OBS_KEY_BRACKETRIGHT");
#undef add_translation

	return NULL;
}

static char *translate_hotkey(obs_data_t *hotkey, const char *source)
{
	const char *name = obs_data_get_string(hotkey, "actionName");

	char *res = bmalloc(2);
	strcpy(res, "");

#define add_translation(in, str, out, source)   \
	if (strcmp(in, str) == 0) {             \
		bfree(res);                     \
		res = bmalloc(strlen(out) + 1); \
		strcpy(res, out);               \
                                                \
		if (source != NULL)             \
			strcat(res, source);    \
                                                \
		return res;                     \
	}

	add_translation(name, "TOGGLE_SOURCE_VISIBILITY_SHOW",
			"libobs.show_scene_item.", source);
	add_translation(name, "TOGGLE_SOURCE_VISIBILITY_HIDE",
			"libobs.hide_scene_item.", source);

	const char *nullptr = NULL;

	add_translation(name, "SWITCH_TO_SCENE", "OBSBasic.SelectScene",
			nullptr);

	add_translation(name, "TOGGLE_MUTE", "libobs.mute", nullptr);
	add_translation(name, "TOGGLE_UNMUTE", "libobs.unmute", nullptr);
	add_translation(name, "PUSH_TO_MUTE", "libobs.push-to-mute", nullptr);
	add_translation(name, "PUSH_TO_TALK", "libobs.push-to-talk", nullptr);
	add_translation(name, "GAME_CAPTURE_HOTKEY_START", "hotkey_start",
			nullptr);
	add_translation(name, "GAME_CAPTURE_HOTKEY_STOP", "hotkey_stop",
			nullptr);

	return res;
#undef add_translation
}

static const char *get_source_name_from_id(obs_data_t *root, const char *id)
{
	obs_data_t *in_sources = obs_data_get_obj(root, "sources");
	obs_data_array_t *source_arr = obs_data_get_array(in_sources, "items");

	int i = 0;
	obs_data_t *item = obs_data_array_item(source_arr, i++);
	while (item) {
		const char *source_id = obs_data_get_string(item, "id");

		if (strcmp(source_id, id) == 0) {
			obs_data_release(item);
			obs_data_release(in_sources);
			obs_data_array_release(source_arr);

			return obs_data_get_string(item, "name");
		}

		obs_data_release(item);
		item = obs_data_array_item(source_arr, i++);
	}

	obs_data_release(in_sources);
	obs_data_array_release(source_arr);

	obs_data_t *in_scenes = obs_data_get_obj(root, "scenes");
	obs_data_array_t *scene_arr = obs_data_get_array(in_scenes, "items");

	i = 0;
	item = obs_data_array_item(scene_arr, i++);
	while (item) {
		const char *source_id = obs_data_get_string(item, "id");

		if (strcmp(source_id, id) == 0) {
			obs_data_release(item);
			obs_data_release(in_scenes);
			obs_data_array_release(scene_arr);

			return obs_data_get_string(item, "name");
		}

		obs_data_release(item);
		item = obs_data_array_item(scene_arr, i++);
	}

	obs_data_release(in_scenes);
	obs_data_array_release(scene_arr);

	return NULL;
}

static void get_hotkey_bindings(obs_data_t *out_hotkeys,
				obs_data_array_t *in_hotkeys, const char *name)
{
	int i = 0;
	obs_data_t *hotkey = obs_data_array_item(in_hotkeys, i++);
	while (hotkey != NULL) {
		obs_data_array_t *out_hotkey = obs_data_array_create();
		obs_data_array_t *bindings =
			obs_data_get_array(hotkey, "bindings");

		char *hotkey_name = translate_hotkey(hotkey, name);

		int index = 0;
		obs_data_t *binding = obs_data_array_item(bindings, index++);
		while (binding != NULL) {
			obs_data_t *b = obs_data_create();
			obs_data_t *modifiers =
				obs_data_get_obj(binding, "modifiers");

			obs_data_set_bool(b, "control",
					  obs_data_get_bool(modifiers, "ctrl"));
			obs_data_set_bool(b, "shift",
					  obs_data_get_bool(modifiers,
							    "shift"));
			obs_data_set_bool(b, "command",
					  obs_data_get_bool(modifiers, "meta"));
			obs_data_set_bool(b, "alt",
					  obs_data_get_bool(modifiers, "alt"));

			char *key = translate_key(
				obs_data_get_string(binding, "key"));

			obs_data_set_string(b, "key", key);

			bfree(key);

			obs_data_array_insert(out_hotkey, 0, b);
			obs_data_release(b);
			obs_data_release(modifiers);

			obs_data_release(binding);
			binding = obs_data_array_item(bindings, index++);
		}

		obs_data_array_release(bindings);

		obs_data_set_array(out_hotkeys, hotkey_name, out_hotkey);
		obs_data_array_release(out_hotkey);

		obs_data_release(hotkey);
		hotkey = obs_data_array_item(in_hotkeys, i++);

		bfree(hotkey_name);
	}
}

static void get_scene_items(obs_data_t *root, obs_data_t *scene,
			    obs_data_array_t *in)
{
	obs_data_t *out = obs_data_create();
	obs_data_array_t *items_out = obs_data_array_create();
	int length = 0;
	obs_data_t *hotkeys = obs_data_get_obj(scene, "hotkeys");

	int i = 0;
	obs_data_t *item = obs_data_array_item(in, i++);
	while (item) {
		obs_data_t *out_item = obs_data_create();
		obs_data_t *pos = obs_data_create();
		obs_data_t *scale = obs_data_create();

		obs_data_t *in_crop = obs_data_get_obj(item, "crop");

		const char *id = obs_data_get_string(item, "sourceId");
		const char *name = get_source_name_from_id(root, id);

		obs_data_set_double(pos, "x", obs_data_get_double(item, "x"));
		obs_data_set_double(pos, "y", obs_data_get_double(item, "y"));
		obs_data_set_double(scale, "x",
				    obs_data_get_double(item, "scaleX"));
		obs_data_set_double(scale, "y",
				    obs_data_get_double(item, "scaleY"));

		obs_data_t *in_hotkeys = obs_data_get_obj(item, "hotkeys");
		obs_data_array_t *hotkey_items =
			obs_data_get_array(in_hotkeys, "items");
		obs_data_t *out_hotkeys = obs_data_get_obj(scene, "hotkeys");

		get_hotkey_bindings(out_hotkeys, hotkey_items, name);

		obs_data_set_double(out_item, "crop_top",
				    obs_data_get_double(in_crop, "top"));
		obs_data_set_double(out_item, "crop_bottom",
				    obs_data_get_double(in_crop, "bottom"));
		obs_data_set_double(out_item, "crop_left",
				    obs_data_get_double(in_crop, "left"));
		obs_data_set_double(out_item, "crop_right",
				    obs_data_get_double(in_crop, "right"));

		obs_data_set_int(out_item, "id", length++);
		obs_data_set_string(out_item, "name", name);

		obs_data_set_obj(out_item, "pos", pos);
		obs_data_set_obj(out_item, "scale", scale);

		obs_data_set_double(out_item, "rot",
				    obs_data_get_double(item, "rotation"));

		obs_data_set_bool(out_item, "visible",
				  obs_data_get_bool(item, "visible"));

		obs_data_array_insert(
			items_out, obs_data_array_count(items_out), out_item);

		obs_data_array_release(hotkey_items);
		obs_data_release(in_crop);
		obs_data_release(out_hotkeys);
		obs_data_release(in_hotkeys);
		obs_data_release(out_item);
		obs_data_release(pos);
		obs_data_release(scale);

		obs_data_release(item);
		item = obs_data_array_item(in, i++);
	}

	obs_data_set_array(out, "items", items_out);
	obs_data_set_int(out, "id_counter", length);
	obs_data_set_obj(scene, "settings", out);

	obs_data_release(out);
	obs_data_array_release(items_out);
	obs_data_release(hotkeys);
}

static int attempt_import(obs_data_t *root, const char *name, obs_data_t **res)
{
	obs_data_t *sources = obs_data_get_obj(root, "sources");
	obs_data_array_t *source_arr = obs_data_get_array(sources, "items");

	obs_data_t *scenes = obs_data_get_obj(root, "scenes");
	obs_data_array_t *scene_arr = obs_data_get_array(scenes, "items");

	obs_data_t *t = obs_data_get_obj(root, "transitions");
	obs_data_array_t *t_arr = obs_data_get_array(t, "items");

	obs_data_release(sources);
	obs_data_release(scenes);
	obs_data_release(t);

	obs_data_array_t *out_sources = obs_data_array_create();

	int i = 0;
	obs_data_t *source = obs_data_array_item(source_arr, i++);

	while (source != NULL) {
		obs_data_t *out = obs_data_create();

		obs_data_t *out_hotkeys = obs_data_create();

		obs_data_t *in_hotkeys = obs_data_get_obj(source, "hotkeys");
		obs_data_array_t *hotkey_items =
			obs_data_get_array(in_hotkeys, "items");
		obs_data_t *in_filters = obs_data_get_obj(source, "filters");
		obs_data_array_t *filter_items =
			obs_data_get_array(in_filters, "items");

		obs_data_t *in_settings = obs_data_get_obj(source, "settings");
		obs_data_t *in_sync = obs_data_get_obj(source, "syncOffset");

		int sync = (int)(obs_data_get_int(in_sync, "sec") * 1000000000 +
				 obs_data_get_int(in_sync, "nsec"));

		double vol = obs_data_get_double(source, "volume");
		bool muted = obs_data_get_bool(source, "muted");
		const char *name = obs_data_get_string(source, "name");
		int monitoring =
			(int)obs_data_get_int(source, "monitoringType");

		get_hotkey_bindings(out_hotkeys, hotkey_items, NULL);

		for (size_t i = 0; i < obs_data_array_count(filter_items);
		     i++) {
			obs_data_t *filter =
				obs_data_array_item(filter_items, i);
			const char *type = obs_data_get_string(filter, "type");
			obs_data_set_string(filter, "id", type);

			obs_data_release(filter);
		}

		obs_data_set_obj(out, "hotkeys", out_hotkeys);
		obs_data_set_array(out, "filters", filter_items);
		obs_data_set_string(out, "id",
				    obs_data_get_string(source, "type"));
		obs_data_set_obj(out, "settings", in_settings);
		obs_data_set_int(out, "sync", sync);
		obs_data_set_double(out, "volume", vol);
		obs_data_set_bool(out, "muted", muted);
		obs_data_set_string(out, "name", name);
		obs_data_set_int(out, "monitoring_type", monitoring);

		obs_data_array_insert(out_sources, 0, out);

		obs_data_release(in_sync);
		obs_data_release(in_settings);
		obs_data_array_release(filter_items);
		obs_data_array_release(hotkey_items);
		obs_data_release(in_filters);
		obs_data_release(in_hotkeys);
		obs_data_release(out_hotkeys);
		obs_data_release(out);

		obs_data_release(source);
		source = obs_data_array_item(source_arr, i++);
	}

	const char *scene_name = NULL;

	i = 0;
	obs_data_t *scene = obs_data_array_item(scene_arr, i++);

	while (scene != NULL) {
		obs_data_t *out = obs_data_create();

		if (!scene_name)
			scene_name = obs_data_get_string(scene, "name");

		obs_data_t *out_hotkeys = obs_data_create();

		obs_data_t *in_hotkeys = obs_data_get_obj(scene, "hotkeys");
		obs_data_array_t *hotkey_items =
			obs_data_get_array(in_hotkeys, "items");
		obs_data_t *in_filters = obs_data_get_obj(scene, "filters");
		obs_data_array_t *filter_items =
			obs_data_get_array(in_filters, "items");

		obs_data_t *in_settings = obs_data_get_obj(scene, "settings");
		obs_data_t *in_sync = obs_data_get_obj(scene, "syncOffset");

		long long sync = obs_data_get_int(in_sync, "sec") * 1000000000 +
				 obs_data_get_int(in_sync, "nsec");

		double vol = obs_data_get_double(scene, "volume");
		bool muted = obs_data_get_bool(scene, "muted");
		const char *name = obs_data_get_string(scene, "name");
		long long monitoring =
			obs_data_get_int(scene, "monitoringType");

		get_hotkey_bindings(out_hotkeys, hotkey_items, NULL);

		for (size_t i = 0; i < obs_data_array_count(filter_items);
		     i++) {
			obs_data_t *filter =
				obs_data_array_item(filter_items, i);
			const char *type = obs_data_get_string(filter, "type");
			obs_data_set_string(filter, "id", type);

			obs_data_release(filter);
		}

		obs_data_set_obj(out, "hotkeys", out_hotkeys);
		obs_data_set_array(out, "filters", filter_items);
		obs_data_set_string(out, "id", "scene");
		obs_data_set_obj(out, "settings", in_settings);
		obs_data_set_int(out, "sync", sync);
		obs_data_set_double(out, "volume", vol);
		obs_data_set_bool(out, "muted", muted);
		obs_data_set_string(out, "name", name);
		obs_data_set_int(out, "monitoring_type", monitoring);

		obs_data_t *empty = obs_data_create();
		obs_data_set_obj(out, "private_settings", empty);
		obs_data_release(empty);

		obs_data_t *in_items = obs_data_get_obj(scene, "sceneItems");
		obs_data_array_t *items_arr =
			obs_data_get_array(in_items, "items");

		get_scene_items(root, out, items_arr);

		obs_data_array_insert(out_sources,
				      obs_data_array_count(out_sources), out);

		obs_data_release(in_sync);
		obs_data_release(in_settings);
		obs_data_array_release(filter_items);
		obs_data_array_release(hotkey_items);
		obs_data_release(in_filters);
		obs_data_release(in_hotkeys);
		obs_data_release(out_hotkeys);
		obs_data_release(out);
		obs_data_array_release(items_arr);
		obs_data_release(in_items);

		obs_data_release(scene);
		scene = obs_data_array_item(scene_arr, i++);
	}

	obs_data_set_array(*res, "sources", out_sources);
	obs_data_set_string(*res, "current_scene", scene_name);
	obs_data_set_string(*res, "current_program_scene", scene_name);

	if (!name)
		obs_data_set_string(*res, "name", "StreamLabs Import");
	else
		obs_data_set_string(*res, "name", name);

	obs_data_array_release(out_sources);
	obs_data_array_release(source_arr);
	obs_data_array_release(scene_arr);
	obs_data_array_release(t_arr);

	return IMPORTER_SUCCESS;

fail:
	return IMPORTER_ERROR_DURING_CONVERSION;
}

static char *streamlabs_name(const char *path)
{
	char name[1024];

	char *folder = get_folder_from_path(path);
	char *manifest_file = get_filename_from_path(path);
	char manifest_path[1024];

	strcpy(manifest_path, folder);
	strcat(manifest_path, "manifest.json");

	if (os_file_exists(manifest_path)) {
		obs_data_t *data =
			obs_data_create_from_json_file(manifest_path);

		if (data != NULL) {
			obs_data_array_t *collections =
				obs_data_get_array(data, "collections");

			bool name_set = false;

			int i = 0;
			obs_data_t *c = obs_data_array_item(collections, i++);
			while (c != NULL) {
				const char *c_id = obs_data_get_string(c, "id");

				const char *c_name =
					obs_data_get_string(c, "name");

				if (strcmp(c_id, manifest_file) == 0) {
					strcpy(name, c_name);
					name_set = true;
					obs_data_release(c);
					break;
				}

				obs_data_release(c);
				c = obs_data_array_item(collections, i++);
			}

			if (!name_set) {
				strcpy(name, "Unknown Streamlabs Import");
			}

			obs_data_array_release(collections);
			obs_data_release(data);
		}
	} else {
		strcpy(name, "Unknown Streamlabs Import");
	}

	bfree(folder);
	bfree(manifest_file);

	char *res = bmalloc(strlen(name) + 1);
	strcpy(res, name);

	return res;
}

static int streamlabs_import(const char *path, const char *name,
			     obs_data_t **res)
{
	obs_data_t *data = obs_data_create_from_json_file(path);

	if (data) {
		int result = IMPORTER_ERROR_DURING_CONVERSION;

		const char *node_type = obs_data_get_string(data, "nodeType");

		if (*res == NULL) {
			obs_data_t *d = obs_data_create();
			*res = d;
		}

		if (strcmp(node_type, "RootNode") == 0) {
			if (name == NULL) {
				char *auto_name = streamlabs_name(path);
				result = attempt_import(data, auto_name, res);

				bfree(auto_name);
			} else {
				result = attempt_import(data, name, res);
			}
		}

		translate_os_studio(*res);

		obs_data_release(data);
		return result;
	}

	return IMPORTER_FILE_WONT_OPEN;
}

static bool streamlabs_check(const char *path)
{
	bool check = false;

	char *file_data = os_quick_read_utf8_file(path);

	if (file_data) {
		obs_data_t *root = obs_data_create_from_json(file_data);

		if (root != NULL) {
			const char *node_type =
				obs_data_get_string(root, "nodeType");

			if (node_type != NULL &&
			    strcmp(node_type, "RootNode") == 0)
				check = true;

			obs_data_release(root);
		}
		bfree(file_data);
	}

	return check;
}

static obs_importer_files streamlabs_find_files()
{
	obs_importer_files res;
	da_init(res);
#ifdef _WIN32
	char dst[512];

	int found = os_get_config_path(dst, 512,
				       "slobs-client\\SceneCollections\\");
	if (found == -1)
		return res;

	os_dir_t *dir = os_opendir(dst);
	struct os_dirent *ent;
	while ((ent = os_readdir(dir)) != NULL) {
		if (ent->directory || *ent->d_name == '.' ||
		    strcmp(ent->d_name, "manifest.json") == 0)
			continue;

		char *pos = strstr(ent->d_name, ".json");
		char *end_pos = (ent->d_name + strlen(ent->d_name)) - 5;
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

struct obs_importer sl_importer = {
	.prog = "Streamlabs",
	.import_scenes = streamlabs_import,
	.name = streamlabs_name,
	.check = streamlabs_check,
	.find_files = streamlabs_find_files,
};
