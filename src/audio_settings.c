/*
    tg
    Copyright (C) 2015 Marcello Mamino

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* Audio settings dialog: audio system, input device and sample rate.
 *
 * The dialog is not modal, so the paperstrip and the output panels keep
 * updating behind it. Every change is applied at once (audio restarts),
 * and the choices below the changed one go back to a default:
 *   audio system -> its default input, default rate
 *   input device -> the current rate if the device has it, else the default
 * Only the rates the selected device accepts are offered. */

#include "tg.h"

#define LEVEL_FLOOR_DB -60.0	// the level bar spans LEVEL_FLOOR_DB to 0 dBFS
#define LEVEL_UPDATE_MS 100

struct audio_settings {
	struct main_window *w;
	GtkWidget *dialog;
	GtkWidget *api_combo;
	GtkWidget *device_combo;
	GtkWidget *rate_combo;
	GtkWidget *level_bar;
	GtkWidget *level_label;
	GtkWidget *status_label;
	guint level_timeout;
	int updating;	// set while the combos are filled programmatically
};

static int audio_change_allowed(struct main_window *w)
{
	return w->controls_active && !w->calibrate && !w->restart_audio;
}

static gchar *rate_label(int rate)
{
	return g_strdup_printf("%g kHz", rate / 1000.0);
}

/* The rate to use with a device: preferred if the device accepts it,
 * else the default rate, else the lowest rate it accepts */
static int pick_rate(int device, int preferred)
{
	int rates[16];
	int n = list_supported_sample_rates(device, rates, 16);
	if(!n) return preferred;
	int has_default = 0;
	for(int i = 0; i < n; i++) {
		if(rates[i] == preferred) return preferred;
		if(rates[i] == (int)PA_SAMPLE_RATE) has_default = 1;
	}
	return has_default ? (int)PA_SAMPLE_RATE : rates[0];
}

static int combo_active_int(GtkWidget *combo, int *value)
{
	const gchar *id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo));
	if(!id) return 0;
	char *end = NULL;
	long v = strtol(id, &end, 10);
	if(!end || *end) return 0;
	*value = (int)v;
	return 1;
}

static void combo_append_int(GtkWidget *combo, int id, const char *text)
{
	char s[32];
	snprintf(s, sizeof(s), "%d", id);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), s, text);
}

static void combo_set_active_int(GtkWidget *combo, int id)
{
	char s[32];
	snprintf(s, sizeof(s), "%d", id);
	if(!gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), s))
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), -1);
}

/* Text for the top bar button and the dialog: device, audio system, rate */
static gchar *audio_summary(struct main_window *w)
{
	if(w->restart_audio)
		return g_strdup("Opening audio input…");
	const char *device = get_audio_input_device_name();
	if(!w->audio_ok || !device)
		return g_strdup("No audio input");
	const char *api = get_audio_api_name();
	gchar *rate = rate_label(w->nominal_sr);
	gchar *s = g_strdup_printf("%s · %s · %s", device, api ? api : "?", rate);
	g_free(rate);
	return s;
}

static void fill_api_combo(struct audio_settings *as)
{
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(as->api_combo));
	int *types = NULL;
	char **names = NULL;
	int count = 0;
	int current = AUDIO_API_DEFAULT;
	if(!list_audio_apis(&types, &names, &count, &current)) {
		for(int i = 0; i < count; i++)
			combo_append_int(as->api_combo, types[i], names[i]);
		free_audio_apis(types, names, count);
	}
	combo_set_active_int(as->api_combo, current);
}

static void fill_device_combo(struct audio_settings *as)
{
	struct main_window *w = as->w;
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(as->device_combo));
	int *devices = NULL;
	char **names = NULL;
	int count = 0;
	int default_device = AUDIO_DEVICE_DEFAULT;
	if(!list_audio_input_devices(&devices, &names, &count, &default_device)) {
		for(int i = 0; i < count; i++) {
			gchar *text = g_strconcat(names[i], devices[i] == default_device ? " (default)" : "", NULL);
			combo_append_int(as->device_combo, devices[i], text);
			g_free(text);
		}
		free_audio_input_devices(devices, names, count);
	}
	/* Nothing is selected when no input is open */
	combo_set_active_int(as->device_combo, w->audio_ok ? w->audio_device : AUDIO_DEVICE_DEFAULT);
}

