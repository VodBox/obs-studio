/*****************************************************************************
Copyright (C) 2018 by VodBox <dillon@vodbox.io>

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
*****************************************************************************/
#include <obs-module.h>
#include <graphics/vec3.h>
#include <graphics/vec4.h>

struct levels_data {
	obs_source_t                   *context;

	gs_effect_t                    *effect;
	
	gs_eparam_t                    *gamma_param;
	gs_eparam_t                    *red_param;
	gs_eparam_t                    *green_param;
	gs_eparam_t                    *blue_param;

	struct vec3                    *gamma;
	struct vec4                    *red;
	struct vec4                    *green;
	struct vec4                    *blue;
};

static const char *levels_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("LevelsFilter");
}

static void levels_filter_destroy(void *data)
{
	struct levels_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

static void levels_filter_update(void *data, obs_data_t *settings)
{
	struct levels_data *filter = data;

	bool rgb = obs_data_get_bool(settings, "rgb");

	if (rgb) {
		double r_gamma = obs_data_get_double(settings, "r_gamma");
		r_gamma = (r_gamma < 0.0) ? (-r_gamma + 1.0) : (1.0 / (r_gamma + 1.0));
		float r_i_black = (float)obs_data_get_double(settings, "r_i_black");
		float r_i_white = (float)obs_data_get_double(settings, "r_i_white");
		float r_o_black = (float)obs_data_get_double(settings, "r_o_black");
		float r_o_white = (float)obs_data_get_double(settings, "r_o_white");
		double g_gamma = obs_data_get_double(settings, "g_gamma");
		g_gamma = (g_gamma < 0.0) ? (-g_gamma + 1.0) : (1.0 / (g_gamma + 1.0));
		float g_i_black = (float)obs_data_get_double(settings, "g_i_black");
		float g_i_white = (float)obs_data_get_double(settings, "g_i_white");
		float g_o_black = (float)obs_data_get_double(settings, "g_o_black");
		float g_o_white = (float)obs_data_get_double(settings, "g_o_white");
		double b_gamma = obs_data_get_double(settings, "b_gamma");
		b_gamma = (b_gamma < 0.0) ? (-b_gamma + 1.0) : (1.0 / (b_gamma + 1.0));
		float b_i_black = (float)obs_data_get_double(settings, "b_i_black");
		float b_i_white = (float)obs_data_get_double(settings, "b_i_white");
		float b_o_black = (float)obs_data_get_double(settings, "b_o_black");
		float b_o_white = (float)obs_data_get_double(settings, "b_o_white");



		vec3_set(&filter->gamma, (float)r_gamma, (float)g_gamma,
			(float)b_gamma);
		vec4_set(&filter->red, r_i_black, r_i_white, r_o_black, r_o_white);
		vec4_set(&filter->green, g_i_black, g_i_white, g_o_black, g_o_white);
		vec4_set(&filter->blue, b_i_black, b_i_white, b_o_black, b_o_white);
	} else {
		double gamma = obs_data_get_double(settings, "rgb_gamma");
		gamma = (gamma < 0.0) ? (-gamma + 1.0) : (1.0 / (gamma + 1.0));
		float i_black = (float)obs_data_get_double(settings, "rgb_i_black");
		float i_white = (float)obs_data_get_double(settings, "rgb_i_white");
		float o_black = (float)obs_data_get_double(settings, "rgb_o_black");
		float o_white = (float)obs_data_get_double(settings, "rgb_o_white");
		vec3_set(&filter->gamma, (float)gamma, (float)gamma,
			(float)gamma);
		vec4_set(&filter->red, i_black, i_white, o_black, o_white);
		vec4_set(&filter->green, i_black, i_white, o_black, o_white);
		vec4_set(&filter->blue, i_black, i_white, o_black, o_white);
	}
}

static void *levels_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct levels_data *filter =
		bzalloc(sizeof(struct levels_data));
	char *effect_path = obs_module_file("levels_filter.effect");

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->gamma_param = gs_effect_get_param_by_name(
			filter->effect, "gamma");
		filter->red_param = gs_effect_get_param_by_name(
			filter->effect, "red");
		filter->green_param = gs_effect_get_param_by_name(
			filter->effect, "green");
		filter->blue_param = gs_effect_get_param_by_name(
			filter->effect, "blue");
	}

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		levels_filter_destroy(filter);
		return NULL;
	}

	levels_filter_update(filter, settings);
	return filter;
}
static void levels_filter_render(void *data, gs_effect_t *effect)
{
	struct levels_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
		OBS_ALLOW_DIRECT_RENDERING))
		return;

	gs_effect_set_vec3(filter->gamma_param, &filter->gamma);
	gs_effect_set_vec4(filter->red_param, &filter->red);
	gs_effect_set_vec4(filter->green_param, &filter->green);
	gs_effect_set_vec4(filter->blue_param, &filter->blue);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

