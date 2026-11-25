/*
    tg-expert
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

/* TG-16: File > Export report. The contents are chosen in a tree of check
 * boxes, run by run as the tabs show them (the newest first): for each run
 * its Trends, its final state and its snapshots; then the current state, the
 * snapshots of older files without a run, the table of the runs, colour or
 * monochrome and whether the owner is included. When Export is pressed the
 * data is copied at once, so that the report shows one moment even while
 * the measurement goes on; then the file is chosen and written. The session
 * and its notes are not changed. */

#include "tg.h"

/* A run in the dialog, with the check boxes of its parts */
struct dialog_run {
	int number;
	const char *name;	// given by the user (the tab's), or NULL
	int in_progress;	// the run being recorded (it has no tab yet)
	GtkWidget *tab;		// the run's tab, NULL for the run in progress
	GtkWidget *check, *trends, *state;
	GtkWidget *children;	// box with the parts, sensitive while the run is checked
	GtkWidget **snaps, **snap_tabs;	// newest first, as the tabs
	int snap_count;
};

static const char *tab_title(GtkWidget *tab)
{
	GtkLabel *label = g_object_get_data(G_OBJECT(tab), "tab-label");
	return label ? gtk_label_get_text(label) : _("Snapshot");
}

static void toggle_sensitive(GtkToggleButton *b, GtkWidget *target)
{
	gtk_widget_set_sensitive(target, gtk_toggle_button_get_active(b));
}

/* Checked, and not inside an unchecked run or group */
static int chosen(GtkWidget *c)
{
	return c && gtk_widget_is_sensitive(c) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c));
}

static void message(struct main_window *w, GtkMessageType type, const char *text, const char *detail)
{
	GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_CLOSE, "%s", text);
	if(detail)
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(d), "%s", detail);
	gtk_dialog_run(GTK_DIALOG(d));
	gtk_widget_destroy(d);
}

static GtkWidget *section_label(const char *text)
{
	GtkWidget *l = gtk_label_new(NULL);
	char *markup = g_markup_printf_escaped("<b>%s</b>", text);
	gtk_label_set_markup(GTK_LABEL(l), markup);
	g_free(markup);
	gtk_label_set_xalign(GTK_LABEL(l), 0);
	return l;
}

static GtkWidget *check(GtkWidget *box, const char *label, int active, int available,
	const char *unavailable)
{
	GtkWidget *c = gtk_check_button_new_with_label(label);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c), active && available);
	if(!available) {
		gtk_widget_set_sensitive(c, FALSE);
		if(unavailable)
			gtk_widget_set_tooltip_text(c, unavailable);
	}
	gtk_box_pack_start(GTK_BOX(box), c, FALSE, FALSE, 0);
	return c;
}

static struct report_measurement measurement_copy(const struct snapshot *s, const char *title)
{
	struct report_measurement m;
	m.snapshot = snapshot_clone((struct snapshot *)s);
	m.title = g_strdup(title);
	m.negative_span = waveform_negative_span(m.snapshot);
	return m;
}

/* "Run 2", or "Run 3 (in progress)", as in the notes panel. g_free(). */
static char *dialog_run_name(const struct dialog_run *r)
{
	char *name = r->number > 0 ? g_strdup_printf(_("Run %d"), r->number) : g_strdup(_("Run"));
	if(r->name && *r->name) {
		char *t = g_strdup_printf("%s %s", name, r->name);
		g_free(name);
		name = t;
	}
	if(!r->in_progress) return name;
	char *s = g_strdup_printf(_("%s (in progress)"), name);
	g_free(name);
	return s;
}

/* The run's part of the tree: its check box, and indented under it the
 * Trends, the final state and its snapshots */
static void add_run(GtkWidget *tree, struct dialog_run *r, int have_trend, int have_state)
{
	char *name = dialog_run_name(r);
	r->check = check(tree, name, 1, 1, NULL);
	g_free(name);
	r->children = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_widget_set_margin_start(r->children, 24);
	gtk_box_pack_start(GTK_BOX(tree), r->children, FALSE, FALSE, 0);
	g_signal_connect(r->check, "toggled", G_CALLBACK(toggle_sensitive), r->children);
	/* TRANSLATORS: Export report dialog: the Trend page contents of a run */
	r->trends = check(r->children, _("Trends: statistics, graphs and correlations"), 1, have_trend,
		_("This run has no Trend readings."));
	/* TRANSLATORS: Export report dialog: the real-time tab at the end of a
	 * run. Checked by default only for runs without snapshots */
	r->state = check(r->children, r->in_progress ? _("State when exporting") : _("Final state"),
		r->snap_count == 0, have_state, _("There is no measurement with a waveform in this run."));
	for(int k = 0; k < r->snap_count; k++)
		r->snaps[k] = check(r->children, tab_title(r->snap_tabs[k]), 1, 1, NULL);
}

