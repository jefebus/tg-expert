/*
    tg-expert
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include "tg.h"

enum { GROUP_FILTER, GROUP_NOISE, GROUP_PERIOD, GROUP_AUTO, GROUP_BEAT,
	GROUP_FOLD, GROUP_TRACE, GROUP_AMPLITUDE, GROUP_CALIBRATION, GROUP_COUNT };
enum { RESPONSE_RESET = 1 };

struct parameter_info {
	const char *key, *label;
	int expert, group, digits;
	double initial, min, max, step;
};

static const struct parameter_info parameters[ALG_COUNT] = {
	{"filter", N_("Audio and envelope cutoff (Hz)"), 0, GROUP_FILTER, 0, 3000, 500, 5000, 100},
	{"noise_window", N_("Noise energy window (ms)"), 1, GROUP_NOISE, 0, 20, 10, 30, 1},
	{"noise_segment", N_("Noise reference segment length (ms)"), 1, GROUP_NOISE, 0, 500, 300, 700, 10},
	{"noise_threshold", N_("Noise rejection threshold (× reference)"), 0, GROUP_NOISE, 1, 2, 1, 3, 0.1},
	{"taper", N_("FFT edge taper (ms)"), 1, GROUP_FILTER, 0, 100, 50, 150, 5},
	{"lobes", N_("Maximum peak lobes"), 1, GROUP_PERIOD, 0, 20, 10, 30, 1},
	{"guess_search", N_("Period search half-width (± ms)"), 0, GROUP_PERIOD, 0, 20, 10, 30, 1},
	{"harmonic", N_("Minimum shorter-period peak (%)"), 1, GROUP_AUTO, 0, 90, 80, 100, 1},
	{"double_trigger", N_("Double-trigger check threshold (%)"), 1, GROUP_AUTO, 0, 20, 10, 30, 1},
	{"period_refine", N_("Period refinement half-width (± ms)"), 1, GROUP_PERIOD, 0, 20, 10, 30, 1},
	{"be_search", N_("Opposite-beat search half-width (± ms)"), 0, GROUP_BEAT, 0, 20, 10, 30, 1},
	{"tic_smoothing", N_("Tic/toc alignment smoothing (ms)"), 1, GROUP_BEAT, 1, 0.5, 0.1, 1, 0.1},
	{"event_search", N_("Trace event search half-width (± ms)"), 1, GROUP_TRACE, 0, 20, 10, 30, 1},
	{"trim", N_("High samples discarded per phase bin (%)"), 1, GROUP_FOLD, 0, 20, 10, 30, 1},
	{"amp_smoothing", N_("Lift-pulse smoothing (ms)"), 1, GROUP_AMPLITUDE, 1, 1, 0.5, 2, 0.1},
	/* xgettext:no-c-format */
	{"amp_initial", N_("Starting lift-pulse threshold (% of peak)"), 1, GROUP_AMPLITUDE, 1, 1, 0.1, 5, 0.1},
	{"amp_noise", N_("Lift-pulse background multiplier (×)"), 1, GROUP_AMPLITUDE, 1, 1.4, 1.2, 2, 0.1},
	/* xgettext:no-c-format */
	{"amp_limit", N_("Lift-pulse threshold ceiling (% of peak)"), 1, GROUP_AMPLITUDE, 0, 20, 10, 30, 1},
	{"amp_step", N_("Amplitude threshold multiplier"), 1, GROUP_AMPLITUDE, 1, 1.4, 1.2, 2, 0.1},
	{"amp_min", N_("Minimum accepted balance amplitude (°)"), 0, GROUP_AMPLITUDE, 0, 135, 90, 135, 1},
	{"amp_difference", N_("Maximum tic/toc amplitude difference (°)"), 1, GROUP_AMPLITUDE, 0, 60, 60, 100, 1},
	{"period_limit", N_("Maximum period multiplier"), 0, GROUP_PERIOD, 2, 1.2, 1.1, 1.3, 0.01},
	{"period_sigma", N_("Period instability fraction"), 0, GROUP_PERIOD, 5, 0.0001, 0.00005, 0.00015, 0.00001},
	{"cal_count", N_("Observation count"), 0, GROUP_CALIBRATION, 0, 900, 300, 3600, 10},
	{"cal_edge", N_("Calibration edge threshold (%)"), 0, GROUP_CALIBRATION, 0, 5, 3, 10, 1},
	{"cal_interval", N_("Minimum observation interval (s)"), 0, GROUP_CALIBRATION, 1, 0.9, 0.5, 2, 0.1},
	{"cal_precision", N_("Fit acceptance precision (s/day)"), 0, GROUP_CALIBRATION, 1, 0.1, 0.1, 0.5, 0.1},
	{"cal_lock_low", N_("Lower position in folded second (%)"), 1, GROUP_CALIBRATION, 0, 40, 35, 45, 1},
	{"cal_lock_high", N_("Upper position in folded second (%)"), 1, GROUP_CALIBRATION, 0, 60, 55, 65, 1}
};

