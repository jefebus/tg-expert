/*
    tg
    Copyright (C) 2015 Marcello Mamino
    Copyright (C) 2026 jefebus

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

#include "tg.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <ctype.h>

#ifdef DEBUG
int testing = 0;
#endif

int preset_bph[] = PRESET_BPH;

static guint kick_computer(struct main_window *w);
guint save_on_change_timer(struct main_window *w);
static void remove_runtime_timers(struct main_window *w);
static void update_sensitivity(struct main_window *w);

void print_debug(char *format,...)
{
	va_list args;
	va_start(args,format);
	vfprintf(stderr,format,args);
	va_end(args);
}

void error(char *format,...)
{
	char s[100];
	va_list args;

	va_start(args,format);
	int size = vsnprintf(s,100,format,args);
	va_end(args);

	char *t;
	if(size < 100) {
		t = s;
	} else {
		t = alloca(size+1);
		va_start(args,format);
		vsnprintf(t,size+1,format,args);
		va_end(args);
	}

	fprintf(stderr,"%s\n",t);

#ifdef DEBUG
	if(testing) return;
#endif

	GtkWidget *dialog = gtk_message_dialog_new(NULL,0,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,"%s",t);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void refresh_results(struct main_window *w)
{
	w->active_snapshot->bph = w->bph;
	w->active_snapshot->la = w->la;
	w->active_snapshot->cal = w->cal;
	compute_results(w->active_snapshot);
}

static void kill_computer(struct main_window *w);
static void watch_sync_selection(struct main_window *w);
static void show_position(struct main_window *w, int pos);
static int session_watch_change_allowed(struct main_window *w, const char *name);
static void session_mark_changed(struct main_window *w);
static int session_confirm_discard(struct main_window *w);

static void handle_bph_change(GtkComboBox *b, struct main_window *w)
{
	if(!w->controls_active) return;
	char *s = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(b));
	if(s) {
		int bph;
		char *t;
		int n = (int)strtol(s,&t,10);
		if(*t || n < MIN_BPH || n > MAX_BPH) bph = 0;
		else bph = n;
		g_free(s);
		w->bph = bph;
		refresh_results(w);
		gtk_widget_queue_draw(w->notebook);
		watch_sync_selection(w);
	}
}

static void handle_la_change(GtkSpinButton *b, struct main_window *w)
{
	if(!w->controls_active) return;
	double la = gtk_spin_button_get_value(b);
	if(la < MIN_LA || la > MAX_LA) la = DEFAULT_LA;
	w->la = la;
	refresh_results(w);
	gtk_widget_queue_draw(w->notebook);
	watch_sync_selection(w);
}

/* Show bph in the bph combo box: a preset entry, "guess" (0) or custom text */
static void set_bph_combo(struct main_window *w, int bph)
{
	int i, current = 0;
	for(i = 0; preset_bph[i]; i++) {
		if(bph == preset_bph[i]) {
			current = i+1;
			break;
		}
	}
	if(current || bph == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(w->bph_combo_box), current);
	else {
		char s[32];
		sprintf(s,"%d",bph);
		GtkEntry *e = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(w->bph_combo_box)));
		gtk_entry_set_text(e,s);
	}
}

/* ---- Saved watches (bph + lift angle presets stored in the config file) ---- */

/* TRANSLATORS: entry of the saved watches list meaning that no watch is selected */
#define WATCH_NONE_LABEL N_("(none)")

/* Name of the watch selected in the combo box, or NULL for "(none)" */
static const char *selected_watch(struct main_window *w)
{
	const gchar *id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(w->watch_combo_box));
	return id && *id ? id : NULL;
}

static void watch_update_buttons(struct main_window *w)
{
	// Watches change bph and lift angle, which are locked while calibrating
	int enabled = w->controls_active && !w->calibrate && !w->calibration_stop_pending;
	gtk_widget_set_sensitive(w->watch_combo_box, enabled);
	gtk_widget_set_sensitive(w->watch_save_button, enabled);
	gtk_widget_set_sensitive(w->watch_delete_button, enabled && selected_watch(w));
}

/* Select a watch by name without applying it (NULL selects "(none)") */
static void watch_select(struct main_window *w, const char *name)
{
	w->watch_updating++;
	if(!name || !gtk_combo_box_set_active_id(GTK_COMBO_BOX(w->watch_combo_box), name))
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(w->watch_combo_box), "");
	w->watch_updating--;
	watch_update_buttons(w);
}

/* The last watch the user picked, kept in [tg] so it survives restarts */
static gchar *watch_remembered(struct main_window *w)
{
	return g_key_file_get_string(w->config_file, "tg", "watch", NULL);
}

static void watch_remember(struct main_window *w, const char *name)
{
	if(name)
		g_key_file_set_string(w->config_file, "tg", "watch", name);
	else
		g_key_file_remove_key(w->config_file, "tg", "watch", NULL);
}

static int watch_matches(struct main_window *w, const char *name, int bph, double la)
{
	int b;
	double l;
	if(!name || get_watch(w, name, &b, &l)) return 0;
	return b == bph && fabs(l - la) < 0.05;
}

/* Rebuild the combo box from the config file and select `name` */
static void watch_populate(struct main_window *w, const char *name)
{
	GtkComboBoxText *c = GTK_COMBO_BOX_TEXT(w->watch_combo_box);
	w->watch_updating++;
	gtk_combo_box_text_remove_all(c);
	gtk_combo_box_text_append(c, "", _(WATCH_NONE_LABEL));
	gchar **names = list_watches(w);
	int i;
	for(i = 0; names[i]; i++)
		gtk_combo_box_text_append(c, names[i], names[i]);
	g_strfreev(names);
	w->watch_updating--;
	watch_select(w, name);
}

/* Select the remembered watch if the current bph/lift angle still match it */
static void watch_restore(struct main_window *w)
{
	gchar *name = watch_remembered(w);
	watch_select(w, watch_matches(w, name, w->bph, w->la) ? name : NULL);
	g_free(name);
}

/* After a manual bph/lift angle edit, drop the selection if it no longer matches */
static void watch_sync_selection(struct main_window *w)
{
	if(w->watch_updating) return;
	const char *name = selected_watch(w);
	if(name && !watch_matches(w, name, w->bph, w->la))
		watch_select(w, NULL);
}

static void watch_apply(struct main_window *w, int bph, double la)
{
	w->watch_updating++;
	set_bph_combo(w, bph);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->la_spin_button), la);
	w->watch_updating--;

	w->bph = bph;
	w->la = la;
	refresh_results(w);
	gtk_widget_queue_draw(w->notebook);
}

static void handle_watch_change(GtkComboBox *b, struct main_window *w)
{
	UNUSED(b);
	if(w->watch_updating || !w->controls_active) return;
	watch_update_buttons(w);

	// The selected watch is the watch of the session: ask before changing it
	// when the session already has measurements (or put the selection back)
	if(!session_watch_change_allowed(w, selected_watch(w))) return;
	const char *name = selected_watch(w);
	if(!name) {
		watch_remember(w, NULL);
		return;
	}

	int bph;
	double la;
	if(get_watch(w, name, &bph, &la)) {
		watch_populate(w, NULL);
		return;
	}
	watch_remember(w, name);
	watch_apply(w, bph, la);
}

static void watch_message(struct main_window *w, GtkMessageType type, const char *text)
{
	GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			type, GTK_BUTTONS_CLOSE, "%s", text);
	gtk_dialog_run(GTK_DIALOG(d));
	gtk_widget_destroy(d);
}

static int watch_confirm(struct main_window *w, const char *text)
{
	GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", text);
	int ret = gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_YES;
	gtk_widget_destroy(d);
	return ret;
}

static void handle_watch_save(GtkButton *b, struct main_window *w)
{
	UNUSED(b);
	if(!w->controls_active) return;

	const char *sel = selected_watch(w);
	gchar *prefill = sel ? g_strdup(sel) : watch_remembered(w);

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Save watch"), GTK_WINDOW(w->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Save"), GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_set_border_width(GTK_CONTAINER(area), 10);
	gtk_box_set_spacing(GTK_BOX(area), 6);

	/* TRANSLATORS: "bph guess" is the text shown for automatic beat rate detection; it replaces %s in the next message */
	gchar *bph_text = w->bph ? g_strdup_printf("%d bph", w->bph) : g_strdup(_("bph guess"));
	gchar *text = g_strdup_printf(_("Save the current settings (%s, lift angle %g deg) as:"), bph_text, w->la);
	GtkWidget *label = gtk_label_new(text);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_box_pack_start(GTK_BOX(area), label, FALSE, FALSE, 0);
	g_free(text);
	g_free(bph_text);

	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), WATCH_NAME_MAX);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_entry_set_placeholder_text(GTK_ENTRY(entry), _("Watch name"));
	if(prefill) gtk_entry_set_text(GTK_ENTRY(entry), prefill);
	gtk_box_pack_start(GTK_BOX(area), entry, FALSE, FALSE, 0);
	g_free(prefill);

	gtk_widget_show_all(area);
	gtk_widget_grab_focus(entry);

	gchar *name = NULL;
	while(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *t = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))));
		if(!watch_name_valid(t)) {
			gchar *msg = g_strdup_printf(_("Please enter a name of up to %d characters, without '[' or ']'."), WATCH_NAME_MAX);
			watch_message(w, GTK_MESSAGE_ERROR, msg);
			g_free(msg);
			g_free(t);
			continue;
		}
		if(watch_exists(w, t)) {
			gchar *msg = g_strdup_printf(_("A watch named \"%s\" already exists.\nReplace it with the current settings?"), t);
			int ok = watch_confirm(w, msg);
			g_free(msg);
			if(!ok) {
				g_free(t);
				continue;
			}
		}
		name = t;
		break;
	}
	gtk_widget_destroy(dialog);

	if(name) {
		watch_remember(w, name);
		set_watch(w, name, w->bph, w->la); // also writes the config file
		watch_populate(w, name);
		// The new watch is now selected: it becomes the watch of the session
		// as if picked in the selector (it has the current bph and lift angle)
		session_watch_change_allowed(w, name);
		g_free(name);
	}
}

static void handle_watch_delete(GtkButton *b, struct main_window *w)
{
	UNUSED(b);
	if(!w->controls_active) return;
	const char *sel = selected_watch(w);
	if(!sel) return;

	gchar *name = g_strdup(sel);
	gchar *msg = g_strdup_printf(_("Delete the saved watch \"%s\"?\nThe current bph and lift angle are kept."), name);
	if(watch_confirm(w, msg)) {
		gchar *remembered = watch_remembered(w);
		if(remembered && !strcmp(remembered, name))
			watch_remember(w, NULL);
		g_free(remembered);
		delete_watch(w, name); // also writes the config file
		watch_populate(w, NULL);
	}
	g_free(msg);
	g_free(name);
}

static void handle_cal_change(GtkSpinButton *b, struct main_window *w)
{
	if(!w->controls_active) return;
	int cal = gtk_spin_button_get_value(b);
	w->cal = cal;
	refresh_results(w);
	gtk_widget_queue_draw(w->notebook);
}

/* Restart audio with another audio system, input device or sample rate.
 * The change is applied when the computation thread has stopped, in
 * computer_terminated(). Used by the audio settings dialog. */
void request_audio_restart(struct main_window *w, int api, int device, int rate)
{
	if(!w->controls_active || w->calibrate || w->calibration_stop_pending ||
			w->restart_audio || w->restart_algorithm) return;
	if(w->listen_active) {
		stop_audio_monitor();
		w->listen_active = 0;
		audio_settings_update(w);
	}
	w->pending_audio_api = api;
	w->pending_audio_device = device;
	w->pending_audio_rate = rate;
	w->restart_audio = 1;
	w->audio_message = NULL;
	update_sensitivity(w);

	lock_computer(w->computer);
	if(w->computer->recompute >= 0)
		kill_computer(w);
	unlock_computer(w->computer);
}

static gboolean output_cal(GtkSpinButton *spin, gpointer data)
{
	UNUSED(data);
	GtkAdjustment *adj;
	gchar *text;
	int value;

	adj = gtk_spin_button_get_adjustment (spin);
	value = (int)gtk_adjustment_get_value (adj);
	text = g_strdup_printf ("%c%d.%d", value < 0 ? '-' : '+', abs(value)/10, abs(value)%10);
	gtk_entry_set_text (GTK_ENTRY (spin), text);
	g_free (text);

	return TRUE;
}