static void fill_rate_combo(struct audio_settings *as)
{
	struct main_window *w = as->w;
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(as->rate_combo));
	if(!w->audio_ok) return;
	int rates[16];
	int n = list_supported_sample_rates(w->audio_device, rates, 16);
	for(int i = 0; i < n; i++) {
		gchar *text = rate_label(rates[i]);
		combo_append_int(as->rate_combo, rates[i], text);
		g_free(text);
	}
	combo_set_active_int(as->rate_combo, w->nominal_sr);
}

static void update_status(struct audio_settings *as)
{
	struct main_window *w = as->w;
	gchar *summary = audio_summary(w);
	const char *note = NULL;
	if(w->calibrate)
		note = "Audio settings are locked during calibration.";
	else if(!w->controls_active)
		note = "Switch to the real-time tab to change audio settings.";
	else if(!w->restart_audio)
		note = w->audio_message;

	gchar *text = note ? g_strconcat(summary, "\n", note, NULL) : g_strdup(summary);
	gtk_label_set_text(GTK_LABEL(as->status_label), text);
	g_free(text);
	g_free(summary);
}

static void refresh(struct audio_settings *as)
{
	int allowed = audio_change_allowed(as->w);
	as->updating = 1;
	fill_api_combo(as);
	fill_device_combo(as);
	fill_rate_combo(as);
	as->updating = 0;
	gtk_widget_set_sensitive(as->api_combo, allowed);
	gtk_widget_set_sensitive(as->device_combo, allowed);
	gtk_widget_set_sensitive(as->rate_combo, allowed && as->w->audio_ok);
	update_status(as);
}

static void handle_api_changed(GtkComboBox *combo, struct audio_settings *as)
{
	UNUSED(combo);
	struct main_window *w = as->w;
	int type;
	if(as->updating || !audio_change_allowed(w) || !combo_active_int(as->api_combo, &type))
		return;
	if(type == get_audio_api())
		return;
	/* A new audio system starts on its default input at the default rate */
	set_audio_api(type);
	int device = get_default_audio_input_device();
	int rate = device >= 0 ? pick_rate(device, PA_SAMPLE_RATE) : (int)PA_SAMPLE_RATE;
	request_audio_restart(w, type, AUDIO_DEVICE_DEFAULT, rate);
}

static void handle_device_changed(GtkComboBox *combo, struct audio_settings *as)
{
	UNUSED(combo);
	struct main_window *w = as->w;
	int device;
	if(as->updating || !audio_change_allowed(w) || !combo_active_int(as->device_combo, &device))
		return;
	if(w->audio_ok && device == w->audio_device)
		return;
	/* Keep the current rate if the new device has it */
	request_audio_restart(w, w->audio_api, device, pick_rate(device, w->nominal_sr));
}

static void handle_rate_changed(GtkComboBox *combo, struct audio_settings *as)
{
	UNUSED(combo);
	struct main_window *w = as->w;
	int rate;
	if(as->updating || !audio_change_allowed(w) || !combo_active_int(as->rate_combo, &rate))
		return;
	if(rate == w->nominal_sr)
		return;
	request_audio_restart(w, w->audio_api, w->audio_device, rate);
}

/* Input level in dBFS, from the RMS level measured in the audio callback */
static gboolean update_level(struct audio_settings *as)
{
	double fraction = 0;
	gchar *text;
	if(as->w->audio_ok && !as->w->restart_audio) {
		double level = get_audio_level();
		double db = level > 0 ? 20 * log10(level) : LEVEL_FLOOR_DB;
		if(db < LEVEL_FLOOR_DB) db = LEVEL_FLOOR_DB;
		fraction = (db - LEVEL_FLOOR_DB) / -LEVEL_FLOOR_DB;
		text = g_strdup_printf("%.0f dBFS", db);
	} else {
		text = g_strdup("—");
	}
	if(fraction > 1) fraction = 1;
	gtk_level_bar_set_value(GTK_LEVEL_BAR(as->level_bar), fraction);
	gtk_label_set_text(GTK_LABEL(as->level_label), text);
	g_free(text);
	return TRUE;
}

static void handle_destroy(GtkWidget *dialog, struct audio_settings *as)
{
	UNUSED(dialog);
	if(as->level_timeout)
		g_source_remove(as->level_timeout);
	as->w->audio_settings = NULL;
	free(as);
}

