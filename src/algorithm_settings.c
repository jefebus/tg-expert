/*
    tg-expert
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include "tg.h"

enum { GROUP_FILTER, GROUP_TIMING, GROUP_AMPLITUDE, GROUP_CALIBRATION, GROUP_COUNT };
enum { RESPONSE_RESET = 1 };

struct parameter_info {
	const char *key, *label;
	int expert, group, digits;
	double initial, min, max, step;
};

static const struct parameter_info parameters[ALG_COUNT] = {
	{"filter", N_("Filter cutoff (Hz)"), 0, GROUP_FILTER, 0, 3000, 500, 5000, 100},
	{"noise_window", N_("Noise energy window (ms)"), 1, GROUP_FILTER, 0, 20, 10, 30, 1},
	{"noise_segment", N_("Noise reference segment (ms)"), 1, GROUP_FILTER, 0, 500, 300, 700, 10},
	{"noise_threshold", N_("Noise threshold (× reference)"), 0, GROUP_FILTER, 1, 2, 1, 3, 0.1},
	{"taper", N_("FFT edge taper (ms)"), 1, GROUP_FILTER, 0, 100, 50, 150, 5},
	{"lobes", N_("Maximum peak lobes"), 1, GROUP_TIMING, 0, 20, 10, 30, 1},
	{"guess_search", N_("Period search tolerance (ms)"), 1, GROUP_TIMING, 0, 20, 10, 30, 1},
	{"harmonic", N_("Subharmonic strength (%)"), 1, GROUP_TIMING, 0, 90, 80, 100, 1},
	{"double_trigger", N_("Double-trigger threshold (%)"), 1, GROUP_TIMING, 0, 20, 10, 30, 1},
	{"period_refine", N_("Period refinement tolerance (ms)"), 1, GROUP_TIMING, 0, 20, 10, 30, 1},
	{"be_search", N_("Beat-error search tolerance (ms)"), 1, GROUP_TIMING, 0, 20, 10, 30, 1},
	{"tic_smoothing", N_("Tic/toc smoothing (ms)"), 1, GROUP_TIMING, 1, 0.5, 0.1, 1, 0.1},
	{"event_search", N_("Event search tolerance (ms)"), 1, GROUP_TIMING, 0, 20, 10, 30, 1},
	{"trim", N_("Waveform outlier trim (%)"), 1, GROUP_FILTER, 0, 20, 10, 30, 1},
	{"amp_smoothing", N_("Amplitude smoothing (ms)"), 1, GROUP_AMPLITUDE, 1, 1, 0.5, 2, 0.1},
	{"amp_initial", N_("Initial amplitude threshold (%)"), 0, GROUP_AMPLITUDE, 1, 1, 0.1, 5, 0.1},
	{"amp_noise", N_("Amplitude background multiplier"), 1, GROUP_AMPLITUDE, 1, 1.4, 1.2, 2, 0.1},
	{"amp_limit", N_("Maximum amplitude threshold (%)"), 1, GROUP_AMPLITUDE, 0, 20, 10, 30, 1},
	{"amp_step", N_("Amplitude threshold multiplier"), 1, GROUP_AMPLITUDE, 1, 1.4, 1.2, 2, 0.1},
	{"amp_min", N_("Minimum amplitude (degrees)"), 0, GROUP_AMPLITUDE, 0, 135, 90, 135, 1},
	{"amp_difference", N_("Maximum tic/toc difference (degrees)"), 0, GROUP_AMPLITUDE, 0, 60, 60, 100, 1},
	{"period_limit", N_("Maximum period multiplier"), 0, GROUP_TIMING, 2, 1.2, 1.1, 1.3, 0.01},
	{"period_sigma", N_("Period instability fraction"), 0, GROUP_TIMING, 5, 0.0001, 0.00005, 0.00015, 0.00001},
	{"cal_count", N_("Calibration observations"), 0, GROUP_CALIBRATION, 0, 900, 300, 3600, 10},
	{"cal_edge", N_("Calibration edge threshold (%)"), 0, GROUP_CALIBRATION, 0, 5, 3, 10, 1},
	{"cal_interval", N_("Calibration observation interval (s)"), 0, GROUP_CALIBRATION, 1, 0.9, 0.5, 2, 0.1},
	{"cal_precision", N_("Calibration uncertainty (s/day)"), 0, GROUP_CALIBRATION, 1, 0.1, 0.1, 0.5, 0.1},
	{"cal_lock_low", N_("Calibration lock start (%)"), 1, GROUP_CALIBRATION, 0, 40, 35, 45, 1},
	{"cal_lock_high", N_("Calibration lock end (%)"), 1, GROUP_CALIBRATION, 0, 60, 55, 65, 1}
};

double algorithm_parameters[ALG_COUNT];

static int noise_suppression_parameter(int i)
{
	return i == ALG_NOISE_WINDOW || i == ALG_NOISE_SEGMENT || i == ALG_NOISE_THRESHOLD;
}

int algorithm_settings_custom(int light)
{
	for(int i = 0; i < ALG_COUNT; i++) {
		if(parameters[i].group == GROUP_CALIBRATION ||
				(light && noise_suppression_parameter(i)))
			continue;
		if(algorithm_parameters[i] != parameters[i].initial)
			return 1;
	}
	return 0;
}

void algorithm_settings_load(struct main_window *w)
{
	for(int i = 0; i < ALG_COUNT; i++) {
		const struct parameter_info *p = &parameters[i];
		algorithm_parameters[i] = p->initial;
		if(!g_key_file_has_key(w->config_file, "algorithm", p->key, NULL))
			continue;
		GError *err = NULL;
		double value = g_key_file_get_double(w->config_file, "algorithm", p->key, &err);
		if(err || !isfinite(value) || value < p->min || value > p->max ||
				(p->digits == 0 && value != floor(value))) {
			error(_("Invalid algorithm setting: %s. Using its default."), p->key);
			g_clear_error(&err);
		} else
			algorithm_parameters[i] = value;
	}
}

static int save_algorithm_settings(struct main_window *w, GtkWidget **spins, int light)
{
	char *old[ALG_COUNT];
	char *old_light = g_key_file_get_value(w->config_file, "tg", "light_algorithm", NULL);
	int changed = light != w->is_light;
	for(int i = 0; i < ALG_COUNT; i++) {
		gtk_spin_button_update(GTK_SPIN_BUTTON(spins[i]));
		double value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spins[i]));
		if(value != algorithm_parameters[i]) changed = 1;
		old[i] = g_key_file_get_value(w->config_file, "algorithm", parameters[i].key, NULL);
		g_key_file_set_double(w->config_file, "algorithm", parameters[i].key, value);
	}
	g_key_file_set_integer(w->config_file, "tg", "light_algorithm", light);
	gsize length;
	char *data = g_key_file_to_data(w->config_file, &length, NULL);
	GError *err = NULL;
	gboolean ok = g_file_set_contents(w->config_file_name, data, length, &err);
	g_free(data);
	if(!ok) {
		for(int i = 0; i < ALG_COUNT; i++) {
			if(old[i])
				g_key_file_set_value(w->config_file, "algorithm", parameters[i].key, old[i]);
			else
				g_key_file_remove_key(w->config_file, "algorithm", parameters[i].key, NULL);
		}
		if(old_light)
			g_key_file_set_value(w->config_file, "tg", "light_algorithm", old_light);
		else
			g_key_file_remove_key(w->config_file, "tg", "light_algorithm", NULL);
		error(_("Could not save algorithm settings: %s"), err->message);
		g_error_free(err);
	} else if(changed)
		request_algorithm_restart(w, light);
	g_free(old_light);
	for(int i = 0; i < ALG_COUNT; i++) g_free(old[i]);
	return ok;
}

static void set_parameter_highlight(GtkWidget *label, int modified)
{
	GtkStyleContext *style = gtk_widget_get_style_context(label);
	if(modified)
		gtk_style_context_add_class(style, "algorithm-modified");
	else
		gtk_style_context_remove_class(style, "algorithm-modified");
}

static void update_parameter_highlight(GtkSpinButton *spin, GtkWidget *label)
{
	int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(label), "algorithm-parameter")) - 1;
	const struct parameter_info *p = &parameters[index];
	set_parameter_highlight(label,
		fabs(gtk_spin_button_get_value(spin) - p->initial) > p->step * 1e-6);
}

static void update_light_highlight(GtkToggleButton *light, GtkWidget *label)
{
	set_parameter_highlight(label, gtk_toggle_button_get_active(light));
}

void algorithm_settings_open(struct main_window *w)
{
	static const char *groups[] = {
		N_("Filtering and noise"), N_("Timing and detection"),
		N_("Amplitude"), N_("Calibration")
	};
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Algorithm configuration"),
		GTK_WINDOW(w->window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		_("Cancel"), GTK_RESPONSE_CANCEL, _("Reset to defaults"), RESPONSE_RESET,
		_("Apply"), GTK_RESPONSE_APPLY,
		_("Save"), GTK_RESPONSE_OK, NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 640, 600);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	GtkWidget *warning = gtk_label_new(_("Warning: changing Advanced or Expert parameters can produce incorrect measurements. Applying changes restarts the measurement and clears the live Trend."));
	gtk_label_set_line_wrap(GTK_LABEL(warning), TRUE);
	gtk_box_pack_start(GTK_BOX(content), warning, FALSE, FALSE, 8);
	GtkWidget *tabs = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(content), tabs, TRUE, TRUE, 0);
	GtkWidget *legend = gtk_label_new(_("Highlighted parameters differ from their defaults."));
	gtk_box_pack_start(GTK_BOX(content), legend, FALSE, FALSE, 8);
	GtkCssProvider *highlight = gtk_css_provider_new();
	gtk_css_provider_load_from_data(highlight,
		".algorithm-modified { color: #4a2d00; background-color: #ffe2a1; font-weight: bold; }",
		-1, NULL);
	GtkWidget *basic = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_container_set_border_width(GTK_CONTAINER(basic), 8);
	GtkWidget *light = gtk_check_button_new_with_label(_("Light algorithm"));
	GtkWidget *light_label = gtk_bin_get_child(GTK_BIN(light));
	gtk_style_context_add_provider(gtk_widget_get_style_context(light_label),
		GTK_STYLE_PROVIDER(highlight), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(light), w->is_light);
	gtk_widget_set_tooltip_text(light, _("Default: Full"));
	g_signal_connect_object(light, "toggled", G_CALLBACK(update_light_highlight), light_label, 0);
	update_light_highlight(GTK_TOGGLE_BUTTON(light), light_label);
	gtk_box_pack_start(GTK_BOX(basic), light, FALSE, FALSE, 0);
	GtkWidget *description = gtk_label_new(_("Light reduces CPU load by processing half the audio samples, using shorter analysis windows and skipping noise suppression. Results update faster but are less stable and more sensitive to noise."));
	gtk_label_set_line_wrap(GTK_LABEL(description), TRUE);
	gtk_label_set_xalign(GTK_LABEL(description), 0);
	gtk_box_pack_start(GTK_BOX(basic), description, FALSE, FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(tabs), basic, gtk_label_new(_("Basic")));
	GtkWidget *spins[ALG_COUNT];
	for(int level = 0; level < 2; level++) {
		GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
		gtk_container_set_border_width(GTK_CONTAINER(box), 8);
		gtk_container_add(GTK_CONTAINER(scroll), box);
		gtk_notebook_append_page(GTK_NOTEBOOK(tabs), scroll,
			gtk_label_new(level ? _("Expert") : _("Advanced")));
		for(int group = 0; group < GROUP_COUNT; group++) {
			GtkWidget *grid = gtk_grid_new();
			gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
			gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
			int row = 0;
			for(int i = 0; i < ALG_COUNT; i++) {
				const struct parameter_info *p = &parameters[i];
				if(p->expert != level || p->group != group) continue;
				char *text = noise_suppression_parameter(i) ?
					g_strdup_printf(_("%s (Full only)"), _(p->label)) : g_strdup(_(p->label));
				GtkWidget *label = gtk_label_new(text);
				g_free(text);
				gtk_widget_set_halign(label, GTK_ALIGN_START);
				gtk_style_context_add_provider(gtk_widget_get_style_context(label),
					GTK_STYLE_PROVIDER(highlight), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
				spins[i] = gtk_spin_button_new_with_range(p->min, p->max, p->step);
				gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spins[i]), p->digits);
				gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spins[i]), TRUE);
				GError *err = NULL;
				double value = g_key_file_get_double(w->config_file, "algorithm", p->key, &err);
				if(err || !isfinite(value) || value < p->min || value > p->max) {
					g_clear_error(&err);
					value = algorithm_parameters[i];
				}
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(spins[i]), value);
				char *tooltip = g_strdup_printf(_("Default: %.*f"), p->digits, p->initial);
				gtk_widget_set_tooltip_text(label, tooltip);
				gtk_widget_set_tooltip_text(spins[i], tooltip);
				g_free(tooltip);
				g_object_set_data(G_OBJECT(label), "algorithm-parameter", GINT_TO_POINTER(i + 1));
				g_signal_connect_object(spins[i], "value-changed",
					G_CALLBACK(update_parameter_highlight), label, 0);
				update_parameter_highlight(GTK_SPIN_BUTTON(spins[i]), label);
				gtk_grid_attach(GTK_GRID(grid), spins[i], 1, row++, 1, 1);
			}
			if(row) {
				GtkWidget *frame = gtk_frame_new(_(groups[group]));
				gtk_container_add(GTK_CONTAINER(frame), grid);
				gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
			} else
				gtk_widget_destroy(grid);
		}
	}
	g_object_unref(highlight);
	gtk_widget_show_all(dialog);
	for(;;) {
		int response = gtk_dialog_run(GTK_DIALOG(dialog));
		if(response == RESPONSE_RESET) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(light), FALSE);
			for(int i = 0; i < ALG_COUNT; i++)
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(spins[i]), parameters[i].initial);
		} else if(response == GTK_RESPONSE_APPLY || response == GTK_RESPONSE_OK) {
			if(save_algorithm_settings(w, spins,
					gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(light))) &&
					response == GTK_RESPONSE_OK) break;
		} else
			break;
	}
	gtk_widget_destroy(dialog);
}