static gboolean input_cal(GtkSpinButton *spin, double *val, gpointer data)
{
	UNUSED(data);
	double x = 0;
	sscanf(gtk_entry_get_text (GTK_ENTRY (spin)), "%lf", &x);
	int n = round(x*10);
	if(n < MIN_CAL) n = MIN_CAL;
	if(n > MAX_CAL) n = MAX_CAL;
	*val = n;

	return TRUE;
}

static void on_shutdown(GApplication *app, void *p)
{
	UNUSED(p);
	debug("Main loop has terminated\n");
	struct main_window *w = g_object_get_data(G_OBJECT(app), "main-window");
	if(w) {
		save_config(w);
		if(w->computer)
			computer_destroy(w->computer);
		op_destroy(w->active_panel);
		project_metadata_free(&w->project);
		g_free(w->session_path);
		close_config(w);
		g_free(w->audio_device_name);
		g_free(w->audio_output_device_name);
		free(w);
	}
	terminate_portaudio();
}

static void recompute(struct main_window *w);
static void computer_callback(void *w);
guint refresh(struct main_window *w);
static gboolean watch_audio_monitor(struct main_window *w);

static void remove_runtime_timers(struct main_window *w)
{
	if(w->kick_timeout) {
		g_source_remove(w->kick_timeout);
		w->kick_timeout = 0;
	}
	if(w->save_timeout) {
		g_source_remove(w->save_timeout);
		w->save_timeout = 0;
	}
}

/* Shown once at startup when no input could be opened, with a way to fix it */
static void startup_audio_warning(struct main_window *w)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, _("No audio input could be opened."));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
		_("Choose an audio system and input device in the audio settings, "
		"or check the system's microphone settings."));
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
		_("_Close"), GTK_RESPONSE_CLOSE,
		_("_Audio settings…"), GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	int response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if(response == GTK_RESPONSE_ACCEPT)
		audio_settings_open(w);
}

/* One attempt to open the audio input. Errors are collected by the caller. */
static int open_audio(int device, int rate, double *real_sr)
{
	int nominal = rate;
	terminate_portaudio();
	set_audio_input_device(device);
	set_audio_sample_rate(rate);
	*real_sr = rate;
	return start_portaudio(&nominal, real_sr);
}

static guint computer_terminated(struct main_window *w)
{
	if(w->zombie) {
		debug("Closing main window\n");
		gtk_widget_destroy(w->window);
	} else {
		debug("Restarting computer");

		int algorithm_changed = w->restart_algorithm;
		if(algorithm_changed) {
			computer_join(w->computer);
			algorithm_settings_load(w);
		}

		if(w->restart_audio) {
			int requested_device = w->pending_audio_device;
			int requested_rate = w->pending_audio_rate;
			double real_sr;
			/* The audio system changes even if no device can be opened,
			 * so that the settings dialog lists its devices */
			w->audio_api = w->pending_audio_api;
			set_audio_api(w->audio_api);
			set_audio_input_device_name(NULL);
			/* Collect errors from the attempts and report once at the end */
			set_audio_errors_quiet(1);
			int failed = open_audio(requested_device, requested_rate, &real_sr);
			if(failed && requested_rate != PA_SAMPLE_RATE)
				failed = open_audio(requested_device, PA_SAMPLE_RATE, &real_sr);
			if(failed && requested_device != AUDIO_DEVICE_DEFAULT)
				failed = open_audio(AUDIO_DEVICE_DEFAULT, PA_SAMPLE_RATE, &real_sr);
			set_audio_errors_quiet(0);

			w->audio_ok = !failed;
			if(failed) {
				/* PortAudio stays initialized so that devices can still be listed */
				w->audio_device = AUDIO_DEVICE_DEFAULT;
				w->nominal_sr = PA_SAMPLE_RATE;
				w->audio_message = _("Could not open this input. It may be in use or disabled.");
			} else {
				w->audio_device = get_audio_input_device();
				w->nominal_sr = get_audio_sample_rate();
				const char *name = get_audio_input_device_name();
				g_free(w->audio_device_name);
				w->audio_device_name = g_strdup(name ? name : "");
				/* A request for the default input is met by whichever device that is */
				if(requested_device != AUDIO_DEVICE_DEFAULT && w->audio_device != requested_device)
					w->audio_message = _("The chosen input could not be opened; using the default input.");
				else if(w->nominal_sr != requested_rate)
					w->audio_message = _("The chosen rate is not available; using the default rate.");
			}
			w->pending_audio_api = w->audio_api;
			w->pending_audio_device = w->audio_device;
			w->pending_audio_rate = w->nominal_sr;
			w->restart_audio = 0;
			/* Readings from another input are not comparable */
			trend_reset(w->active_panel->trend);
			/* The settings dialog shows the result itself */
			if(w->audio_message && !audio_settings_is_open(w))
				error("%s", w->audio_message);
		}

		struct computer *c = start_computer(w->nominal_sr, w->bph, w->la, w->cal, w->is_light);
		if(!c) {
			remove_runtime_timers(w);
			w->zombie = 1;
			error(_("Failed to restart computation thread"));
			gtk_widget_destroy(w->window);
		} else {
			computer_destroy(w->computer);
			w->active_panel->computer = w->computer = c;

			w->computer->callback = computer_callback;
			w->computer->callback_data = w;

			if(algorithm_changed) {
				trend_reset(w->active_panel->trend);
				refresh(w);
			}
			w->restart_algorithm = 0;
			recompute(w);
			update_sensitivity(w);
		}
	}
	return FALSE;
}

static void computer_quit(void *w)
{
	gdk_threads_add_idle((GSourceFunc)computer_terminated,w);
}

static void kill_computer(struct main_window *w)
{
	w->computer->recompute = -1;
	w->computer->callback = computer_quit;
	w->computer->callback_data = w;
}

static gboolean quit(struct main_window *w)
{
	remove_runtime_timers(w);
	w->zombie = 1;
	if(w->computer) {
		lock_computer(w->computer);
		kill_computer(w);
		unlock_computer(w->computer);
	} else {
		gtk_widget_destroy(w->window);
	}
	return FALSE;
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer w)
{
	UNUSED(widget);
	UNUSED(event);
	debug("Received delete event\n");
	struct main_window *mw = w;
	if(!mw->zombie && session_confirm_discard(mw))
		quit(mw);
	return TRUE;
}

static void handle_quit(GtkMenuItem *m, struct main_window *w)
{
	UNUSED(m);
	if(!w->zombie && session_confirm_discard(w))
		quit(w);
}

static void recompute(struct main_window *w)
{
	w->computer_timeout = 0;
	lock_computer(w->computer);
	if(w->computer->recompute >= 0) {
		if(w->is_light != w->computer->actv->is_light) {
			kill_computer(w);
		} else {
			w->computer->bph = w->bph;
			w->computer->la = w->la;
			w->computer->calibrate = w->calibrate;
			w->computer->recompute = 1;
		}
	}
	unlock_computer(w->computer);
}

static guint kick_computer(struct main_window *w)
{
	watch_audio_monitor(w);
	op_update_settling_status(w->active_panel);
	w->computer_timeout++;
	if(w->calibrate && w->computer_timeout < 10) {
		return TRUE;
	} else {
		recompute(w);
		return TRUE;
	}
}

/* Menu items whose availability depends on the program state (e.g. "Export
 * data" only with Trend readings) remember the wanted state here, so that
 * calibration can lock them and later restore them correctly. */
static void set_menu_item_enabled(struct main_window *w, GtkWidget *item, int enabled)
{
	g_object_set_data(G_OBJECT(item), "enabled-wanted", GINT_TO_POINTER(enabled ? 1 : 2));
	gtk_widget_set_sensitive(item, enabled && !w->calibrate && !w->calibration_stop_pending);
}

static int menu_item_wanted(GtkWidget *item)
{
	return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "enabled-wanted")) != 2;
}

static struct trend *current_trend(struct main_window *w, GtkWidget **tab)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(w->notebook));
	GtkWidget *current = page >= 0 ?
		gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), page) : NULL;
	if(tab)
		*tab = current;
	if(!current)
		return NULL;
	struct output_panel *op = g_object_get_data(G_OBJECT(current), "op-pointer");
	if(op)
		return op->trend;
	return w->active_panel ? w->active_panel->trend : NULL;
}

static void update_export_item(struct main_window *w)
{
	if(!w->export_item)
		return;
	struct trend *trend = current_trend(w, NULL);
	int enabled = trend && trend->count > 0;
	if(menu_item_wanted(w->export_item) != enabled)
		set_menu_item_enabled(w, w->export_item, enabled);
}

/* Apply widget sensitivity from controls_active (real-time tab shown) and calibrate */
static void update_sensitivity(struct main_window *w)
{
	int live = w->controls_active;
	int calibration_locked = w->calibrate || w->calibration_stop_pending;
	int settings = live && !calibration_locked && !w->restart_algorithm;

	watch_update_buttons(w);
	gtk_widget_set_sensitive(w->bph_combo_box, settings);
	gtk_widget_set_sensitive(w->la_spin_button, settings);
	gtk_widget_set_sensitive(w->cal_spin_button, settings);
	if(w->listen_button)
		gtk_widget_set_sensitive(w->listen_button, w->audio_ok && !w->restart_audio);
	// The audio settings dialog locks itself when audio cannot be changed
	audio_settings_update(w);

	// Always allow stopping a running calibration, whichever tab is shown
	gtk_widget_set_sensitive(w->cal_button,
		(live || w->calibrate) && !w->restart_algorithm && !w->calibration_stop_pending);
	gtk_menu_item_set_label(GTK_MENU_ITEM(w->cal_button),
			w->calibrate ? _("Stop calibration") : _("Start calibration"));

	// Other menu entries are locked while calibrating (Quit stays available)
	gtk_widget_set_sensitive(w->new_session_item, !calibration_locked);
	gtk_widget_set_sensitive(w->open_item, !calibration_locked);
	gtk_widget_set_sensitive(w->algorithm_item, !calibration_locked && !w->restart_audio && !w->restart_algorithm);
	gtk_widget_set_sensitive(w->save_item, !calibration_locked);
	gtk_widget_set_sensitive(w->save_as_item, !calibration_locked);
	if(w->export_item)
		gtk_widget_set_sensitive(w->export_item,
			menu_item_wanted(w->export_item) && !calibration_locked);
}

static void handle_calibrate(GtkMenuItem *b, struct main_window *w)
{
	UNUSED(b);
	w->calibration_stop_pending = w->calibrate;
	w->calibrate = !w->calibrate;
	update_sensitivity(w);
	recompute(w);
}

static void handle_listen_toggled(GtkToggleButton *button, struct main_window *w)
{
	if(w->listen_updating)
		return;
	if(!gtk_toggle_button_get_active(button)) {
		pause_audio_monitor();
		w->listen_active = 0;
		audio_settings_update(w);
		return;
	}
	if(!w->listen_warning_shown) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
			_("Use headphones for live listening. Speakers can feed back into the microphone; "
			  "TG-Expert cannot detect whether headphones are connected."));
		gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Continue"), GTK_RESPONSE_ACCEPT,
			NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
		int response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		w->listen_warning_shown = 1;
		if(response != GTK_RESPONSE_ACCEPT) {
			w->listen_active = 0;
			audio_settings_update(w);
			return;
		}
	}

	char *message = NULL;
	if(!w->audio_ok || start_audio_monitor(get_audio_output_device(),
			w->listen_volume, &message)) {
		w->listen_active = 0;
		w->audio_message = _("Live listening could not be started.");
		audio_settings_update(w);
		if(message) {
			error("%s", message);
			g_free(message);
		} else
			error("%s", w->audio_message);
		return;
	}
	w->audio_message = NULL;
	w->listen_active = 1;
	audio_settings_update(w);
}

static gboolean watch_audio_monitor(struct main_window *w)
{
	if(!w->listen_active)
		return !w->zombie;
	int status = audio_monitor_status();
	if(status == 1)
		return TRUE;
	stop_audio_monitor();
	w->listen_active = 0;
	w->audio_message = _("Live listening stopped because the selected output failed. "
		"Select an available output and activate listening again.");
	audio_settings_update(w);
	error("%s", w->audio_message);
	return !w->zombie;
}

static void format_calibration_value(int value, char *buffer, size_t size)
{
	snprintf(buffer, size, "%s%d.%d", value < 0 ? "-" : "+",
		abs(value) / 10, abs(value) % 10);
}