double algorithm_parameters[ALG_COUNT];

static int editable_parameter(int i)
{
	return i != ALG_TAPER && i != ALG_LOBES &&
		i != ALG_AMP_STEP && i != ALG_PERIOD_LIMIT && i != ALG_PERIOD_SIGMA;
}

static const char *parameter_help(int i)
{
	switch(i) {
	case ALG_FILTER: return _("Sets the cutoff for both the raw-audio high-pass and the tick-envelope low-pass. Raising it rejects more low-frequency vibration but preserves finer detail in the envelope.");
	case ALG_NOISE_THRESHOLD: return _("Mutes audio where the windowed energy exceeds this multiple of the reference level. Higher values mute less; use a higher value if strong, genuine ticks are being removed. Full mode only.");
	case ALG_NOISE_WINDOW: return _("Sets how much audio is combined into each local energy measurement. A longer window spreads the influence of a short impact over more time. Full mode only.");
	case ALG_NOISE_SEGMENT: return _("Takes the highest local energy in each segment, then uses the median of those peaks as the reference. Longer segments mean fewer reference measurements per analysis block. Full mode only.");
	case ALG_GUESS_SEARCH: return _("With manual BPH, searches this far on either side of the expected tic-to-tic interval. Widen it if a clock gains or loses enough to put the true period outside the window. This setting also affects automatic BPH checks.");
	case ALG_PERIOD_REFINE: return _("After finding the initial period, searches this far around each predicted correlation peak. Widen only if the correct peak falls outside the window; a wider search can pick a neighboring peak.");
	case ALG_HARMONIC: return _("A shorter candidate period must exceed this percentage of the original correlation peak to be accepted. Higher values make Auto less willing to choose the shorter period.");
	case ALG_DOUBLE_TRIGGER: return _("Compares the correlation near 1.5 times the candidate period with the peak at the candidate period. A value below this threshold triggers a check for a doubled period. Raising the threshold makes that check more likely.");
	case ALG_BE_SEARCH: return _("Looks for the opposite beat near halfway through the measured cycle. Widen it if tic and toc are genuinely uneven; a steady gain or loss is not a reason to widen it.");
	case ALG_TIC_SMOOTH: return _("Smooths the combined tic/toc envelope before choosing its timing reference. More smoothing reduces fine detail but can shift the point used for alignment.");
	case ALG_TRIM: return _("When combining repeated cycles into one waveform, discards the highest values at each phase position before averaging. Higher values reject more isolated spikes, but unusual loud beats contribute less to the result.");
	case ALG_EVENT_SEARCH: return _("Searches this far around each predicted tick for a trace dot. A wider window may recover missing dots or pick another peak from the same tick. It does not widen the initial rate search.");
	case ALG_AMP_SMOOTH: return _("Smooths the waveform before looking for lift pulses. More smoothing can reduce ripples but can also merge nearby acoustic features.");
	case ALG_AMP_INITIAL: return _("Sets the starting signal threshold relative to the strongest point in the smoothed waveform. The background-based threshold takes precedence if it is higher.");
	case ALG_AMP_NOISE: return _("Sets a threshold from the estimated background level. Raising it makes weak features less likely to be accepted as lift pulses.");
	case ALG_AMP_LIMIT: return _("Stops the search when the trial signal threshold reaches this percentage of the waveform peak. Raising it allows the detector to try higher thresholds; it does not raise the maximum balance amplitude.");
	case ALG_AMP_MIN: return _("Rejects a result if either the tic or toc amplitude estimate does not exceed this value. Lowering it can admit a weaker swing, but it does not improve pulse detection.");
	case ALG_AMP_DIFFERENCE: return _("Accepts a pulse pair only if the two inferred amplitudes differ by less than this value. Raising it permits more disagreement; it does not make the estimates more accurate.");
	case ALG_CAL_COUNT: return _("Number of accepted reference-pulse observations used for the calibration fit.");
	case ALG_CAL_EDGE: return _("Rising-edge threshold as a percentage of the difference between the background and the folded pulse peak.");
	case ALG_CAL_INTERVAL: return _("Minimum time in seconds between accepted calibration observations.");
	case ALG_CAL_PRECISION: return _("Accepts the calibration fit only when its estimated uncertainty is below this value in seconds per day.");
	case ALG_CAL_LOCK_LOW: return _("Lower position within the folded second for the reference pulse peak and rising edge.");
	case ALG_CAL_LOCK_HIGH: return _("Upper position within the folded second for the reference pulse peak.");
	default: return "";
	}
}

