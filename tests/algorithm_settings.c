#include <assert.h>

#include "../src/algorithm_settings.c"

static int errors;

void error(char *format, ...)
{
	UNUSED(format);
	errors++;
}

void request_algorithm_restart(struct main_window *w, int light)
{
	UNUSED(w);
	UNUSED(light);
}

int main(void)
{
	struct main_window w = {0};
	w.config_file = g_key_file_new();
	g_key_file_set_double(w.config_file, "algorithm", "taper", 125);
	g_key_file_set_double(w.config_file, "algorithm", "cal_count", 500);
	g_key_file_set_double(w.config_file, "algorithm", "amp_step", 1);
	algorithm_settings_load(&w);
	assert(errors == 1);
	assert(algorithm_parameters[ALG_TAPER] == 125);
	assert(algorithm_parameters[ALG_CAL_COUNT] == 500);
	assert(algorithm_parameters[ALG_AMP_STEP] == parameters[ALG_AMP_STEP].initial);
	assert(sample_windows_valid(algorithm_parameters));

	double values[ALG_COUNT];
	memcpy(values, algorithm_parameters, sizeof(values));
	values[ALG_NOISE_SEGMENT] = 2000;
	assert(!sample_windows_valid(values));
	memcpy(values, algorithm_parameters, sizeof(values));
	values[ALG_TIC_SMOOTH] = 50;
	assert(!sample_windows_valid(values));
	memcpy(values, algorithm_parameters, sizeof(values));
	values[ALG_CAL_LOCK_LOW] = 60;
	values[ALG_CAL_LOCK_HIGH] = 55;
	assert(!sample_windows_valid(values));

	for(int i = 0; i < ALG_COUNT; i++) values[i] = parameters[i].initial;
	gboolean calibration_edits[ALG_COUNT] = {0};
	values[ALG_AMP_MIN] = 120;
	values[ALG_CAL_COUNT] = 500;
	values[ALG_CAL_INTERVAL] = 1.1;
	calibration_edits[ALG_CAL_INTERVAL] = TRUE;
	g_key_file_set_double(w.config_file, "algorithm", "filter", parameters[ALG_FILTER].initial);
	g_key_file_set_double(w.config_file, "algorithm", "cal_edge", parameters[ALG_CAL_EDGE].initial);
	g_key_file_set_string(w.config_file, "algorithm", "future_setting", "keep");
	set_algorithm_values(w.config_file, values, calibration_edits);
	assert(!g_key_file_has_key(w.config_file, "algorithm", "filter", NULL));
	assert(!g_key_file_has_key(w.config_file, "algorithm", "cal_edge", NULL));
	assert(g_key_file_get_double(w.config_file, "algorithm", "amp_min", NULL) == 120);
	assert(g_key_file_get_double(w.config_file, "algorithm", "cal_count", NULL) == 500);
	assert(g_key_file_get_double(w.config_file, "algorithm", "cal_interval", NULL) == 1.1);
	assert(g_key_file_get_double(w.config_file, "algorithm", "taper", NULL) == 125);
	assert(g_key_file_get_double(w.config_file, "algorithm", "amp_step", NULL) == 1);
	assert(g_key_file_has_key(w.config_file, "algorithm", "future_setting", NULL));
	values[ALG_CAL_COUNT] = 600;
	set_algorithm_values(w.config_file, values, calibration_edits);
	assert(g_key_file_get_double(w.config_file, "algorithm", "cal_count", NULL) == 500);
	calibration_edits[ALG_CAL_COUNT] = TRUE;
	set_algorithm_values(w.config_file, values, calibration_edits);
	assert(g_key_file_get_double(w.config_file, "algorithm", "cal_count", NULL) == 600);
	algorithm_settings_load(&w);
	assert(algorithm_parameters[ALG_CAL_COUNT] == 600);
	assert(algorithm_parameters[ALG_CAL_INTERVAL] == 1.1);
	g_key_file_set_string(w.config_file, "algorithm", "cal_edge", "invalid");
	values[ALG_AMP_MIN] = parameters[ALG_AMP_MIN].initial;
	values[ALG_CAL_INTERVAL] = parameters[ALG_CAL_INTERVAL].initial;
	values[ALG_CAL_COUNT] = parameters[ALG_CAL_COUNT].initial;
	set_algorithm_values(w.config_file, values, calibration_edits);
	assert(!g_key_file_has_key(w.config_file, "algorithm", "amp_min", NULL));
	assert(!g_key_file_has_key(w.config_file, "algorithm", "cal_interval", NULL));
	assert(!g_key_file_has_key(w.config_file, "algorithm", "cal_count", NULL));
	gchar *invalid_edge = g_key_file_get_string(w.config_file, "algorithm", "cal_edge", NULL);
	assert(!strcmp(invalid_edge, "invalid"));
	g_free(invalid_edge);

	g_key_file_set_comment(w.config_file, "algorithm", "taper", "Keep this comment", NULL);
	gchar *before = g_key_file_to_data(w.config_file, NULL, NULL);
	GtkWidget *spins[ALG_COUNT] = {0};
	w.config_file_name = "";
	assert(!save_algorithm_settings(&w, spins, 0));
	gchar *after = g_key_file_to_data(w.config_file, NULL, NULL);
	assert(!strcmp(before, after));
	g_free(before);
	g_free(after);
	g_key_file_free(w.config_file);
	return 0;
}