static void apply_calibration_value(struct main_window *w, int value, gboolean persist)
{
	w->cal = value;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->cal_spin_button), value);
	if(w->controls_active) {
		refresh_results(w);
		gtk_widget_queue_draw(w->notebook);
	}

	if(persist) {
		GError *error = NULL;
		if(!save_config_checked(w, &error)) {
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				_("The correction was applied in memory but could not be saved:\n%s"),
				error->message);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			g_error_free(error);
		}
	}
}

static gboolean confirm_provisional_calibration(struct main_window *w,
	const struct snapshot *snapshot)
{
	char estimate[32], previous[32];
	format_calibration_value(snapshot->cal_estimate_result, estimate, sizeof(estimate));
	format_calibration_value(w->cal, previous, sizeof(previous));
	char *message = g_strdup_printf(
		_("Calibration is not complete.\n\n"
		  "Provisional estimate: ~%s s/d\n"
		  "Previous correction: %s s/d\n"
		  "Data used: %d observations / %.0f s\n\n"
		  "Replace the previous correction with this estimate?"),
		estimate, previous, snapshot->cal_estimate_observations,
		snapshot->cal_estimate_duration);
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "%s", message);
	g_free(message);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
		_("_Keep previous calibration"), GTK_RESPONSE_CANCEL,
		_("_Apply estimate"), GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
	int response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return response == GTK_RESPONSE_ACCEPT;
}

void request_algorithm_restart(struct main_window *w, int light)
{
	w->is_light = light;
	w->restart_algorithm = 1;
	update_sensitivity(w);
	lock_computer(w->computer);
	if(w->computer->recompute >= 0)
		kill_computer(w);
	unlock_computer(w->computer);
}

static void controls_active(struct main_window *w, int active)
{
	w->controls_active = active;
	update_sensitivity(w);
	if(active) {
		gtk_widget_show(w->snapshot_button);
		gtk_widget_hide(w->snapshot_name);
	} else {
		gtk_widget_hide(w->snapshot_button);
		gtk_widget_show(w->snapshot_name);
	}
}

static int blank_string(char *s)
{
	if(!s) return 1;
	for(;*s;s++)
		if(!isspace((unsigned char)*s)) return 0;
	return 1;
}

static void handle_tab_changed(GtkNotebook *nbk, GtkWidget *panel, guint x, struct main_window *w)
{
	UNUSED(nbk);
	UNUSED(x);
	// These are NULL for the Real Time tab
	struct output_panel *op = g_object_get_data(G_OBJECT(panel), "op-pointer");
	char *tab_name = g_object_get_data(G_OBJECT(panel), "tab-name");

	controls_active(w, !op);

	// Measurements loaded from a session file keep their position; any snapshot
	// can be renamed, except the saved state, whose title comes from its role
	int from_file = op && GPOINTER_TO_INT(g_object_get_data(G_OBJECT(panel), "from-file"));
	gtk_widget_set_sensitive(w->snapshot_name_entry, !op || !op->snst->saved_state);
	gtk_widget_set_sensitive(w->snapshot_position_combo, !from_file);

	int bph, cal;
	double la;
	if(op) {
		w->session_updating++; // showing the name is not renaming
		gtk_entry_set_text(GTK_ENTRY(w->snapshot_name_entry), tab_name ? tab_name : "");
		w->session_updating--;
		show_position(w, op->snst->position);
		bph = op->snst->bph;
		cal = op->snst->cal;
		la = op->snst->la;
	} else {
		show_position(w, w->live_position);
		bph = w->bph;
		cal = w->cal;
		la = w->la;
	}

	set_bph_combo(w, bph);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->la_spin_button), la);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->cal_spin_button), cal);
	update_export_item(w);

	// Snapshot tabs show their own bph/la, so no saved watch is selected there
	if(op)
		watch_select(w, NULL);
	else
		watch_restore(w);
}

static void handle_tab_closed(GtkNotebook *nbk, GtkWidget *panel, guint x, struct main_window *w)
{
	UNUSED(x);
	if(gtk_notebook_get_n_pages(nbk) == 1 && !w->zombie) {
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbk), FALSE);
		gtk_notebook_set_show_border(GTK_NOTEBOOK(nbk), FALSE);
	}
	// Now, are we sure that we are not going to segfault?
	struct output_panel *op = g_object_get_data(G_OBJECT(panel), "op-pointer");
	if(op) op_destroy(op);
	free(g_object_get_data(G_OBJECT(panel), "tab-name"));
	g_object_set_data(G_OBJECT(panel), "tab-name", NULL);
	if(op && !w->zombie)
		session_mark_changed(w); // ignored while the session is being replaced
}

static void handle_close_tab(GtkButton *b, struct output_panel *p)
{
	UNUSED(b);
	gtk_widget_destroy(p->panel);
}

/* ---- Watch position of snapshots ---- */

static const char *position_codes[POS_COUNT] = {
	NULL, "DU", "DD", "CU", "CD", "CL", "CR"
};

/* Short code of a position (DU, DD, ...), NULL if not set */
const char *watch_position_code(int pos)
{
	return pos > POS_NONE && pos < POS_COUNT ? position_codes[pos] : NULL;
}

/* The position names are in a combo box and in the tab titles. The codes in
 * parentheses are the same in every language, see the comments below. */
static const char *position_labels[POS_COUNT] = {
	N_("(not set)"),
	/* TRANSLATORS: watch position. Translate the words, keep the code (DU) unchanged */
	N_("Dial up (DU)"),
	/* TRANSLATORS: watch position. Translate the words, keep the code (DD) unchanged */
	N_("Dial down (DD)"),
	/* TRANSLATORS: watch position. Translate the words, keep the code (CU) unchanged */
	N_("Crown up (CU)"),
	/* TRANSLATORS: watch position. Translate the words, keep the code (CD) unchanged */
	N_("Crown down (CD)"),
	/* TRANSLATORS: watch position. Translate the words, keep the code (CL) unchanged */
	N_("Crown left (CL)"),
	/* TRANSLATORS: watch position. Translate the words, keep the code (CR) unchanged */
	N_("Crown right (CR)")
};

const char *watch_position_label(int pos)
{
	if(pos < POS_NONE || pos >= POS_COUNT)
		pos = POS_NONE;
	return _(position_labels[pos]);
}

/* Tab title: "<name or Snapshot>" plus " (<position>)" when a position is set */
static void update_tab_label(GtkWidget *panel)
{
	GtkLabel *label = g_object_get_data(G_OBJECT(panel), "tab-label");
	struct output_panel *op = g_object_get_data(G_OBJECT(panel), "op-pointer");
	if(!label || !op) return;

	const char *name = g_object_get_data(G_OBJECT(panel), "tab-name");
	/* TRANSLATORS: title of the tab with what the real-time tab showed when the
	 * session was saved. Short (tab title) */
	if(op->snst->saved_state) name = _("Saved state");
	/* TRANSLATORS: default title of a snapshot tab, short (tab title) */
	if(!name) name = _("Snapshot");
	int pos = op->snst->position;

	if(pos > POS_NONE && pos < POS_COUNT) {
		gchar *text = g_strdup_printf("%s (%s)", name, position_codes[pos]);
		gtk_label_set_text(label, text);
		g_free(text);
	} else
		gtk_label_set_text(label, name);
}

static void handle_name_change(GtkEntry *e, struct main_window *w)
{
	int p = gtk_notebook_get_current_page(GTK_NOTEBOOK(w->notebook));
	GtkWidget *panel = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), p);
	if(!g_object_get_data(G_OBJECT(panel), "op-pointer")) return; // real-time tab
	if(w->session_updating) return; // the entry is only showing the name
	struct output_panel *op = g_object_get_data(G_OBJECT(panel), "op-pointer");
	if(op->snst->saved_state) return; // titled by its role, not by a name
	char *old = g_object_get_data(G_OBJECT(panel), "tab-name");
	char *name = (char *)gtk_entry_get_text(e);
	name = blank_string(name) ? NULL : strdup(name);
	int changed = g_strcmp0(old, name) != 0;
	free(old);
	g_object_set_data(G_OBJECT(panel), "tab-name", name);
	update_tab_label(panel);
	if(changed)
		session_mark_changed(w);
}

/* Path of the installed manual, or NULL if it is not there. It is looked for
 * in DOCDIR, the build's $(docdir), and in doc/PACKAGE under the system data
 * folders: /usr/share, /usr/local/share... on Linux, and on Windows the share
 * folder of the installation, where the installer puts it. */
static gchar *find_doc_file(const char *name)
{
	gchar *path = g_build_filename(DOCDIR, name, NULL);
	if(g_file_test(path, G_FILE_TEST_IS_REGULAR))
		return path;
	g_free(path);
	const gchar * const *dirs;
	for(dirs = g_get_system_data_dirs(); *dirs; dirs++) {
		path = g_build_filename(*dirs, "doc", PACKAGE, name, NULL);
		if(g_file_test(path, G_FILE_TEST_IS_REGULAR))
			return path;
		g_free(path);
	}
	return NULL;
}

/* The manual in the user's language (MANUAL_FILE_NAME with -<lang> before
 * the extension, e.g. tg-expert-manual-es.pdf), else the English one */
static gchar *find_manual(void)
{
	const gchar * const *langs;
	for(langs = g_get_language_names(); *langs; langs++) {
		if(!strcmp(*langs, "C") || strchr(*langs, '.') || strchr(*langs, '@'))
			continue;	// "C" is English; es_ES.UTF-8 also comes as es_ES and es
		gchar *base = g_strndup(MANUAL_FILE_NAME, strlen(MANUAL_FILE_NAME) - strlen(".pdf"));
		gchar *name = g_strdup_printf("%s-%s.pdf", base, *langs);
		gchar *path = find_doc_file(name);
		g_free(name);
		g_free(base);
		if(path)
			return path;
	}
	return find_doc_file(MANUAL_FILE_NAME);
}

/* Open the manual in the PDF viewer, or the releases page if it is not installed.
 * On Windows, GLib starts the viewer through gspawn-win64-helper.exe, which
 * the installer ships next to the executable (packaging/tg-expert_resources). */
static void handle_manual(GtkMenuItem *m, struct main_window *w)
{
	UNUSED(m);
	gchar *path = find_manual();
	gchar *uri = path ? g_filename_to_uri(path, NULL, NULL) : NULL;
	if(!uri)
		uri = g_strdup(MANUAL_URL);
	GError *err = NULL;
	if(!gtk_show_uri_on_window(GTK_WINDOW(w->window), uri, GDK_CURRENT_TIME, &err)) {
		error(_("Could not open the manual (%s): %s"), uri, err ? err->message : _("unknown error"));
		g_clear_error(&err);
	}
	g_free(uri);
	g_free(path);
}

static void handle_position_change(GtkComboBox *b, struct main_window *w)
{
	if(w->position_updating) return;
	int p = gtk_notebook_get_current_page(GTK_NOTEBOOK(w->notebook));
	GtkWidget *panel = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), p);
	struct output_panel *op = g_object_get_data(G_OBJECT(panel), "op-pointer");

	int pos = gtk_combo_box_get_active(b);
	if(pos < POS_NONE || pos >= POS_COUNT) pos = POS_NONE;
	if(!op) {
		// Real-time tab: remember it for the next snapshot
		if(pos != w->live_position && w->live_position != POS_NONE) {
			trend_start_settling(w->active_panel->trend, w->settling_seconds);
			op_update_settling_status(w->active_panel);
		}
		w->live_position = pos;
		return;
	}
	if(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(panel), "from-file"))) {
		show_position(w, op->snst->position); // kept as it was saved
		return;
	}
	if(op->snst->position == pos) return;
	op->snst->position = pos;
	update_tab_label(panel);
	session_mark_changed(w);
}

/* Show a position in the combo box without triggering a change */
static void show_position(struct main_window *w, int pos)
{
	if(pos < POS_NONE || pos >= POS_COUNT) pos = POS_NONE;
	w->position_updating++;
	gtk_combo_box_set_active(GTK_COMBO_BOX(w->snapshot_position_combo), pos);
	w->position_updating--;
}

#ifdef WIN_XP
static GtkWidget *image_from_file(char *filename)
{
	char *dir = g_win32_get_package_installation_directory_of_module(NULL);
	char *s;
	if(dir) {
		s = alloca( strlen(dir) + strlen(filename) + 2 );
		sprintf(s, "%s/%s", dir, filename);
	} else {
		s = filename;
	}
	GtkWidget *img = gtk_image_new_from_file(s);
	g_free(dir);
	return img;
}
#endif

