#include <obs-module.h>

OBS_DECLARE_MODULE()
MODULE_EXPORT const char *obs_module_description(void)
{
	return "sRGB Repro";
}

struct repro_data {
	obs_source_t *context;

	gs_effect_t *effect;

	gs_eparam_t *color_param;

	struct vec4 color;
};

static const char *repro_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Repro";
}

static void repro_destroy(void *data)
{
	struct repro_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

static void repro_update(void *data, obs_data_t *settings) {
	struct repro_data *filter = data;

	uint32_t color = (uint32_t)obs_data_get_int(settings, "color");
	vec4_from_rgba(&filter->color, color);
}

static void *repro_create(obs_data_t *settings, obs_source_t *context)
{
	struct repro_data *filter = bzalloc(sizeof(struct repro_data));

	char *effect_path = obs_module_file("repro.effect");

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);

	if (filter->effect) {
		filter->color_param =
			gs_effect_get_param_by_name(filter->effect, "color");
	}

	obs_leave_graphics();

	bfree(effect_path);

	repro_update(filter, settings);
	return filter;
}

static void repro_render(void *data, gs_effect_t *effect)
{
	struct repro_data *filter = data;

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	gs_effect_set_vec4(filter->color_param, &filter->color);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

static obs_properties_t *repro_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_color(props, "color", "Color");

	return props;
}

static void repro_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "color", 0xFF00FF00);
}

struct obs_source_info repro_filter = {
	.id = "repro_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = repro_name,
	.create = repro_create,
	.destroy = repro_destroy,
	.video_render = repro_render,
	.update = repro_update,
	.get_properties = repro_properties,
	.get_defaults = repro_defaults,
};

bool obs_module_load(void)
{
	obs_register_source(&repro_filter);
	return true;
}