static void handle_response(GtkDialog *dialog, gint response, struct audio_settings *as)
{
	UNUSED(response);
	UNUSED(as);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static GtkWidget *grid_label(GtkWidget *grid, const char *text, int row)
{
	GtkWidget *label = gtk_label_new(text);
	gtk_widget_set_halign(label, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
	return label;
}

void audio_settings_open(struct main_window *w)
{
	if(w->audio_settings) {
		gtk_window_present(GTK_WINDOW(w->audio_settings->dialog));
		return;
	}

	struct audio_settings *as = calloc(1, sizeof(*as));
	if(!as) return;
	as->w = w;
	w->audio_settings = as;

	/* Transient for the main window, so it stays on top of it, but not modal */
	as->dialog = gtk_dialog_new_with_buttons("Audio settings", GTK_WINDOW(w->window),
		GTK_DIALOG_DESTROY_WITH_PARENT, "_Close", GTK_RESPONSE_CLOSE, NULL);
	gtk_window_set_resizable(GTK_WINDOW(as->dialog), FALSE);

	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(as->dialog))), grid);

	grid_label(grid, "Audio system", 0);
	as->api_combo = gtk_combo_box_text_new();
	gtk_widget_set_tooltip_text(as->api_combo,
		"The sound system used to record.\n"
		"Choosing another one selects its\n"
		"default input and the default rate.");
	gtk_grid_attach(GTK_GRID(grid), as->api_combo, 1, 0, 2, 1);

	grid_label(grid, "Input device", 1);
	as->device_combo = gtk_combo_box_text_new();
	gtk_widget_set_hexpand(as->device_combo, TRUE);
	gtk_grid_attach(GTK_GRID(grid), as->device_combo, 1, 1, 2, 1);

	grid_label(grid, "Sample rate", 2);
	as->rate_combo = gtk_combo_box_text_new();
	gtk_widget_set_tooltip_text(as->rate_combo, "Only the rates the input device accepts are listed.");
	gtk_grid_attach(GTK_GRID(grid), as->rate_combo, 1, 2, 2, 1);

	grid_label(grid, "Input level", 3);
	as->level_bar = gtk_level_bar_new_for_interval(0, 1);
	gtk_widget_set_hexpand(as->level_bar, TRUE);
	gtk_widget_set_valign(as->level_bar, GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(grid), as->level_bar, 1, 3, 1, 1);
	as->level_label = gtk_label_new("");
	gtk_label_set_width_chars(GTK_LABEL(as->level_label), 9);
	gtk_label_set_xalign(GTK_LABEL(as->level_label), 1);
	gtk_grid_attach(GTK_GRID(grid), as->level_label, 2, 3, 1, 1);

	as->status_label = gtk_label_new("");
	gtk_label_set_xalign(GTK_LABEL(as->status_label), 0);
	gtk_label_set_line_wrap(GTK_LABEL(as->status_label), TRUE);
	gtk_label_set_max_width_chars(GTK_LABEL(as->status_label), 50);
	gtk_label_set_selectable(GTK_LABEL(as->status_label), TRUE);
	gtk_grid_attach(GTK_GRID(grid), as->status_label, 0, 4, 3, 1);

	refresh(as);

	g_signal_connect(as->api_combo, "changed", G_CALLBACK(handle_api_changed), as);
	g_signal_connect(as->device_combo, "changed", G_CALLBACK(handle_device_changed), as);
	g_signal_connect(as->rate_combo, "changed", G_CALLBACK(handle_rate_changed), as);
	g_signal_connect(as->dialog, "response", G_CALLBACK(handle_response), as);
	g_signal_connect(as->dialog, "destroy", G_CALLBACK(handle_destroy), as);

	as->level_timeout = g_timeout_add(LEVEL_UPDATE_MS, (GSourceFunc)update_level, as);
	update_level(as);

	gtk_widget_show_all(as->dialog);
	gtk_window_present(GTK_WINDOW(as->dialog));
}

/* Show the current audio state in the top bar button and, if open, the dialog */
void audio_settings_update(struct main_window *w)
{
	if(w->audio_button_label) {
		gchar *summary = audio_summary(w);
		gtk_label_set_text(GTK_LABEL(w->audio_button_label), summary);
		gchar *tip = g_strconcat(summary, "\nClick to change the audio settings.", NULL);
		gtk_widget_set_tooltip_text(w->audio_button, tip);
		g_free(tip);
		g_free(summary);
	}
	if(w->audio_settings)
		refresh(w->audio_settings);
}

int audio_settings_is_open(struct main_window *w)
{
	return w->audio_settings != NULL;
}