static GtkWidget *make_tab_label(char *name, struct output_panel *panel_to_close)
{
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	/* TRANSLATORS: tab titles, short. "Real time" is the live measurement tab, "Snapshot" a kept measurement */
	const char *nm = panel_to_close ? name ? name : _("Snapshot") : _("Real time");
	GtkWidget *label = gtk_label_new(nm);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	if(panel_to_close) {
#ifdef WIN_XP
		GtkWidget *image = image_from_file("window-close.png");
#else
		GtkWidget *image = gtk_image_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
#endif
		GtkWidget *button = gtk_button_new();
		gtk_button_set_image(GTK_BUTTON(button), image);
		gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
		g_signal_connect(button, "clicked", G_CALLBACK(handle_close_tab), panel_to_close);
		gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
		g_object_set_data(G_OBJECT(panel_to_close->panel), "tab-close", button);
		g_object_set_data(G_OBJECT(panel_to_close->panel), "op-pointer", panel_to_close);
		g_object_set_data(G_OBJECT(panel_to_close->panel), "tab-label", label);
		g_object_set_data(G_OBJECT(panel_to_close->panel), "tab-name", name ? strdup(name) : NULL);
		update_tab_label(panel_to_close->panel);
	}

	gtk_widget_show_all(hbox);

	return hbox;
}

static void handle_paperstrip_settings_changed(void *data, double tick_seconds,
	double window_seconds)
{
	struct main_window *w = data;
	w->paperstrip_tick_seconds = tick_seconds;
	w->paperstrip_window_seconds = window_seconds;
	save_on_change(w);
}

static void handle_settling_setting_changed(void *data, int seconds)
{
	struct main_window *w = data;
	w->settling_seconds = seconds;
	save_on_change(w);
}

static struct output_panel *add_new_tab(struct snapshot *s, char *name, struct main_window *w)
{
	struct output_panel *op = init_output_panel(NULL, s, 5);
	op_set_paperstrip_settings(op, w->active_panel->paperstrip_tick_seconds,
		w->active_panel->paperstrip_window_seconds);
	GtkWidget *label = make_tab_label(name, op);
	gtk_widget_show_all(op->panel);

	op_set_border(w->active_panel, 5);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w->notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(w->notebook), TRUE);
	gtk_notebook_append_page(GTK_NOTEBOOK(w->notebook), op->panel, label);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(w->notebook), op->panel, TRUE);
	return op;
}

static void handle_snapshot(GtkButton *b, struct main_window *w)
{
	UNUSED(b);
	if(w->active_snapshot->calibrate) return;
	struct snapshot *s = snapshot_clone(w->active_snapshot);
	s->timestamp = get_timestamp(s->is_light);
	s->audio_status = w->active_panel->audio_status;
	s->position = w->live_position; // can still be changed on the snapshot tab
	// Name the snapshot after the selected watch, if any ("Snapshot" otherwise)
	struct output_panel *op = add_new_tab(s, (char *)selected_watch(w), w);
	// The snapshot keeps the long-term statistics as they are now
	op->trend = trend_clone(w->active_panel->trend);
	update_export_item(w);
	session_mark_changed(w);
}

/* Keyboard shortcut for the most frequent action: Ctrl+Space takes a snapshot.
 * A chord is used because a plain key would clash with typing in the entries
 * and with the focused button. It presses the button itself, so it is only
 * available when the button is: real-time tab, measurable signal, not calibrating. */
static gboolean handle_key_press(GtkWidget *widget, GdkEventKey *event, struct main_window *w)
{
	UNUSED(widget);
	guint mods = event->state & gtk_accelerator_get_default_mod_mask();
	if(mods == GDK_CONTROL_MASK && event->keyval == GDK_KEY_space) {
		if(gtk_widget_get_visible(w->snapshot_button) && gtk_widget_get_sensitive(w->snapshot_button))
			gtk_button_clicked(GTK_BUTTON(w->snapshot_button));
		return TRUE;
	}
	return FALSE;
}

static void chooser_set_filters(GtkFileChooser *chooser, const char *extension)
{
	GtkFileFilter *file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, extension);
	gchar *pattern = g_strdup_printf("*%s", extension);
	gtk_file_filter_add_pattern(file_filter, pattern);
	g_free(pattern);
	gtk_file_chooser_add_filter(chooser, file_filter);

	GtkFileFilter *all_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(all_filter, _("All files"));
	gtk_file_filter_add_pattern(all_filter, "*");
	gtk_file_chooser_add_filter(chooser, all_filter);

	// On windows seems not to work...
	gtk_file_chooser_set_filter(chooser, file_filter);
}

static FILE *fopen_check(char *filename, char *mode, struct main_window *w)
{
	FILE *f = NULL;

#ifdef _WIN32
	wchar_t *name = NULL;
	wchar_t *md = NULL;

	name = (wchar_t*)g_convert(filename, -1, "UTF-16LE", "UTF-8", NULL, NULL, NULL);
	if(!name) goto error;

	md = (wchar_t*)g_convert(mode, -1, "UTF-16LE", "UTF-8", NULL, NULL, NULL);
	if(!md) goto error;

	f = _wfopen(name, md);

error:	g_free(name);
	g_free(md);
#else
	f = fopen(filename, mode);
#endif

	if(!f) {
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),0,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,
					_("Error opening file\n"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}

	return f;
}

/* The file chosen in a save dialog, with the extension added when needed and
 * a confirmation if it exists. NULL if cancelled or unusable. Free with g_free(). */
static char *chooser_save_path(struct main_window *w, GtkFileChooser *chooser,
	const char *extension, int force_extension)
{
	GFile *gf = gtk_file_chooser_get_file(chooser);
	if(!gf) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window), 0,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Error opening file\n"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}
	char *filename = g_file_get_path(gf);
	g_object_unref(gf);
	if(!filename) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window), 0,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Error opening file\n"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}

	const char *filter_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(chooser));
	int add_extension = force_extension || (filter_name && !strcmp(extension, filter_name));
	size_t name_length = strlen(filename);
	size_t extension_length = strlen(extension);
	if(add_extension && (name_length < extension_length ||
			strcasecmp(extension, filename + name_length - extension_length))) {
		char *with_extension = g_strconcat(filename, extension, NULL);
		g_free(filename);
		filename = with_extension;
	}

	// g_file_test takes UTF-8 names on Windows too (stat() would not)
	if(g_file_test(filename, G_FILE_TEST_EXISTS)) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_OK_CANCEL,
			_("File %s already exists. Do you want to replace it?"),
			filename);
		int replace = GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(!replace) {
			g_free(filename);
			return NULL;
		}
	}
	return filename;
}

static void add_recent_file(const char *filename)
{
	char *uri = g_filename_to_uri(filename, NULL, NULL);
	if(uri)
		gtk_recent_manager_add_item(gtk_recent_manager_get_default(), uri);
	g_free(uri);
}

static FILE *save_selected_file(struct main_window *w, GtkFileChooser *chooser,
	const char *extension, int force_extension)
{
	char *filename = chooser_save_path(w, chooser, extension, force_extension);
	if(!filename) return NULL;
	FILE *f = fopen_check(filename, "wb", w);
	if(f)
		add_recent_file(filename);
	g_free(filename);
	return f;
}

static FILE *choose_file_for_save_with_options(struct main_window *w, const char *title,
	char *suggestion, const char *extension, GtkWidget *extra, GtkWidget *choice,
	int *selected_choice, int force_extension)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(title,
		GTK_WINDOW(w->window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		_("Cancel"),
		GTK_RESPONSE_CANCEL,
		_("Save"),
		GTK_RESPONSE_ACCEPT,
		NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	if(suggestion)
		gtk_file_chooser_set_current_name(chooser, suggestion);
	chooser_set_filters(chooser, extension);
	if(extra) {
		gtk_file_chooser_set_extra_widget(chooser, extra);
		gtk_widget_show_all(extra);
	}

	FILE *f = NULL;
	if(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog))) {
		if(selected_choice && choice)
			*selected_choice = gtk_combo_box_get_active(GTK_COMBO_BOX(choice));
		f = save_selected_file(w, chooser, extension, force_extension);
	}
	gtk_widget_destroy(dialog);
	return f;
}

/* A save dialog that returns the chosen path (UTF-8) instead of opening it */
static char *choose_path_for_save(struct main_window *w, const char *title,
	const char *suggestion, const char *extension)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(title,
		GTK_WINDOW(w->window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		_("Cancel"),
		GTK_RESPONSE_CANCEL,
		_("Save"),
		GTK_RESPONSE_ACCEPT,
		NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	if(w->session_path) {
		gchar *dir = g_path_get_dirname(w->session_path);
		gtk_file_chooser_set_current_folder(chooser, dir);
		g_free(dir);
	}
	if(suggestion)
		gtk_file_chooser_set_current_name(chooser, suggestion);
	chooser_set_filters(chooser, extension);

	char *path = NULL;
	if(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
		path = chooser_save_path(w, chooser, extension, 1);
	gtk_widget_destroy(dialog);
	return path;
}

/* ---- The session ----
 *
 * One document: the project metadata (with the watch label), the snapshot
 * tabs and, when saved, what the real-time tab shows ("Saved state").
 * Save writes all of it, whichever tab is shown. Opening a session replaces
 * the current one and lets it go on: new snapshots are added, while the
 * measurements loaded from the file (tab data "from-file") are kept as they
 * are. Each save writes one new "Saved state" with the current real-time
 * data; the ones loaded from the file stay, so an interrupted recording
 * keeps what was saved before. Nothing here changes the saved watches, the
 * calibration or any other setting of the configuration file. */

static int session_snapshot_count(struct main_window *w)
{
	int n = 0, pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(w->notebook));
	for(int i = 0; i < pages; i++) {
		GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), i);
		if(g_object_get_data(G_OBJECT(tab), "op-pointer"))
			n++;
	}
	return n;
}

static int text_set(const char *s)
{
	return s && *s;
}

/* The session holds something besides the live display: then new live
 * readings are changes too. Just watching the live display is not. */
static int session_in_use(struct main_window *w)
{
	const struct project_metadata *m = &w->project;
	return w->session_path || session_snapshot_count(w) ||
		text_set(m->title) || text_set(m->project_reference) || text_set(m->owner) ||
		text_set(m->last_service_date) || text_set(m->notes);
}

static int live_trend_changed(struct main_window *w)
{
	const struct trend *t = w->active_panel ? w->active_panel->trend : NULL;
	if(!t) return 0;
	return t->count != w->saved_trend_count ||
		(t->count && t->start_us != w->saved_trend_start_us);
}

static int session_is_modified(struct main_window *w)
{
	return w->session_dirty || (session_in_use(w) && live_trend_changed(w));
}

/* "Tg Expert <version> — <project> (<file>) *", * with unsaved changes */
static void session_update_title(struct main_window *w)
{
	if(w->zombie || !w->window) return;
	int dirty = session_is_modified(w);
	w->title_dirty_shown = dirty;
	const char *title = text_set(w->project.title) ? w->project.title : NULL;
	gchar *file = w->session_path ? g_path_get_basename(w->session_path) : NULL;
	GString *s = g_string_new(PROGRAM_NAME " " VERSION);
	if(title || file)
		g_string_append(s, " \xE2\x80\x94 ");
	if(title)
		g_string_append(s, title);
	if(file)
		g_string_append_printf(s, title ? " (%s)" : "%s", file);
	if(dirty)
		g_string_append(s, " *");
	gtk_window_set_title(GTK_WINDOW(w->window), s->str);
	g_string_free(s, TRUE);
	g_free(file);
}

static void session_mark_changed(struct main_window *w)
{
	if(w->session_updating) return;
	w->session_dirty = 1;
	session_update_title(w);
}

/* The session now matches its file (or is new and empty) */
static void session_mark_saved(struct main_window *w)
{
	const struct trend *t = w->active_panel ? w->active_panel->trend : NULL;
	w->session_dirty = 0;
	w->saved_trend_count = t ? t->count : 0;
	w->saved_trend_start_us = t ? t->start_us : 0;
	session_update_title(w);
}

static void session_set_watch_label(struct main_window *w, const char *name)
{
	free(w->project.watch_label);
	w->project.watch_label = name ? strdup(name) : NULL;
}

static void session_message(struct main_window *w, GtkMessageType type,
	const char *text, const char *detail)
{
	GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		type, GTK_BUTTONS_CLOSE, "%s", text);
	if(detail)
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(d), "%s", detail);
	gtk_dialog_run(GTK_DIALOG(d));
	gtk_widget_destroy(d);
}