static int noise_suppression_parameter(int i)
{
	return i == ALG_NOISE_WINDOW || i == ALG_NOISE_SEGMENT || i == ALG_NOISE_THRESHOLD;
}

static int sample_windows_valid(const double *values)
{
	static const int rates[] = SAMPLE_RATES;
	for(int i = 0; rates[i]; i++) {
		int rate = rates[i];
		int window = (int)(rate * values[ALG_NOISE_WINDOW] / 1000);
		int segment = (int)(rate * values[ALG_NOISE_SEGMENT] / 1000);
		int shortest_block = rate * (1 << FIRST_STEP);
		if(window < 1 || segment < 1 || window + segment > shortest_block + 1)
			return 0;
		/* Light mode has half the sample rate; use the shortest nominal cycle. */
		rate /= 2;
		int period = (int)floor(7200.0 * rate / MAX_BPH);
		int beat_search = (int)(rate * values[ALG_BE_SEARCH] / 1000);
		int tic_window = MAX(1, (int)(rate * values[ALG_TIC_SMOOTH] / 1000));
		int amp_window = MAX(1, (int)(rate * values[ALG_AMP_SMOOTH] / 1000));
		int cal_low = (int)(rate * values[ALG_CAL_LOCK_LOW] / 100);
		int cal_high = (int)(rate * values[ALG_CAL_LOCK_HIGH] / 100);
		if(period / 2 - beat_search <= 0 ||
				period - (period / 2 + beat_search) <= tic_window ||
				amp_window >= period || cal_low <= rate / 4 ||
				cal_high >= rate * 3 / 4 || cal_low >= cal_high)
			return 0;
	}
	return 1;
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
	if(!sample_windows_valid(algorithm_parameters)) {
		error(_("Algorithm window settings are incompatible with supported sample rates. Using defaults for the affected windows."));
		int indices[] = { ALG_NOISE_WINDOW, ALG_NOISE_SEGMENT, ALG_BE_SEARCH,
			ALG_TIC_SMOOTH, ALG_AMP_SMOOTH, ALG_CAL_LOCK_LOW, ALG_CAL_LOCK_HIGH };
		for(unsigned i = 0; i < G_N_ELEMENTS(indices); i++)
			algorithm_parameters[indices[i]] = parameters[indices[i]].initial;
	}
}

void algorithm_settings_prune_defaults(GKeyFile *config)
{
	for(int i = 0; i < ALG_COUNT; i++) {
		GError *err = NULL;
		double value = g_key_file_get_double(config, "algorithm", parameters[i].key, &err);
		if(!err && isfinite(value) && value == parameters[i].initial)
			g_key_file_remove_key(config, "algorithm", parameters[i].key, NULL);
		g_clear_error(&err);
	}
}