static int snapshot_order(const void *pa, const void *pb)
{
	const struct report_measurement *a = pa, *b = pb;
	if(a->snapshot->snap != b->snapshot->snap)
		return a->snapshot->snap < b->snapshot->snap ? -1 : 1;
	if(a->snapshot->wall_time != b->snapshot->wall_time)
		return a->snapshot->wall_time < b->snapshot->wall_time ? -1 : 1;
	return 0;
}

static int run_order(const void *pa, const void *pb)
{
	const struct report_run *a = pa, *b = pb;
	return a->number - b->number;	// chronological
}

/* The chosen parts of a run, copied */
static void copy_run(struct main_window *w, const struct dialog_run *dr, struct report_run *run,
	int64_t now)
{
	memset(run, 0, sizeof(*run));
	run->number = dr->number;
	run->name = dr->name && *dr->name ? g_strdup(dr->name) : NULL;
	run->in_progress = dr->in_progress;
	run->snapshots_taken = dr->snap_count;
	char *name = dialog_run_name(dr);
	/* TRANSLATORS: PDF report: heading of the last state of a run; %s is
	 * "Run 2" or "Run 3 (in progress)" */
	char *title = g_strdup_printf(dr->in_progress ? _("%s: state when exporting") : _("%s: final state"), name);
	g_free(name);
	// The state is copied even when it is not shown: it has the run's notes
	struct snapshot *state = NULL;
	const struct trend *trend = NULL;
	if(dr->in_progress) {
		state = session_current_run(w);	// already a copy
		if(state)
			run->state = (struct report_measurement){state, g_strdup(title), waveform_negative_span(state)};
		run->start = w->run_start;
		run->end = now;
		trend = w->active_panel->trend;
	} else {
		struct output_panel *op = g_object_get_data(G_OBJECT(dr->tab), "op-pointer");
		if(op) {
			run->state = measurement_copy(op->snst, title);
			run->start = op->snst->run_start;
			run->end = op->snst->wall_time;
			trend = op->trend;
		}
	}
	g_free(title);
	if(trend && trend->count)
		run->trend = trend_clone(trend);
	run->show_trends = chosen(dr->trends) && run->trend;
	run->show_state = chosen(dr->state) && run->state.snapshot;
	run->snapshots = g_new0(struct report_measurement, dr->snap_count + 1);
	for(int k = 0; k < dr->snap_count; k++) {
		if(!chosen(dr->snaps[k])) continue;
		struct output_panel *op = g_object_get_data(G_OBJECT(dr->snap_tabs[k]), "op-pointer");
		if(op)
			run->snapshots[run->snapshot_count++] = measurement_copy(op->snst, tab_title(dr->snap_tabs[k]));
	}
	qsort(run->snapshots, run->snapshot_count, sizeof(*run->snapshots), snapshot_order);
}