/* What the real-time tab shows, as the "Saved state" entry, or NULL when
 * there is nothing to keep (no waveform yet, or calibrating) */
static struct snapshot *session_live_state(struct main_window *w)
{
	struct snapshot *a = w->active_snapshot;
	if(!a || a->calibrate || !a->pb) return NULL;
	struct snapshot *s = snapshot_clone(a);
	if(!s->timestamp)
		s->timestamp = get_timestamp(s->is_light);
	s->audio_status = w->active_panel->audio_status;
	s->position = w->live_position;
	s->saved_state = 1;
	free(s->notes);
	s->notes = NULL;
	return s;
}

struct session_contents {
	struct snapshot **s;
	char **names;
	struct trend **trends;
	uint64_t count;
	const struct project_metadata *meta;
};

static int session_writer(FILE *f, void *data)
{
	struct session_contents *c = data;
	return write_file(f, c->s, c->names, c->trends, c->count, c->meta);
}

/* Write the whole session to path, safely. 0 on success. */
static int session_write(struct main_window *w, const char *path)
{
	int pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(w->notebook));
	struct snapshot **s = g_new0(struct snapshot *, pages + 1);
	char **names = g_new0(char *, pages + 1);
	struct trend **trends = g_new0(struct trend *, pages + 1);
	uint64_t n = 0;
	for(int i = 0; i < pages; i++) {
		GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), i);
		struct output_panel *op = g_object_get_data(G_OBJECT(tab), "op-pointer");
		if(!op) continue; // the real-time tab, written last as "Saved state"
		s[n] = op->snst;
		trends[n] = op->trend;
		names[n++] = g_object_get_data(G_OBJECT(tab), "tab-name");
	}
	// The data is copied here, in the GUI thread, so it is coherent even
	// while the measurement goes on
	struct snapshot *live = session_live_state(w);
	if(live) {
		s[n] = live;
		trends[n] = w->active_panel->trend;
		// Older programs show this name, newer ones the role
		names[n++] = (char *)_("Saved state");
	}
	struct session_contents c = {s, names, trends, n, &w->project};
	int failed = session_write_file_safely(path, session_writer, &c);
	if(live)
		snapshot_destroy(live);
	g_free(s);
	g_free(names);
	g_free(trends);
	return failed;
}

static int session_save_to(struct main_window *w, const char *path)
{
	if(session_write(w, path)) {
		session_message(w, GTK_MESSAGE_ERROR,
			_("The session could not be saved. The previous file, if any, was not changed."),
			path);
		return 0;
	}
	if(w->session_path != path) {
		g_free(w->session_path);
		w->session_path = g_strdup(path);
	}
	add_recent_file(path);
	session_mark_saved(w);
	return 1;
}

/* "YYYY-MM-DD <project name>.tgj", or the watch label without a project name */
static char *session_suggested_name(struct main_window *w)
{
	GDateTime *now = g_date_time_new_now_local();
	char *date = now ? g_date_time_format(now, "%Y-%m-%d") : NULL;
	if(now) g_date_time_unref(now);
	const char *name = text_set(w->project.title) ? w->project.title : w->project.watch_label;
	char *s = session_suggest_filename(date ? date : "", name);
	g_free(date);
	return s;
}

static int session_save_as(struct main_window *w)
{
	char *suggestion = session_suggested_name(w);
	char *path = choose_path_for_save(w, _("Save session"), suggestion, ".tgj");
	g_free(suggestion);
	if(!path) return 0;
	int saved = session_save_to(w, path);
	g_free(path);
	return saved;
}

static int session_save(struct main_window *w)
{
	return w->session_path ? session_save_to(w, w->session_path) : session_save_as(w);
}

/* Before the session is replaced or the program closed: 1 to go on, 0 to stop */
static int session_confirm_discard(struct main_window *w)
{
	if(!session_is_modified(w)) return 1;
	GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
		"%s", _("Save the changes to the current session?"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(d), "%s",
		_("If you do not save them, they will be lost."));
	gtk_dialog_add_buttons(GTK_DIALOG(d),
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("_Discard"), GTK_RESPONSE_REJECT,
		_("_Save"), GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_ACCEPT);
	int r = gtk_dialog_run(GTK_DIALOG(d));
	gtk_widget_destroy(d);
	if(r == GTK_RESPONSE_REJECT) return 1;
	if(r == GTK_RESPONSE_ACCEPT) return session_save(w);
	return 0;
}

static void session_bar_hide(struct main_window *w)
{
	if(w->session_bar)
		gtk_widget_hide(w->session_bar);
}

/* After opening: say how the watch of the session relates to the saved
 * watches. Nothing is selected, applied, added or changed by itself. */
static void session_show_watch_notice(struct main_window *w, int have_ref, int ref_bph, double ref_la)
{
	const char *label = w->project.watch_label;
	if(!w->session_bar || !label) {
		session_bar_hide(w);
		return;
	}
	int bph;
	double la;
	gchar *text = NULL;
	int can_use = 0;
	GtkMessageType type = GTK_MESSAGE_INFO;
	if(get_watch(w, label, &bph, &la)) {
		text = g_strdup_printf(_("The watch \"%s\" of this session is not among the saved watches. Nothing was added or changed."), label);
	} else if(have_ref && (bph != ref_bph || fabs(la - ref_la) >= 0.05)) {
		gchar *now = bph ? g_strdup_printf("%d bph", bph) : g_strdup(_("bph guess"));
		gchar *then = ref_bph ? g_strdup_printf("%d bph", ref_bph) : g_strdup(_("bph guess"));
		type = GTK_MESSAGE_WARNING;
		text = g_strdup_printf(_("The saved watch \"%s\" has other settings (%s, lift angle %g\xC2\xB0) than the last measurement of this session (%s, lift angle %g\xC2\xB0). Nothing was changed."),
			label, now, la, then, ref_la);
		g_free(now);
		g_free(then);
	} else if(!g_strcmp0(selected_watch(w), label) && w->bph == bph && fabs(w->la - la) < 0.05) {
		session_bar_hide(w); // already in use
		return;
	} else {
		text = g_strdup_printf(_("This session was measured with the watch \"%s\"."), label);
		can_use = 1;
	}
	gtk_info_bar_set_message_type(GTK_INFO_BAR(w->session_bar), type);
	gtk_label_set_text(GTK_LABEL(w->session_bar_label), text);
	gtk_widget_set_visible(w->session_bar_use, can_use);
	gtk_widget_show(w->session_bar);
	g_free(text);
}

/* "Use this watch": select it and apply its bph and lift angle to the live
 * measurement, as if picked in the selector */
static void session_use_watch(struct main_window *w)
{
	const char *label = w->project.watch_label;
	int bph;
	double la;
	if(!label || get_watch(w, label, &bph, &la)) {
		session_bar_hide(w);
		return;
	}
	gtk_notebook_set_current_page(GTK_NOTEBOOK(w->notebook),
		gtk_notebook_page_num(GTK_NOTEBOOK(w->notebook), w->active_panel->panel));
	if(!w->controls_active || w->calibrate || w->calibration_stop_pending)
		return;
	watch_select(w, label);
	watch_remember(w, label);
	watch_apply(w, bph, la);
	session_bar_hide(w);
}

static void handle_session_bar_response(GtkInfoBar *bar, gint response, struct main_window *w)
{
	UNUSED(bar);
	if(response == GTK_RESPONSE_ACCEPT)
		session_use_watch(w);
	else
		session_bar_hide(w);
}

/* Close the snapshot tabs, empty the project and start the live statistics
 * over. The settings and the live measurement itself are not touched. */
static void session_clear(struct main_window *w)
{
	w->session_updating++;
	int i = 0;
	while(i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(w->notebook))) {
		GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), i);
		if(!g_object_get_data(G_OBJECT(tab), "op-pointer")) {  // the real-time tab
			i++;
			continue;
		}
		gtk_widget_destroy(tab);
	}
	w->session_updating--;
	project_metadata_free(&w->project);
	g_free(w->session_path);
	w->session_path = NULL;
	// The live readings so far belong to the previous session
	trend_reset(w->active_panel->trend);
	trend_start_settling(w->active_panel->trend, w->settling_seconds);
	op_update_settling_status(w->active_panel);
	session_bar_hide(w);
	update_export_item(w);
	gtk_widget_queue_draw(w->notebook);
}

static int session_new(struct main_window *w)
{
	if(!session_confirm_discard(w)) return 0;
	session_clear(w);
	session_set_watch_label(w, selected_watch(w));
	session_mark_saved(w);
	return 1;
}

static int session_open(struct main_window *w, const char *path)
{
	if(!session_confirm_discard(w)) return 0;
	FILE *f = fopen_check((char *)path, "rb", w);
	if(!f) return 0;
	struct snapshot **s;
	char **names;
	struct trend **trends;
	uint64_t cnt;
	struct project_metadata meta;
	int failed = read_file(f, &s, &names, &trends, &cnt, &meta);
	fclose(f);
	gchar *base = g_path_get_basename(path);
	if(failed) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),0,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,
					_("Error reading file: %s"), base);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(base);
		return 0;
	}
	// Snapshots without a name (older files) are named after the file
	size_t len = strlen(base);
	if(len > 4 && !g_ascii_strcasecmp(".tgj", base + len - 4))
		base[len - 4] = 0;

	session_clear(w);
	w->session_updating++;
	w->project = meta; // owned by the session from now on
	int have_ref = cnt > 0;
	int ref_bph = cnt ? s[cnt - 1]->bph : 0;
	double ref_la = cnt ? s[cnt - 1]->la : 0;
	for(uint64_t i = 0; i < cnt; i++) {
		struct output_panel *op = add_new_tab(s[i], names[i] ? names[i] : base, w);
		op->trend = trends[i]; // NULL for files saved without one
		g_object_set_data(G_OBJECT(op->panel), "from-file", GINT_TO_POINTER(1));
		GtkWidget *close = g_object_get_data(G_OBJECT(op->panel), "tab-close");
		if(close)
			gtk_widget_hide(close); // measurements in the file are kept
		update_tab_label(op->panel);
		free(names[i]);
	}
	free(s);
	free(names);
	free(trends);
	g_free(base);
	w->session_path = g_strdup(path);
	w->session_updating--;
	add_recent_file(path);
	update_export_item(w);
	session_mark_saved(w);
	session_show_watch_notice(w, have_ref, ref_bph, ref_la);
	return 1;
}

/* The user picked another watch in the selector (name NULL: none). With
 * measurements in the session, ask whether to start a new session or to
 * associate this one with that watch; the measurements keep their names
 * and settings either way. Returns 0 if cancelled: the selection is put
 * back to the watch of the session. */
static int session_watch_change_allowed(struct main_window *w, const char *name)
{
	const char *current = w->project.watch_label;
	if(!g_strcmp0(name, current)) return 1;
	if(!session_snapshot_count(w) && !w->session_path) {
		session_set_watch_label(w, name); // nothing measured yet
		return 1;
	}
	gchar *chosen = g_strdup(name);
	gchar *question = chosen ?
		g_strdup_printf(_("Use the watch \"%s\" for this session?"), chosen) :
		g_strdup(_("Remove the watch from this session?"));
	GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "%s", question);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(d), "%s",
		_("The session already has measurements. Start a new session for the other watch, or associate this session with it. The measurements keep their own settings and names."));
	gtk_dialog_add_buttons(GTK_DIALOG(d),
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("_New session"), 1,
		_("_Associate"), 2,
		NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CANCEL);
	int r = gtk_dialog_run(GTK_DIALOG(d));
	gtk_widget_destroy(d);
	g_free(question);

	int allowed = 0;
	if(r == 1 && session_new(w)) {
		session_set_watch_label(w, chosen);
		watch_select(w, chosen); // closing the tabs may have restored another one
		allowed = 1;
	} else if(r == 2) {
		session_set_watch_label(w, chosen);
		session_mark_changed(w);
		allowed = 1;
	}
	if(!allowed)
		watch_select(w, w->project.watch_label);
	g_free(chosen);
	return allowed;
}

static void handle_new_session(GtkMenuItem *m, struct main_window *w)
{
	UNUSED(m);
	session_new(w);
}

static void handle_save(GtkMenuItem *m, struct main_window *w)
{
	UNUSED(m);
	session_save(w);
}

static void handle_save_as(GtkMenuItem *m, struct main_window *w)
{
	UNUSED(m);
	session_save_as(w);
}