static void set_algorithm_values(GKeyFile *config, const double *values, const gboolean *calibration_edits)
{
	for(int i = 0; i < ALG_COUNT; i++) {
		if(!editable_parameter(i) || (i >= ALG_CAL_COUNT && !calibration_edits[i])) continue;
		if(values[i] == parameters[i].initial)
			g_key_file_remove_key(config, "algorithm", parameters[i].key, NULL);
		else
			g_key_file_set_double(config, "algorithm", parameters[i].key, values[i]);
	}
	algorithm_settings_prune_defaults(config);
}

static int save_algorithm_settings(struct main_window *w, GtkWidget **spins, int light)
{
	int changed = light != w->is_light;
	double values[ALG_COUNT];
	gboolean calibration_edits[ALG_COUNT] = {0};
	memcpy(values, algorithm_parameters, sizeof(values));
	for(int i = 0; i < ALG_COUNT; i++) {
		if(!spins[i]) continue;
		if(i >= ALG_CAL_COUNT)
			calibration_edits[i] = g_object_get_data(G_OBJECT(spins[i]), "calibration-edited") != NULL;
		if(i < ALG_CAL_COUNT || calibration_edits[i])
			gtk_spin_button_update(GTK_SPIN_BUTTON(spins[i]));
		if(i < ALG_CAL_COUNT || calibration_edits[i])
			values[i] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spins[i]));
	}
	if(!sample_windows_valid(values)) {
		error(_("Algorithm window settings are incompatible with supported sample rates."));
		return 0;
	}
	for(int i = 0; i < ALG_COUNT; i++) {
		if(spins[i] && values[i] != algorithm_parameters[i]) changed = 1;
	}
	gsize length;
	char *data = g_key_file_to_data(w->config_file, &length, NULL);
	GError *err = NULL;
	GKeyFile *staged = g_key_file_new();
	gboolean ok = g_key_file_load_from_data(staged, data, length,
		G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &err);
	g_free(data);
	if(!ok) {
		error(_("Could not save algorithm settings: %s"), err->message);
		g_error_free(err);
		g_key_file_free(staged);
		return 0;
	}
	set_algorithm_values(staged, values, calibration_edits);
	g_key_file_set_integer(staged, "tg", "light_algorithm", light);
	data = g_key_file_to_data(staged, &length, NULL);
	ok = g_file_set_contents(w->config_file_name, data, length, &err);
	g_free(data);
	if(!ok) {
		error(_("Could not save algorithm settings: %s"), err->message);
		g_error_free(err);
		g_key_file_free(staged);
	} else {
		g_key_file_free(w->config_file);
		w->config_file = staged;
		if(changed) request_algorithm_restart(w, light);
	}
	return ok;
}

static void set_expert_visibility(GtkToggleButton *button, GPtrArray *items)
{
	for(guint i = 0; i < items->len; i++)
		gtk_widget_set_visible(g_ptr_array_index(items, i), gtk_toggle_button_get_active(button));
}

static void set_light_notice(GtkToggleButton *button, GtkWidget *notice)
{
	gtk_widget_set_visible(notice, gtk_toggle_button_get_active(button));
}

static void set_light_parameter_notice(GtkToggleButton *button, GtkWidget *notice)
{
	gtk_label_set_text(GTK_LABEL(notice), gtk_toggle_button_get_active(button) ?
		_("Not used in Light mode") : _("Full mode only"));
}

static void set_light_parameter_sensitive(GtkToggleButton *button, GtkWidget *line)
{
	gtk_widget_set_sensitive(line, !gtk_toggle_button_get_active(button));
}

static void set_calibration_editable(GtkToggleButton *button, GtkWidget **spins)
{
	for(int i = ALG_CAL_COUNT; i < ALG_COUNT; i++)
		gtk_widget_set_sensitive(spins[i], gtk_toggle_button_get_active(button));
}

static void mark_calibration_edited(GtkEditable *editable, gpointer data)
{
	UNUSED(data);
	g_object_set_data(G_OBJECT(editable), "calibration-edited", GINT_TO_POINTER(1));
}