void report_dialog_open(struct main_window *w)
{
	int pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(w->notebook));

	// The runs, as the tabs show them: the one being recorded, then the
	// closed ones (the tabs are already the newest first)
	struct dialog_run *runs = g_new0(struct dialog_run, pages + 2);
	int run_count = 0;
	if(w->recording) {
		runs[run_count].number = w->run_number;
		runs[run_count++].in_progress = 1;
	}
	for(int i = 0; i < pages; i++) {
		GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), i);
		struct output_panel *op = g_object_get_data(G_OBJECT(tab), "op-pointer");
		if(!op || !op->snst->saved_state) continue;
		runs[run_count].number = op->snst->run;
		runs[run_count].name = g_object_get_data(G_OBJECT(tab), "tab-name");
		runs[run_count++].tab = tab;
	}
	// Each snapshot under its run; without one (older files), in a group of its own
	GtkWidget **other_tabs = g_new0(GtkWidget *, pages + 1);
	GtkWidget **others = g_new0(GtkWidget *, pages + 1);
	int other_count = 0;
	for(int k = 0; k < run_count; k++) {
		runs[k].snaps = g_new0(GtkWidget *, pages + 1);
		runs[k].snap_tabs = g_new0(GtkWidget *, pages + 1);
	}
	for(int i = 0; i < pages; i++) {
		GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w->notebook), i);
		struct output_panel *op = g_object_get_data(G_OBJECT(tab), "op-pointer");
		if(!op || op->snst->saved_state) continue;
		int k;
		for(k = 0; k < run_count; k++)
			if(op->snst->run > 0 && runs[k].number == op->snst->run) break;
		if(k < run_count)
			runs[k].snap_tabs[runs[k].snap_count++] = tab;
		else
			other_tabs[other_count++] = tab;
	}

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Export report"), GTK_WINDOW(w->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		/* TRANSLATORS: button of the Export report dialog */
		_("_Export"), GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_set_border_width(GTK_CONTAINER(area), 12);
	gtk_box_set_spacing(GTK_BOX(area), 6);

	/* TRANSLATORS: Export report dialog: the parts of the report */
	gtk_box_pack_start(GTK_BOX(area), section_label(_("Contents")), FALSE, FALSE, 0);
	GtkWidget *tree = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(scroll), 340);
	gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scroll), TRUE);
	gtk_container_add(GTK_CONTAINER(scroll), tree);
	gtk_box_pack_start(GTK_BOX(area), scroll, TRUE, TRUE, 0);

	for(int k = 0; k < run_count; k++) {
		struct dialog_run *r = &runs[k];
		int have_trend, have_state;
		if(r->in_progress) {
			have_trend = w->active_panel->trend && w->active_panel->trend->count;
			struct snapshot *s = session_current_run(w);
			have_state = s != NULL;
			if(s) snapshot_destroy(s);
		} else {
			struct output_panel *op = g_object_get_data(G_OBJECT(r->tab), "op-pointer");
			have_trend = op->trend && op->trend->count;
			have_state = 1;
		}
		add_run(tree, r, have_trend, have_state);
	}
	if(!run_count) {
		GtkWidget *none = gtk_label_new(_("There are no runs in the session."));
		gtk_label_set_xalign(GTK_LABEL(none), 0);
		gtk_widget_set_sensitive(none, FALSE);
		gtk_box_pack_start(GTK_BOX(tree), none, FALSE, FALSE, 0);
	}

	// Current state: the real-time tab, when it has a measurement. Only
	// checked here: the live snapshot itself is taken after the dialog, since
	// refresh() replaces (and frees) it while the dialog is open.
	const struct snapshot *live = w->active_snapshot;
	int current_ok = live && live->pb && !live->calibrate;
	live = NULL;
	/* TRANSLATORS: Export report dialog; unchecked by default */
	GtkWidget *current = check(tree, _("Current state: the real-time tab, frozen when exporting"), 0,
		current_ok, _("Not available: there is no valid measurement on the real-time tab, or it is calibrating."));
	gtk_widget_set_margin_top(current, 6);

	GtkWidget *other_group = NULL;
	if(other_count) {
		/* TRANSLATORS: Export report dialog: snapshots of files written
		 * before runs, which do not say in which run they were taken */
		other_group = check(tree, _("Other snapshots (older files)"), 1, 1, NULL);
		gtk_widget_set_margin_top(other_group, 6);
		GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
		gtk_widget_set_margin_start(box, 24);
		gtk_box_pack_start(GTK_BOX(tree), box, FALSE, FALSE, 0);
		g_signal_connect(other_group, "toggled", G_CALLBACK(toggle_sensitive), box);
		for(int i = 0; i < other_count; i++)
			others[i] = check(box, tab_title(other_tabs[i]), 1, 1, NULL);
	}

	/* TRANSLATORS: Export report dialog: colour, owner */
	gtk_box_pack_start(GTK_BOX(area), section_label(_("Options")), FALSE, FALSE, 6);
	/* TRANSLATORS: Export report dialog: the table of the runs on the first page */
	GtkWidget *summary = check(area, _("Table of the runs on the first page"), 1, run_count > 0,
		_("There are no runs in the session."));
	GtkWidget *colour_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	/* TRANSLATORS: Export report dialog: graphs in colour */
	GtkWidget *colour = gtk_radio_button_new_with_label(NULL, _("Colour"));
	/* TRANSLATORS: Export report dialog: graphs in black and grey, for printing */
	GtkWidget *mono = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(colour),
		_("Monochrome"));
	gtk_box_pack_start(GTK_BOX(colour_box), colour, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(colour_box), mono, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(area), colour_box, FALSE, FALSE, 0);
	/* TRANSLATORS: Export report dialog; unchecked by default */
	GtkWidget *owner = gtk_check_button_new_with_label(_("Include the owner"));
	gtk_widget_set_sensitive(owner, w->project.owner && *w->project.owner);
	gtk_box_pack_start(GTK_BOX(area), owner, FALSE, FALSE, 0);

	gtk_widget_show_all(area);
	int response = gtk_dialog_run(GTK_DIALOG(dialog));

	// The copy, at once: the report shows this moment
	struct report_data d;
	memset(&d, 0, sizeof(d));
	if(response == GTK_RESPONSE_ACCEPT) {
		project_metadata_copy(&d.project, &w->project);
		d.include_owner = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(owner));
		d.theme = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mono)) ?
			RENDER_DOC_MONO : RENDER_DOC_COLOR;
		GDateTime *now = g_date_time_new_now_local();
		d.date = now ? g_date_time_format(now, "%Y-%m-%d %H:%M") : g_strdup("");
		if(now) g_date_time_unref(now);
		d.paperstrip_tick_seconds = w->paperstrip_tick_seconds;
		d.paperstrip_window_seconds = w->paperstrip_window_seconds;
		d.summary = chosen(summary);
		// The program icon for the page header, in greys for a monochrome report
		d.logo = program_icon(256);
		if(d.logo && d.theme == RENDER_DOC_MONO) {
			GdkPixbuf *grey = gdk_pixbuf_copy(d.logo);
			if(grey) {
				gdk_pixbuf_saturate_and_pixelate(d.logo, grey, 0, FALSE);
				g_object_unref(d.logo);
				d.logo = grey;
			}
		}

		// The chosen runs, in the order recorded
		int64_t now_s = g_get_real_time() / G_USEC_PER_SEC;
		d.runs = g_new0(struct report_run, run_count + 1);
		for(int k = 0; k < run_count; k++)
			if(chosen(runs[k].check))
				copy_run(w, &runs[k], &d.runs[d.run_count++], now_s);
		qsort(d.runs, d.run_count, sizeof(*d.runs), run_order);

		// The snapshot shown now; it may have become unusable meanwhile
		live = w->active_snapshot;
		if(current_ok && chosen(current) && live && live->pb && !live->calibrate) {
			d.current = measurement_copy(live, _("Current state"));
			struct snapshot *c = d.current.snapshot;
			if(!c->timestamp)
				c->timestamp = get_timestamp(c->is_light);
			c->wall_time = now_s;
			c->audio_status = w->active_panel->audio_status;
			c->position = w->live_position;
			free(c->notes);
			c->notes = NULL;
		}
		d.others = g_new0(struct report_measurement, other_count + 1);
		for(int i = 0; i < other_count; i++) {
			if(!chosen(others[i])) continue;
			struct output_panel *op = g_object_get_data(G_OBJECT(other_tabs[i]), "op-pointer");
			if(op)
				d.others[d.other_count++] = measurement_copy(op->snst, tab_title(other_tabs[i]));
		}
	}
	gtk_widget_destroy(dialog);
	for(int k = 0; k < run_count; k++) {
		g_free(runs[k].snaps);
		g_free(runs[k].snap_tabs);
	}
	g_free(runs);
	g_free(others);
	g_free(other_tabs);
	if(response != GTK_RESPONSE_ACCEPT)
		return;

	// "YYYY-MM-DD project.pdf", or the watch name without a project name
	char day[16];
	snprintf(day, sizeof(day), "%.10s", d.date);
	const char *name = d.project.title && *d.project.title ? d.project.title : d.project.watch_label;
	char *suggestion = session_suggest_filename_ext(day, name, ".pdf");
	char *path = choose_path_for_save(w, _("Export report"), suggestion, ".pdf");
	g_free(suggestion);
	if(path) {
		GdkWindow *gw = gtk_widget_get_window(w->window);
		GdkCursor *busy = gdk_cursor_new_from_name(gtk_widget_get_display(w->window), "wait");
		if(gw && busy) {
			gdk_window_set_cursor(gw, busy);
			gdk_display_flush(gtk_widget_get_display(w->window));
		}
		int result = report_write_pdf(path, &d);
		if(gw) gdk_window_set_cursor(gw, NULL);
		if(busy) g_object_unref(busy);
		if(result == REPORT_ERROR_NO_PDF)
			message(w, GTK_MESSAGE_ERROR,
				_("This build cannot write PDF files: the Cairo PDF backend is not available."), NULL);
		else if(result)
			message(w, GTK_MESSAGE_ERROR,
				_("The report could not be written. The previous file, if any, was not changed."), path);
		else {
			char *uri = g_filename_to_uri(path, NULL, NULL);
			if(uri)
				gtk_recent_manager_add_item(gtk_recent_manager_get_default(), uri);
			g_free(uri);
		}
		g_free(path);
	}
	report_data_free(&d);
}