static gboolean localized_csv_available(void)
{
	const char *msgid = "Localized CSV (semicolon, decimal comma)";
	return strcmp(_(msgid), msgid) != 0;
}

static char *csv_suggested_name(struct main_window *w)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(w->notebook));
	GtkWidget *tab = page >= 0 ?
		gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), page) : NULL;
	if(!tab)
		return NULL;

	struct output_panel *op = g_object_get_data(G_OBJECT(tab), "op-pointer");
	const char *clock_name = op ? g_object_get_data(G_OBJECT(tab), "tab-name") :
		selected_watch(w);
	GDateTime *now = g_date_time_new_now_local();
	if(!now)
		return NULL;
	char *timestamp = g_date_time_format(now, "%Y-%m-%d %H-%M");
	g_date_time_unref(now);
	if(!timestamp)
		return NULL;

	char *safe_name = clock_name ? g_strdup(clock_name) : NULL;
	if(safe_name) {
		g_strstrip(safe_name);
		for(char *p = safe_name; *p; p++)
			if((guchar)*p < 0x20 || strchr("\\/:*?\"<>|", *p))
				*p = '_';
		size_t length = strlen(safe_name);
		while(length && (safe_name[length - 1] == ' ' || safe_name[length - 1] == '.'))
			safe_name[--length] = '_';
		if(!length) {
			g_free(safe_name);
			safe_name = NULL;
		}
	}

	char *suggestion = safe_name ?
		g_strdup_printf("%s %s.csv", timestamp, safe_name) :
		g_strconcat(timestamp, ".csv", NULL);
	g_free(safe_name);
	g_free(timestamp);
	return suggestion;
}

static void export_trend_csv(GtkMenuItem *m, struct main_window *w)
{
	UNUSED(m);
	struct trend *trend = current_trend(w, NULL);
	if(!trend || !trend->count)
		return;

	char *suggestion = csv_suggested_name(w);
	if(!suggestion) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window), 0,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not create the export filename."));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	GtkWidget *format_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	GtkWidget *format_label = gtk_label_new(_("CSV format"));
	gtk_box_pack_start(GTK_BOX(format_box), format_label, FALSE, FALSE, 0);
	GtkWidget *format_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(format_combo), "portable",
		_("Portable CSV (comma, decimal point)"));
	gboolean has_localized_format = localized_csv_available();
	if(has_localized_format)
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(format_combo), "localized",
			_("Localized CSV (semicolon, decimal comma)"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(format_combo),
		w->csv_format == CSV_FORMAT_LOCALIZED && has_localized_format ? 1 : 0);
	gtk_box_pack_start(GTK_BOX(format_box), format_combo, FALSE, FALSE, 0);

	int format = w->csv_format;
	FILE *f = choose_file_for_save_with_options(w, _("Export Trend as CSV"),
		suggestion, ".csv", format_box, format_combo, &format, 1);
	g_free(suggestion);
	if(format != w->csv_format) {
		w->csv_format = format;
		save_config(w);
	}
	if(!f)
		return;

	char separator, decimal;
	if(format == CSV_FORMAT_PORTABLE) {
		separator = ',';
		decimal = '.';
	} else if(format == CSV_FORMAT_LOCALIZED && has_localized_format) {
		separator = ';';
		decimal = ',';
	} else {
		fclose(f);
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window), 0,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Invalid CSV format."));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	int write_error = write_trend_csv(f, trend, separator, decimal);
	if(fclose(f))
		write_error = 1;
	if(write_error) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),0,
			GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,_("Error writing file"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

static void handle_open_session(GtkMenuItem *m, struct main_window *w)
{
	UNUSED(m);
	GtkWidget *dialog = gtk_file_chooser_dialog_new (_("Open session"),
			GTK_WINDOW(w->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			_("Cancel"),
			GTK_RESPONSE_CANCEL,
			_("Open"),
			GTK_RESPONSE_ACCEPT,
			NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);

	chooser_set_filters(chooser, ".tgj");

	if(w->session_path) {
		gchar *dir = g_path_get_dirname(w->session_path);
		gtk_file_chooser_set_current_folder(chooser, dir);
		g_free(dir);
	}

	char *filename = NULL;
	if(GTK_RESPONSE_ACCEPT == gtk_dialog_run (GTK_DIALOG (dialog)))
	{
		GFile *gf = gtk_file_chooser_get_file(chooser);
		if(gf) {
			filename = g_file_get_path(gf);
			g_object_unref(gf);
		}
	}
	gtk_widget_destroy(dialog);

	if(filename) {
		session_open(w, filename);
		g_free(filename);
	}
}

/* The top bar is three bars (watch, audio, snapshot) in a flow box: side by side
 * when the window is wide, wrapping to a second row when it is narrow. */
static GtkWidget *make_bar(struct main_window *w)
{
	GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_container_set_border_width(GTK_CONTAINER(bar), 4);

	gtk_container_add(GTK_CONTAINER(w->bar_flow), bar);
	return bar;
}

/* Compact +/- buttons for the spin buttons of the top bar (class "compact"):
 * the default ones are much taller and wider than the number they change */
static void apply_compact_css(void)
{
	static const gchar css[] =
		"spinbutton.compact button { min-width: 14px; min-height: 0px; padding: 0px 2px; }";
	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, css, -1, NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);
}

/* A top-level entry of the menu bar (label with a mnemonic) and its menu */
static GtkWidget *menu_bar_add(GtkWidget *menu_bar, const char *label)
{
	GtkWidget *item = gtk_menu_item_new_with_mnemonic(label);
	GtkWidget *menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), item);
	return menu;
}

#define PROJECT_URL "https://github.com/jefebus/tg-expert"

/* The program icon at the given size, or NULL. From the icon theme when it is
 * installed there (make install, the Windows installer); otherwise from the
 * hicolor folders of the data directories, or from the source tree in debug
 * builds started there. Free with g_object_unref(). */
static GdkPixbuf *program_icon(int size)
{
	GdkPixbuf *p = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
		ICON_NAME, size, 0, NULL);
	if(p) return p;
	gchar *rel = g_strdup_printf("%dx%d/apps/" ICON_NAME ".png", size, size);
	const gchar * const *dirs = g_get_system_data_dirs();
	for(int i = 0; !p && dirs[i]; i++) {
		gchar *path = g_build_filename(dirs[i], "icons", "hicolor", rel, NULL);
		p = gdk_pixbuf_new_from_file(path, NULL);
		g_free(path);
	}
#ifdef DEBUG
	if(!p) {
		gchar *path = g_build_filename("icons", rel, NULL);
		p = gdk_pixbuf_new_from_file(path, NULL);
		g_free(path);
	}
#endif
	g_free(rel);
	return p;
}

static void handle_about(GtkMenuItem *m, struct main_window *w)
{
	UNUSED(m);
	/* All the texts are the program's own, so that they follow its translation.
	 * The licence, authors and credits pages of GtkAboutDialog are not used:
	 * their texts come from GTK's catalogue, which may be incomplete or missing
	 * (the dialog then mixes languages). */
	gchar *comments = g_strdup_printf("%s\n\n%s",
		_("Timegrapher for mechanical watches, using a microphone and the computer's sound card."),
		_("Free software under the GNU General Public License, version 2. It comes with absolutely no warranty."));
	gchar *title = g_strdup(_("About Tg Expert"));
	// Without the icon GTK would show its "missing image" picture
	GdkPixbuf *logo = program_icon(128);
	gtk_show_about_dialog(GTK_WINDOW(w->window),
		"title", title,
		"program-name", PROGRAM_NAME,
		"version", VERSION,
		"logo", logo,
		"comments", comments,
		"copyright", "\xC2\xA9 2015 Marcello Mamino (tg)\n\xC2\xA9 2026 jefebus (tg-expert)",
		"website", PROJECT_URL,
		"website-label", _("Source code and releases on GitHub"),
		NULL);
	if(logo) g_object_unref(logo);
	g_free(title);
	g_free(comments);
}