static bool rgb_modified(obs_properties_t *props,
	obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);

	bool enabled = obs_data_get_bool(settings, "rgb");
	obs_property_t *rgb_gamma = obs_properties_get(props, "rgb_gamma");
	obs_property_t *rgb_i_black = obs_properties_get(props, "rgb_i_black");
	obs_property_t *rgb_i_white = obs_properties_get(props, "rgb_i_white");
	obs_property_t *rgb_o_black = obs_properties_get(props, "rgb_o_black");
	obs_property_t *rgb_o_white = obs_properties_get(props, "rgb_o_white");

	obs_property_t *r_gamma = obs_properties_get(props, "r_gamma");
	obs_property_t *r_i_black = obs_properties_get(props, "r_i_black");
	obs_property_t *r_i_white = obs_properties_get(props, "r_i_white");
	obs_property_t *r_o_black = obs_properties_get(props, "r_o_black");
	obs_property_t *r_o_white = obs_properties_get(props, "r_o_white");
	obs_property_t *g_gamma = obs_properties_get(props, "g_gamma");
	obs_property_t *g_i_black = obs_properties_get(props, "g_i_black");
	obs_property_t *g_i_white = obs_properties_get(props, "g_i_white");
	obs_property_t *g_o_black = obs_properties_get(props, "g_o_black");
	obs_property_t *g_o_white = obs_properties_get(props, "g_o_white");
	obs_property_t *b_gamma = obs_properties_get(props, "b_gamma");
	obs_property_t *b_i_black = obs_properties_get(props, "b_i_black");
	obs_property_t *b_i_white = obs_properties_get(props, "b_i_white");
	obs_property_t *b_o_black = obs_properties_get(props, "b_o_black");
	obs_property_t *b_o_white = obs_properties_get(props, "b_o_white");

	obs_property_set_visible(rgb_gamma, !enabled);
	obs_property_set_visible(rgb_i_black, !enabled);
	obs_property_set_visible(rgb_i_white, !enabled);
	obs_property_set_visible(rgb_o_black, !enabled);
	obs_property_set_visible(rgb_o_white, !enabled);

	obs_property_set_visible(r_gamma, enabled);
	obs_property_set_visible(r_i_black, enabled);
	obs_property_set_visible(r_i_white, enabled);
	obs_property_set_visible(r_o_black, enabled);
	obs_property_set_visible(r_o_white, enabled);
	obs_property_set_visible(g_gamma, enabled);
	obs_property_set_visible(g_i_black, enabled);
	obs_property_set_visible(g_i_white, enabled);
	obs_property_set_visible(g_o_black, enabled);
	obs_property_set_visible(g_o_white, enabled);
	obs_property_set_visible(b_gamma, enabled);
	obs_property_set_visible(b_i_black, enabled);
	obs_property_set_visible(b_i_white, enabled);
	obs_property_set_visible(b_o_black, enabled);
	obs_property_set_visible(b_o_white, enabled);

	return true;
}