static GtkWidget *help_label(const char *text)
{
	GtkWidget *label = gtk_label_new(text);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_max_width_chars(GTK_LABEL(label), 65);
	gtk_widget_set_hexpand(label, TRUE);
	gtk_style_context_add_class(gtk_widget_get_style_context(label), GTK_STYLE_CLASS_DIM_LABEL);
	return label;
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
		N_("Signal conditioning"), N_("Noise suppression"), N_("Period detection"),
		N_("Automatic BPH"), N_("Beat separation"), N_("Waveform folding"),
		N_("Trace tracking"), N_("Amplitude detection"), N_("Calibration service details")
	};
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Algorithm configuration"),
		GTK_WINDOW(w->window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		_("Cancel"), GTK_RESPONSE_CANCEL, _("Reset to defaults"), RESPONSE_RESET,
		_("Apply"), GTK_RESPONSE_APPLY,
		_("Save"), GTK_RESPONSE_OK, NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 780, 760);
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
		".algorithm-modified:not(:disabled) { color: #4a2d00; background-color: #ffe2a1; font-weight: bold; }",
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
	GtkWidget *light_help = gtk_expander_new(_("Help"));
	char *light_help_name = g_strdup_printf(_("Help for %s"), _("Light algorithm"));
	atk_object_set_name(gtk_widget_get_accessible(light_help), light_help_name);
	g_free(light_help_name);
	char *light_description = g_strdup_printf("%s\n%s",
		_("Light reduces CPU load by processing half the audio samples, using shorter analysis windows and skipping noise suppression. Results update faster but are less stable and more sensitive to noise."),
		_("Default: Full"));
	gtk_container_add(GTK_CONTAINER(light_help), help_label(light_description));
	g_free(light_description);
	gtk_box_pack_start(GTK_BOX(basic), light_help, FALSE, FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(tabs), basic, gtk_label_new(_("Basic")));
	GtkWidget *spins[ALG_COUNT] = {0};
	GPtrArray *expert_items = g_ptr_array_new();
	g_object_set_data_full(G_OBJECT(dialog), "expert-items", expert_items, (GDestroyNotify)g_ptr_array_unref);
	GtkWidget *expert = NULL, *calibration_expert = NULL, *light_notice = NULL;
	for(int level = 0; level < 2; level++) {
		GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
		gtk_container_set_border_width(GTK_CONTAINER(box), 8);
		gtk_container_add(GTK_CONTAINER(scroll), box);
		gtk_notebook_append_page(GTK_NOTEBOOK(tabs), scroll,
			gtk_label_new(level ? _("Calibration") : _("Advanced")));
		if(!level) {
			expert = gtk_check_button_new_with_label(_("Show Expert settings"));
			gtk_box_pack_start(GTK_BOX(box), expert, FALSE, FALSE, 0);
		} else {
			calibration_expert = gtk_check_button_new_with_label(_("Edit calibration settings (Expert)"));
			gtk_box_pack_start(GTK_BOX(box), calibration_expert, FALSE, FALSE, 0);
		}
		for(int group = 0; group < GROUP_COUNT; group++) {
			if((group == GROUP_CALIBRATION) != level) continue;
			GtkWidget *group_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
			gtk_container_set_border_width(GTK_CONTAINER(group_box), 8);
			GtkWidget *expert_rows[ALG_COUNT];
			int n_expert = 0, n_advanced = 0;
			if(group == GROUP_NOISE) {
				light_notice = help_label(_("When Light mode is applied, these settings are inactive; their values are retained."));
				gtk_box_pack_start(GTK_BOX(group_box), light_notice, FALSE, FALSE, 0);
				g_signal_connect_object(light, "toggled", G_CALLBACK(set_light_notice), light_notice, 0);
			}
			for(int expert_level = 0; expert_level < (level ? 1 : 2); expert_level++) {
				for(int i = 0; i < ALG_COUNT; i++) {
					const struct parameter_info *p = &parameters[i];
					if(p->group != group || (level ? i < ALG_CAL_COUNT :
							(!editable_parameter(i) || p->expert != expert_level))) continue;
					GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
					GtkWidget *line = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
					GtkWidget *label = gtk_label_new(_(p->label));
					gtk_label_set_xalign(GTK_LABEL(label), 0);
					gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
					gtk_label_set_max_width_chars(GTK_LABEL(label), 38);
					gtk_widget_set_hexpand(label, TRUE);
					gtk_style_context_add_provider(gtk_widget_get_style_context(label),
						GTK_STYLE_PROVIDER(highlight), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
					gtk_box_pack_start(GTK_BOX(line), label, TRUE, TRUE, 0);
					gtk_box_pack_start(GTK_BOX(row), line, FALSE, FALSE, 0);
					{
						spins[i] = gtk_spin_button_new_with_range(p->min, p->max, p->step);
						gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spins[i]), p->digits);
						gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spins[i]), TRUE);
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(spins[i]), algorithm_parameters[i]);
						if(level)
							g_signal_connect(spins[i], "changed", G_CALLBACK(mark_calibration_edited), NULL);
						gtk_label_set_mnemonic_widget(GTK_LABEL(label), spins[i]);
						gtk_box_pack_end(GTK_BOX(line), spins[i], FALSE, FALSE, 0);
						if(noise_suppression_parameter(i)) {
							set_light_parameter_sensitive(GTK_TOGGLE_BUTTON(light), line);
							g_signal_connect_object(light, "toggled",
								G_CALLBACK(set_light_parameter_sensitive), line, 0);
							GtkWidget *notice = help_label("");
							set_light_parameter_notice(GTK_TOGGLE_BUTTON(light), notice);
							g_signal_connect_object(light, "toggled",
								G_CALLBACK(set_light_parameter_notice), notice, 0);
							gtk_box_pack_start(GTK_BOX(row), notice, FALSE, FALSE, 0);
						}
						char *text = g_strdup_printf(_("%s\nDefault: %.*f"),
							parameter_help(i), p->digits, p->initial);
						gtk_widget_set_tooltip_text(spins[i], text);
						GtkWidget *hint = help_label(text);
						g_free(text);
						GtkWidget *details = gtk_expander_new(_("Help"));
						char *name = g_strdup_printf(_("Help for %s"), _(p->label));
						atk_object_set_name(gtk_widget_get_accessible(details), name);
						g_free(name);
						gtk_container_add(GTK_CONTAINER(details), hint);
						gtk_box_pack_start(GTK_BOX(row), details, FALSE, FALSE, 0);
						if(!level && p->expert)
							expert_rows[n_expert++] = row;
						else if(!level)
							n_advanced++;
						g_object_set_data(G_OBJECT(label), "algorithm-parameter", GINT_TO_POINTER(i + 1));
						g_signal_connect_object(spins[i], "value-changed",
							G_CALLBACK(update_parameter_highlight), label, 0);
						update_parameter_highlight(GTK_SPIN_BUTTON(spins[i]), label);
					}
					gtk_box_pack_start(GTK_BOX(group_box), row, FALSE, FALSE, 0);
				}
			}
			GtkWidget *frame = gtk_expander_new(_(groups[group]));
			gtk_expander_set_expanded(GTK_EXPANDER(frame), TRUE);
			gtk_container_add(GTK_CONTAINER(frame), group_box);
			gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
			if(!level && !n_advanced)
				g_ptr_array_add(expert_items, frame);
			else
				for(int i = 0; i < n_expert; i++) g_ptr_array_add(expert_items, expert_rows[i]);
		}
	}
	g_object_unref(highlight);
	gtk_widget_show_all(dialog);
	for(int i = ALG_CAL_COUNT; i < ALG_COUNT; i++)
		g_object_set_data(G_OBJECT(spins[i]), "calibration-edited", NULL);
	g_signal_connect(expert, "toggled", G_CALLBACK(set_expert_visibility), expert_items);
	set_expert_visibility(GTK_TOGGLE_BUTTON(expert), expert_items);
	set_light_notice(GTK_TOGGLE_BUTTON(light), light_notice);
	g_signal_connect(calibration_expert, "toggled", G_CALLBACK(set_calibration_editable), spins);
	set_calibration_editable(GTK_TOGGLE_BUTTON(calibration_expert), spins);
	for(;;) {
		int response = gtk_dialog_run(GTK_DIALOG(dialog));
		if(response == RESPONSE_RESET) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(light), FALSE);
			for(int i = 0; i < ALG_COUNT; i++)
				if(spins[i] && (i < ALG_CAL_COUNT ||
						gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(calibration_expert))))
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