/* Set up the main window and populate with widgets */
static void init_main_window(struct main_window *w)
{
	apply_compact_css();
	w->window = gtk_application_window_new(w->app);

	gtk_widget_set_size_request(w->window, 950, 700);

	g_signal_connect(w->window, "delete_event", G_CALLBACK(delete_event), w);
	g_signal_connect(w->window, "key-press-event", G_CALLBACK(handle_key_press), w);

	gtk_window_set_title(GTK_WINDOW(w->window), PROGRAM_NAME " " VERSION);
	gtk_window_set_icon_name (GTK_WINDOW(w->window), ICON_NAME);

	// The menu bar runs along the top edge; the rest keeps a 10 px border
	GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(w->window), outer);
	GtkWidget *menu_bar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(outer), menu_bar, FALSE, FALSE, 0);
	// Keyboard shortcuts of the menu entries, shown next to them. They only work
	// while the entry is available, so the calibration locks apply to them too.
	GtkAccelGroup *accel = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(w->window), accel);
	g_object_unref(accel);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_box_pack_start(GTK_BOX(outer), vbox, TRUE, TRUE, 0);

	// Top strip: the bars (watch, audio, snapshot)
	GtkWidget *top_strip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start(GTK_BOX(vbox), top_strip, FALSE, FALSE, 0);

	w->bar_flow = gtk_flow_box_new();
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(w->bar_flow), GTK_SELECTION_NONE);
	gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(w->bar_flow), FALSE);
	gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(w->bar_flow), 16);
	gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(w->bar_flow), 6);
	gtk_box_pack_start(GTK_BOX(top_strip), w->bar_flow, TRUE, TRUE, 0);

	// Watch bar
	GtkWidget *hbox = make_bar(w);

	// Saved watch selector (populated once bph and lift angle widgets exist)
	GtkWidget *label = NULL;
	w->watch_combo_box = gtk_combo_box_text_new();
	gtk_widget_set_tooltip_text(w->watch_combo_box, _("Saved watches (bph and lift angle)"));
	gtk_box_pack_start(GTK_BOX(hbox), w->watch_combo_box, FALSE, FALSE, 0);
	w->watch_save_button = gtk_button_new_from_icon_name("document-save-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text(w->watch_save_button, _("Save the current bph and lift angle as a watch"));
	gtk_box_pack_start(GTK_BOX(hbox), w->watch_save_button, FALSE, FALSE, 0);
	w->watch_delete_button = gtk_button_new_from_icon_name("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text(w->watch_delete_button, _("Delete the selected watch"));
	gtk_box_pack_start(GTK_BOX(hbox), w->watch_delete_button, FALSE, FALSE, 0);

	// BPH label
	label = gtk_label_new("BEAT");
	gtk_widget_set_tooltip_text(label, _("Beats per hour (bph)"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	// BPH combo box
	w->bph_combo_box = gtk_combo_box_text_new_with_entry();
	gtk_entry_set_width_chars(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(w->bph_combo_box))), 6);
	gtk_box_pack_start(GTK_BOX(hbox), w->bph_combo_box, FALSE, FALSE, 0);
	// Fill in pre-defined values
	/* TRANSLATORS: first entry of the beats-per-hour list: the program detects the beat rate by itself. Short.
	 * The program reads the entry back as a number and treats any non-numeric text as "guess". */
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w->bph_combo_box), _("guess"));
	int i,current = 0;
	for(i = 0; preset_bph[i]; i++) {
		char s[100];
		sprintf(s,"%d", preset_bph[i]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w->bph_combo_box), s);
		if(w->bph == preset_bph[i]) current = i+1;
	}
	if(current || w->bph == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(w->bph_combo_box), current);
	else {
		char s[32];
		sprintf(s,"%d",w->bph);
		GtkEntry *e = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(w->bph_combo_box)));
		gtk_entry_set_text(e,s);
	}
	g_signal_connect (w->bph_combo_box, "changed", G_CALLBACK(handle_bph_change), w);

	// Lift angle label
	label = gtk_label_new("L.A.");
	gtk_widget_set_tooltip_text(label, _("Lift angle (degrees)"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	// Lift angle spin button
	w->la_spin_button = gtk_spin_button_new_with_range(MIN_LA, MAX_LA, 1);
	gtk_entry_set_width_chars(GTK_ENTRY(w->la_spin_button), 3);
	gtk_style_context_add_class(gtk_widget_get_style_context(w->la_spin_button), "compact");
	gtk_box_pack_start(GTK_BOX(hbox), w->la_spin_button, FALSE, FALSE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->la_spin_button), w->la);
	g_signal_connect(w->la_spin_button, "value_changed", G_CALLBACK(handle_la_change), w);

	// Fill the saved watch selector and hook it up
	g_signal_connect(w->watch_combo_box, "changed", G_CALLBACK(handle_watch_change), w);
	g_signal_connect(w->watch_save_button, "clicked", G_CALLBACK(handle_watch_save), w);
	g_signal_connect(w->watch_delete_button, "clicked", G_CALLBACK(handle_watch_delete), w);
	watch_populate(w, NULL);
	watch_restore(w);

	// Audio bar
	hbox = make_bar(w);

	// Audio input in use (device, audio system, rate): opens the audio settings dialog
	w->audio_button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(w->audio_button), GTK_RELIEF_NONE);
	w->audio_button_label = gtk_label_new("");
	gtk_label_set_max_width_chars(GTK_LABEL(w->audio_button_label), 40);
	gtk_label_set_ellipsize(GTK_LABEL(w->audio_button_label), PANGO_ELLIPSIZE_END);
	gtk_container_add(GTK_CONTAINER(w->audio_button), w->audio_button_label);
	gtk_box_pack_start(GTK_BOX(hbox), w->audio_button, FALSE, FALSE, 0);
	g_signal_connect_swapped(w->audio_button, "clicked", G_CALLBACK(audio_settings_open), w);

	w->listen_button = gtk_toggle_button_new_with_label(_("Listen LIVE"));
	gtk_widget_set_tooltip_text(w->listen_button,
		_("Listen to the live input through headphones. Speakers can feed back into the microphone."));
	gtk_box_pack_start(GTK_BOX(hbox), w->listen_button, FALSE, FALSE, 0);
	g_signal_connect(w->listen_button, "toggled", G_CALLBACK(handle_listen_toggled), w);

	// Calibration label
	label = gtk_label_new("cal");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	// Calibration spin button
	w->cal_spin_button = gtk_spin_button_new_with_range(MIN_CAL, MAX_CAL, 1);
	gtk_box_pack_start(GTK_BOX(hbox), w->cal_spin_button, FALSE, FALSE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->cal_spin_button), w->cal);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(w->cal_spin_button), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(w->cal_spin_button), 5);
	gtk_style_context_add_class(gtk_widget_get_style_context(w->cal_spin_button), "compact");
	g_signal_connect(w->cal_spin_button, "value_changed", G_CALLBACK(handle_cal_change), w);
	g_signal_connect(w->cal_spin_button, "output", G_CALLBACK(output_cal), NULL);
	g_signal_connect(w->cal_spin_button, "input", G_CALLBACK(input_cal), NULL);

	// Snapshot bar
	hbox = make_bar(w);

	// Watch position: on the real-time tab it is used for the next snapshot,
	// on a snapshot tab it edits that snapshot (shown in the tab title, saved with it)
	w->snapshot_position_combo = gtk_combo_box_text_new();
	int pos;
	for(pos = POS_NONE; pos < POS_COUNT; pos++)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w->snapshot_position_combo), _(position_labels[pos]));
	gtk_combo_box_set_active(GTK_COMBO_BOX(w->snapshot_position_combo), w->live_position);
	gtk_widget_set_tooltip_text(w->snapshot_position_combo,
		_("Position of the watch.\n"
		"\n"
		"Real time: applied to\n"
		"the next snapshot.\n"
		"Snapshot: changes the\n"
		"position of this one.\n"
		"\n"
		"Shown in the tab title\n"
		"and saved with the\n"
		"snapshot."));
	gtk_box_pack_start(GTK_BOX(hbox), w->snapshot_position_combo, FALSE, FALSE, 0);
	g_signal_connect(w->snapshot_position_combo, "changed", G_CALLBACK(handle_position_change), w);

	// Snapshot button
	w->snapshot_button = gtk_button_new_with_label(_("Take Snapshot"));
	gtk_widget_set_tooltip_text(w->snapshot_button,
		_("Keep the current\n"
		"measurement in a\n"
		"new tab.\n"
		"\n"
		"Shortcut: Ctrl+Space"));
	gtk_box_pack_start(GTK_BOX(hbox), w->snapshot_button, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(w->snapshot_button, FALSE);
	g_signal_connect(w->snapshot_button, "clicked", G_CALLBACK(handle_snapshot), w);

	// Snapshot name field
	GtkWidget *name_label = gtk_label_new(_("Current snapshot:"));
	w->snapshot_name_entry = gtk_entry_new();
	w->snapshot_name = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start(GTK_BOX(w->snapshot_name), name_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(w->snapshot_name), w->snapshot_name_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), w->snapshot_name, FALSE, FALSE, 0);
	g_signal_connect(w->snapshot_name_entry, "changed", G_CALLBACK(handle_name_change), w);

	// Menu bar: File, Settings, Tools, Help
	GtkWidget *file_menu = menu_bar_add(menu_bar, _("_File"));
	GtkWidget *settings_menu = menu_bar_add(menu_bar, _("_Settings"));
	GtkWidget *tools_menu = menu_bar_add(menu_bar, _("_Tools"));
	GtkWidget *help_menu = menu_bar_add(menu_bar, _("_Help"));

	// File: the session (one document with the project, the snapshots and the
	// saved state), the data export and Quit
	w->new_session_item = gtk_menu_item_new_with_label(_("New session"));
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), w->new_session_item);
	g_signal_connect(w->new_session_item, "activate", G_CALLBACK(handle_new_session), w);
	gtk_widget_add_accelerator(w->new_session_item, "activate", accel,
		GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	w->open_item = gtk_menu_item_new_with_label(_("Open session…"));
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), w->open_item);
	g_signal_connect(w->open_item, "activate", G_CALLBACK(handle_open_session), w);
	gtk_widget_add_accelerator(w->open_item, "activate", accel,
		GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	w->save_item = gtk_menu_item_new_with_label(_("Save"));
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), w->save_item);
	g_signal_connect(w->save_item, "activate", G_CALLBACK(handle_save), w);
	gtk_widget_add_accelerator(w->save_item, "activate", accel,
		GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	w->save_as_item = gtk_menu_item_new_with_label(_("Save as…"));
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), w->save_as_item);
	g_signal_connect(w->save_as_item, "activate", G_CALLBACK(handle_save_as), w);
	gtk_widget_add_accelerator(w->save_as_item, "activate", accel,
		GDK_KEY_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);

	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());

	// The Trend of the selected tab, as CSV (TG-02)
	w->export_item = gtk_menu_item_new_with_label(_("Export data…"));
	gtk_widget_set_tooltip_text(w->export_item, _("Export the Trend of the selected tab as CSV."));
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), w->export_item);
	g_signal_connect(w->export_item, "activate", G_CALLBACK(export_trend_csv), w);
	gtk_widget_add_accelerator(w->export_item, "activate", accel,
		GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	set_menu_item_enabled(w, w->export_item, FALSE);

	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());

	GtkWidget *quit_item = gtk_menu_item_new_with_label(_("Quit"));
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
	g_signal_connect(quit_item, "activate", G_CALLBACK(handle_quit), w);
	gtk_widget_add_accelerator(quit_item, "activate", accel,
		GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	// Settings: audio system, input device and rate; algorithm parameters
	GtkWidget *audio_settings_item = gtk_menu_item_new_with_label(_("Audio settings…"));
	gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), audio_settings_item);
	g_signal_connect_swapped(audio_settings_item, "activate", G_CALLBACK(audio_settings_open), w);

	w->algorithm_item = gtk_menu_item_new_with_label(_("Algorithm configuration…"));
	gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), w->algorithm_item);
	g_signal_connect_swapped(w->algorithm_item, "activate", G_CALLBACK(algorithm_settings_open), w);

	// Tools: Start / Stop calibration (label switches with the state)
	w->cal_button = gtk_menu_item_new_with_label(_("Start calibration"));
	gtk_widget_set_tooltip_text(w->cal_button,
		_("Measures the error of\n"
		"your sound card clock,\n"
		"using a quartz watch\n"
		"ticking once per second\n"
		"as reference.\n"
		"\n"
		"Takes about 15 minutes\n"
		"with default settings.\n"
		"The result is stored\n"
		"in the cal field.")
		);
	gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), w->cal_button);
	g_signal_connect(w->cal_button, "activate", G_CALLBACK(handle_calibrate), w);

	// Help: user manual and About
	GtkWidget *manual_item = gtk_menu_item_new_with_label(_("User manual"));
	gtk_widget_set_tooltip_text(manual_item, _("Open the user manual (PDF)."));
	gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), manual_item);
	g_signal_connect(manual_item, "activate", G_CALLBACK(handle_manual), w);
	gtk_widget_add_accelerator(manual_item, "activate", accel,
		GDK_KEY_F1, 0, GTK_ACCEL_VISIBLE);

	gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), gtk_separator_menu_item_new());

	GtkWidget *about_item = gtk_menu_item_new_with_label(_("About Tg Expert"));
	gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_item);
	g_signal_connect(about_item, "activate", G_CALLBACK(handle_about), w);

	gtk_widget_show_all(menu_bar);

	// Notice about the watch of an opened session, hidden until then
	w->session_bar = gtk_info_bar_new();
	gtk_info_bar_set_show_close_button(GTK_INFO_BAR(w->session_bar), TRUE);
	w->session_bar_label = gtk_label_new("");
	gtk_label_set_line_wrap(GTK_LABEL(w->session_bar_label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(w->session_bar_label), 0);
	gtk_container_add(GTK_CONTAINER(gtk_info_bar_get_content_area(GTK_INFO_BAR(w->session_bar))),
		w->session_bar_label);
	gtk_widget_show(w->session_bar_label);
	w->session_bar_use = gtk_info_bar_add_button(GTK_INFO_BAR(w->session_bar),
		_("Use this watch"), GTK_RESPONSE_ACCEPT);
	gtk_widget_set_tooltip_text(w->session_bar_use,
		_("Selects the watch and applies its bph and lift angle to new measurements."));
	g_signal_connect(w->session_bar, "response", G_CALLBACK(handle_session_bar_response), w);
	gtk_widget_set_no_show_all(w->session_bar, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), w->session_bar, FALSE, FALSE, 0);

	// The tabs' container
	w->notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), w->notebook, TRUE, TRUE, 0);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(w->notebook), TRUE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w->notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(w->notebook), FALSE);
	g_signal_connect(w->notebook, "page-removed", G_CALLBACK(handle_tab_closed), w);
	g_signal_connect_after(w->notebook, "switch-page", G_CALLBACK(handle_tab_changed), w);

	// The main tab
	GtkWidget *tab_label = make_tab_label(NULL, NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(w->notebook), w->active_panel->panel, tab_label);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(w->notebook), w->active_panel->panel, TRUE);

	gtk_window_maximize(GTK_WINDOW(w->window));
	gtk_widget_show_all(w->window);
	gtk_widget_hide(w->snapshot_name);
	gtk_window_set_focus(GTK_WINDOW(w->window), NULL);

	// A new, empty session for the watch selected at startup
	session_set_watch_label(w, selected_watch(w));
	session_mark_saved(w);
}

guint save_on_change_timer(struct main_window *w)
{
	save_on_change(w);
	return TRUE;
}

guint refresh(struct main_window *w)
{
	if(!w->computer)
		return FALSE;
	lock_computer(w->computer);
	struct snapshot *s = w->computer->curr;
	if(s) {
		double trace_centering = w->active_snapshot->trace_centering;
		double trace_zoom = w->active_snapshot->trace_zoom;
		snapshot_destroy(w->active_snapshot);
		w->active_snapshot = s;
		w->computer->curr = NULL;
		s->trace_centering = trace_centering;
		s->trace_zoom = trace_zoom > 0 ? trace_zoom : 1.0;
		if(w->computer->clear_trace && !s->calibrate) {
			memset(s->events,0,s->events_count*sizeof(uint64_t));
			memset(s->events_tictoc,0,s->events_count*sizeof(unsigned char));
			memset(s->amps,0,s->amps_count*sizeof(*s->amps));
			memset(s->amps_time,0,s->amps_count*sizeof(*s->amps_time));
		}
		if(s->calibrate && s->cal_state == 1 && s->cal_result != w->cal) {
			w->cal = s->cal_result;
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->cal_spin_button), s->cal_result);
		}
	}
	unlock_computer(w->computer);
	refresh_results(w);
	trend_update(w->active_panel->trend, w->active_snapshot, w->live_position);
	update_export_item(w);
	op_set_snapshot(w->active_panel, w->active_snapshot);

	int p = gtk_notebook_get_current_page(GTK_NOTEBOOK(w->notebook));
	GtkWidget *panel = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), p);
	int photogenic = 0;
	if(!g_object_get_data(G_OBJECT(panel), "op-pointer")) {
		photogenic = !w->active_snapshot->calibrate && w->active_snapshot->pb;
		gtk_widget_queue_draw(w->notebook);
	}
	gtk_widget_set_sensitive(w->snapshot_button, photogenic);
	// New live readings are changes of a session in use (shown with * in the title)
	if(session_is_modified(w) != w->title_dirty_shown)
		session_update_title(w);

	if(w->calibration_stop_pending && !w->active_snapshot->calibrate) {
		w->calibration_stop_pending = 0;
		if(w->active_snapshot->cal_state == 1) {
			apply_calibration_value(w, w->active_snapshot->cal_result, FALSE);
			save_on_change(w);
		} else if(w->active_snapshot->cal_state == 0 &&
				w->active_snapshot->cal_estimate_available &&
				confirm_provisional_calibration(w, w->active_snapshot)) {
			apply_calibration_value(w, w->active_snapshot->cal_estimate_result, TRUE);
		}
		update_sensitivity(w);
	}
	return FALSE;
}