static obs_properties_t *levels_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *prop;
	prop = obs_properties_add_bool(props, "rgb", "Split RGB");

	obs_property_set_modified_callback(prop, rgb_modified);

	obs_properties_add_float_slider(props, "rgb_gamma",
		"Gamma", -3.0f, 3.0f, 0.01f);
	obs_properties_add_float_slider(props, "rgb_i_black",
		"Input Black", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "rgb_i_white",
		"Input White", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "rgb_o_black",
		"Output Black", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "rgb_o_white",
		"Output White", 0.0f, 1.0f, 0.01f);

	obs_properties_add_float_slider(props, "r_gamma",
		"Gamma (Red)", -3.0f, 3.0f, 0.01f);
	obs_properties_add_float_slider(props, "r_i_black",
		"Input Black (Red)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "r_i_white",
		"Input White (Red)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "r_o_black",
		"Output Black (Red)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "r_o_white",
		"Output White (Red)", 0.0f, 1.0f, 0.01f);

	obs_properties_add_float_slider(props, "g_gamma",
		"Gamma (Green)", -3.0f, 3.0f, 0.01f);
	obs_properties_add_float_slider(props, "g_i_black",
		"Input Black (Green)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "g_i_white",
		"Input White (Green)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "g_o_black",
		"Output Black (Green)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "g_o_white",
		"Output White (Green)", 0.0f, 1.0f, 0.01f);

	obs_properties_add_float_slider(props, "b_gamma",
		"Gamma (Blue)", -3.0f, 3.0f, 0.01f);
	obs_properties_add_float_slider(props, "b_i_black",
		"Input Black (Blue)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "b_i_white",
		"Input White (Blue)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "b_o_black",
		"Output Black (Blue)", 0.0f, 1.0f, 0.01f);
	obs_properties_add_float_slider(props, "b_o_white",
		"Output White (Blue)", 0.0f, 1.0f, 0.01f);

	UNUSED_PARAMETER(data);
	return props;
}

static void levels_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "rgb", false);
	obs_data_set_default_double(settings, "rgb_gamma", 0.0);
	obs_data_set_default_double(settings, "rgb_i_black", 0.0);
	obs_data_set_default_double(settings, "rgb_i_white", 1.0);
	obs_data_set_default_double(settings, "rgb_o_black", 0.0);
	obs_data_set_default_double(settings, "rgb_o_white", 1.0);
	obs_data_set_default_double(settings, "r_gamma", 0.0);
	obs_data_set_default_double(settings, "r_i_black", 0.0);
	obs_data_set_default_double(settings, "r_i_white", 1.0);
	obs_data_set_default_double(settings, "r_o_black", 0.0);
	obs_data_set_default_double(settings, "r_o_white", 1.0);
	obs_data_set_default_double(settings, "g_gamma", 0.0);
	obs_data_set_default_double(settings, "g_i_black", 0.0);
	obs_data_set_default_double(settings, "g_i_white", 1.0);
	obs_data_set_default_double(settings, "g_o_black", 0.0);
	obs_data_set_default_double(settings, "g_o_white", 1.0);
	obs_data_set_default_double(settings, "b_gamma", 0.0);
	obs_data_set_default_double(settings, "b_i_black", 0.0);
	obs_data_set_default_double(settings, "b_i_white", 1.0);
	obs_data_set_default_double(settings, "b_o_black", 0.0);
	obs_data_set_default_double(settings, "b_o_white", 1.0);
}

struct obs_source_info levels_filter = {
	.id = "levels_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = levels_filter_name,
	.create = levels_filter_create,
	.destroy = levels_filter_destroy,
	.video_render = levels_filter_render,
	.update = levels_filter_update,
	.get_properties = levels_filter_properties,
	.get_defaults = levels_filter_defaults
};