static void computer_callback(void *w)
{
	gdk_threads_add_idle((GSourceFunc)refresh,w);
}

static void start_interface(GApplication* app, void *p)
{
	UNUSED(p);
	double real_sr;

	initialize_palette();

	struct main_window *w = malloc(sizeof(struct main_window));
	w->app = GTK_APPLICATION(app);

	w->zombie = 0;
	w->controls_active = 1;
	w->cal = MIN_CAL - 1;
	w->bph = 0;
	w->la = DEFAULT_LA;
	w->calibrate = 0;
	w->calibration_stop_pending = 0;
	w->is_light = 0;
	w->paperstrip_tick_seconds = 10.0;
	w->paperstrip_window_seconds = 60.0;
	w->settling_seconds = 0;
	w->csv_format = CSV_FORMAT_PORTABLE;
	w->nominal_sr = PA_SAMPLE_RATE;
	w->audio_device = AUDIO_DEVICE_DEFAULT;
	w->restart_audio = 0;
	w->restart_algorithm = 0;
	w->pending_audio_device = AUDIO_DEVICE_DEFAULT;
	w->pending_audio_rate = PA_SAMPLE_RATE;
	w->audio_api = AUDIO_API_DEFAULT;
	w->pending_audio_api = AUDIO_API_DEFAULT;
	w->audio_device_name = NULL;
	w->audio_output_device_name = NULL;
	w->listen_volume = 0.15;
	w->listen_active = 0;
	w->listen_updating = 0;
	w->listen_warning_shown = 0;
	w->audio_message = NULL;
	w->audio_ok = 0;
	w->audio_button = NULL;
	w->audio_button_label = NULL;
	w->listen_button = NULL;
	w->audio_settings = NULL;
	w->watch_updating = 0;
	w->position_updating = 0;
	w->live_position = POS_NONE;
	w->window = NULL;
	w->new_session_item = NULL;
	w->save_as_item = NULL;
	w->session_bar = NULL;
	w->session_bar_label = NULL;
	w->session_bar_use = NULL;
	project_metadata_init(&w->project);
	w->session_path = NULL;
	w->session_dirty = 0;
	w->session_updating = 0;
	w->saved_trend_count = 0;
	w->saved_trend_start_us = 0;
	w->title_dirty_shown = -1;

	load_config(w);
	set_audio_output_device_name(w->audio_output_device_name);
	algorithm_settings_load(w);
	if(!isfinite(w->listen_volume) || w->listen_volume < 0 || w->listen_volume > 0.5)
		w->listen_volume = 0.15;
	set_audio_monitor_volume(w->listen_volume);
	if(w->settling_seconds < 0 || w->settling_seconds > TREND_MAX_SETTLING_S)
		w->settling_seconds = 0;
	if(w->csv_format != CSV_FORMAT_PORTABLE && w->csv_format != CSV_FORMAT_LOCALIZED)
		w->csv_format = CSV_FORMAT_PORTABLE;
	if(!isfinite(w->paperstrip_tick_seconds) || w->paperstrip_tick_seconds < 0.01 ||
		w->paperstrip_tick_seconds > 3600.0)
		w->paperstrip_tick_seconds = 10.0;
	if(!isfinite(w->paperstrip_window_seconds) || w->paperstrip_window_seconds < 10.0 ||
		w->paperstrip_window_seconds > 86400.0)
		w->paperstrip_window_seconds = 60.0;
	/* Only use rates the analysis buffers can hold (a saved 96 kHz would overflow them) */
	if(!sample_rate_valid(w->nominal_sr))
		w->nominal_sr = PA_SAMPLE_RATE;
	w->pending_audio_api = w->audio_api;
	set_audio_api(w->audio_api);

	/* Reopen the saved input by name, since device numbers change between
	 * sessions: first at the saved rate, then at the default rate. If that
	 * fails, do the same with the default input. */
	int saved_rate = w->nominal_sr;
	int have_name = w->audio_device_name && *w->audio_device_name;
	int audio_failed = 1;
	set_audio_errors_quiet(1);
	for(int attempt = 0; attempt < 4 && audio_failed; attempt++) {
		int use_name = attempt < 2;
		int rate = attempt % 2 ? (int)PA_SAMPLE_RATE : saved_rate;
		if(use_name && !have_name) continue;
		if(attempt % 2 && saved_rate == PA_SAMPLE_RATE) continue;
		set_audio_input_device_name(use_name ? w->audio_device_name : NULL);
		audio_failed = open_audio(AUDIO_DEVICE_DEFAULT, rate, &real_sr);
	}
	set_audio_input_device_name(NULL);
	if(!w->audio_output_device_name && get_audio_output_device() >= 0) {
		set_audio_output_device(get_audio_output_device());
		w->audio_output_device_name = g_strdup(get_audio_output_device_name());
	}
	set_audio_errors_quiet(0);

	w->audio_ok = !audio_failed;
	if(audio_failed) {
		/* PortAudio stays initialized so that devices can still be listed */
		w->audio_device = AUDIO_DEVICE_DEFAULT;
		w->nominal_sr = PA_SAMPLE_RATE;
		real_sr = w->nominal_sr;
	} else {
		w->audio_device = get_audio_input_device();
		w->nominal_sr = get_audio_sample_rate();
		/* The saved name is kept, so the input is used again once it is back */
		const char *name = get_audio_input_device_name();
		if(have_name && (!name || strcmp(name, w->audio_device_name)))
			w->audio_message = _("The saved input was not found; using the default input.");
		else if(w->nominal_sr != saved_rate)
			w->audio_message = _("The saved rate is not available; using the default rate.");
	}
	w->pending_audio_device = w->audio_device;
	w->pending_audio_rate = w->nominal_sr;

	if(w->la < MIN_LA || w->la > MAX_LA) w->la = DEFAULT_LA;
	if(w->bph < MIN_BPH || w->bph > MAX_BPH) w->bph = 0;
	if(w->cal < MIN_CAL || w->cal > MAX_CAL)
		w->cal = (real_sr - w->nominal_sr) * (3600*24) / w->nominal_sr;

	w->computer_timeout = 0;

	w->computer = start_computer(w->nominal_sr, w->bph, w->la, w->cal, w->is_light);
	if(!w->computer) {
		error(_("Error starting computation thread"));
		g_application_quit(app);
		return;
	}
	w->computer->callback = computer_callback;
	w->computer->callback_data = w;

	w->active_snapshot = w->computer->curr;
	w->computer->curr = NULL;
	compute_results(w->active_snapshot);

	w->active_panel = init_output_panel(w->computer, w->active_snapshot, 0);
	op_set_paperstrip_settings(w->active_panel, w->paperstrip_tick_seconds,
		w->paperstrip_window_seconds);
	op_set_paperstrip_settings_callback(w->active_panel, handle_paperstrip_settings_changed, w);
	op_set_settling_setting(w->active_panel, w->settling_seconds,
		handle_settling_setting_changed, w);
	trend_start_settling(w->active_panel->trend, w->settling_seconds);

	init_main_window(w);
	audio_settings_update(w);
#ifdef DEBUG
	if(!testing)
#endif
	if(audio_failed)
		startup_audio_warning(w);

	w->kick_timeout = g_timeout_add_full(G_PRIORITY_LOW,100,(GSourceFunc)kick_computer,w,NULL);
	w->save_timeout = g_timeout_add_full(G_PRIORITY_LOW,10000,(GSourceFunc)save_on_change_timer,w,NULL);
#ifdef DEBUG
	if(testing)
		g_timeout_add_full(G_PRIORITY_LOW,3000,(GSourceFunc)quit,w,NULL);
#endif

	g_object_set_data(G_OBJECT(app), "main-window", w);
}

static void handle_activate(GApplication* app, void *p)
{
	UNUSED(p);
	struct main_window *w = g_object_get_data(G_OBJECT(app), "main-window");
	if(w) gtk_window_present(GTK_WINDOW(w->window));
}

static void handle_open(GApplication* app, GFile **files, int cnt, char *hint, void *p)
{
	UNUSED(hint);
	UNUSED(p);
	struct main_window *w = g_object_get_data(G_OBJECT(app), "main-window");
	if(w && cnt > 0) {
		gtk_window_present(GTK_WINDOW(w->window));
		// A session is one file: the first one given replaces the current session
		char *path = g_file_get_path(files[0]);
		if(path) {
			if(session_open(w, path))
				gtk_notebook_set_current_page(GTK_NOTEBOOK(w->notebook), -1);
			g_free(path);
		}
		if(cnt > 1)
			session_message(w, GTK_MESSAGE_INFO,
				_("Only the first file was opened: a session is one file."), NULL);
	}
}

/* Folder with translations for the user's languages, or NULL. Debug builds
 * prefer po/locale, followed by LOCALEDIR and the system data folders. */
static gchar *find_locale_dir(void)
{
	const gchar * const *langs = g_get_language_names();
	const gchar * const *data_dirs = g_get_system_data_dirs();
	int n = 0;
	while(data_dirs[n]) n++;
	gchar **dirs = g_new0(gchar *, n + 3);
	int k = 0;
#ifdef DEBUG
	dirs[k++] = g_strdup("po/locale");	// built by make in the source tree
#endif
	dirs[k++] = g_strdup(LOCALEDIR);
	for(int i = 0; i < n; i++)
		dirs[k++] = g_build_filename(data_dirs[i], "locale", NULL);
	gchar *found = NULL;
	for(int d = 0; d < k && !found; d++) {
		if(!dirs[d][0]) continue;
		for(const gchar * const *l = langs; *l && !found; l++) {
			gchar *mo = g_strconcat(PACKAGE, ".mo", NULL);
			gchar *path = g_build_filename(dirs[d], *l, "LC_MESSAGES", mo, NULL);
			if(g_file_test(path, G_FILE_TEST_IS_REGULAR))
				found = g_strdup(dirs[d]);
			g_free(path);
			g_free(mo);
		}
	}
	g_strfreev(dirs);
	return found;
}

/* Interface text in the system language. Only the message category follows
 * the system: numbers keep the C locale, so the decimal separator is always
 * '.', on screen and in the .tgj files, which are parsed with scanf. That is
 * also why GTK is told not to set the locale itself. */
static void setup_translations(void)
{
	gtk_disable_setlocale();
	setlocale(LC_MESSAGES, "");
	gchar *dir = find_locale_dir();
	if(dir) {
		bindtextdomain(PACKAGE, dir);
		bind_textdomain_codeset(PACKAGE, "UTF-8");
		textdomain(PACKAGE);
		debug("Translations from %s\n", dir);
		g_free(dir);
	}
}

int main(int argc, char **argv)
{
	setup_translations();

#ifdef DEBUG
	if(argc > 1 && !strcmp("test",argv[1])) {
		testing = 1;
		argv++; argc--;
	}
#endif

	GtkApplication *app = gtk_application_new ("li.ciovil.tg", G_APPLICATION_HANDLES_OPEN);
	g_signal_connect (app, "startup", G_CALLBACK (start_interface), NULL);
	g_signal_connect (app, "activate", G_CALLBACK (handle_activate), NULL);
	g_signal_connect (app, "open", G_CALLBACK (handle_open), NULL);
	g_signal_connect (app, "shutdown", G_CALLBACK (on_shutdown), NULL);
	int ret = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);

	debug("Interface exited with status %d\n",ret);

	return ret;
}
