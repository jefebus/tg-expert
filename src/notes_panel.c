/*
    tg
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

/* TG-29: the project and notes panel, floating over the right part of the
 * tabs (hidden by default, shown from Tools, resizable), so that the graphs
 * stay visible while writing. Two pages:
 *  - Project: name, job reference, owner, last-service date (foldable), the
 *    watch of the session (shown, not edited: it is the one chosen in the
 *    saved watches list) and the general notes;
 *  - Measurement: the notes of the run or snapshot shown in the selected
 *    tab; on the real-time tab, those of the run being recorded.
 * Editing changes the session at once (no Apply). Programmatic updates of
 * the widgets never change the session. */

#include "tg.h"

#define CONFIG_KEY_VISIBLE "notes_panel"
#define CONFIG_KEY_WIDTH "notes_panel_width"
#define PANEL_MIN_WIDTH 240	// px: below this the fields are not usable
#define PANEL_DEFAULT_WIDTH 340	// px: until the user resizes it
#define PANEL_UNCOVERED 120	// px of the tabs always left uncovered at the left

enum project_entry {
	ENTRY_TITLE,
	ENTRY_REFERENCE,
	ENTRY_OWNER,
	ENTRY_DATE,
	ENTRY_COUNT
};

struct notes_panel {
	struct main_window *w;
	GtkWidget *box;
	GtkWidget *stack;
	GtkWidget *entries[ENTRY_COUNT];
	GtkWidget *watch_label;
	GtkWidget *project_view;
	GtkTextBuffer *project_buffer;
	GtkWidget *measurement_stack;	// "none" or "notes"
	GtkWidget *none_label;		// why there are no notes to write
	GtkWidget *measurement_title;
	GtkWidget *measurement_view;
	GtkTextBuffer *measurement_buffer;
	GtkWidget *toggle;	// check menu item, or NULL
	char **target;		// the notes shown (of a run or snapshot), NULL if none
	int updating;	// the widgets are being filled from the session
	int width;	// px, chosen by dragging the grip at the left edge
	int dragging;
	double drag_x;	// pointer x (root) and width when the drag started
	int drag_width;
};

static char **entry_field(struct notes_panel *np, int i)
{
	struct project_metadata *m = &np->w->project;
	switch(i) {
		case ENTRY_TITLE: return &m->title;
		case ENTRY_REFERENCE: return &m->project_reference;
		case ENTRY_OWNER: return &m->owner;
		default: return &m->last_service_date;
	}
}

/* Replace *field with text (NULL or "" = not set). 1 if it changed. */
static int set_text_field(char **field, const char *text)
{
	if(!text || !*text) text = NULL;
	if(!g_strcmp0(*field, text)) return 0;
	char *copy = text ? strdup(text) : NULL;
	if(text && !copy) return 0;
	free(*field);
	*field = copy;
	return 1;
}

static void show_date_validity(GtkWidget *entry, int valid)
{
	GtkStyleContext *style = gtk_widget_get_style_context(entry);
	if(valid) {
		gtk_style_context_remove_class(style, "error");
		gtk_widget_set_tooltip_text(entry,
			_("Date of the last service: a year, a month or a day (YYYY, YYYY-MM or YYYY-MM-DD). It is kept as typed."));
	} else {
		gtk_style_context_add_class(style, "error");
		gtk_widget_set_tooltip_text(entry,
			_("Not a valid date. Use YYYY, YYYY-MM or YYYY-MM-DD; it is not saved until it is valid."));
	}
}

static void handle_entry_changed(GtkEntry *e, struct notes_panel *np)
{
	if(np->updating) return;
	int i = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(e), "project-entry"));
	const char *text = gtk_entry_get_text(e);
	if(i == ENTRY_DATE) {
		int valid = project_date_valid(text);
		show_date_validity(GTK_WIDGET(e), valid);
		if(!valid) return; // the session keeps the last valid date
	}
	// Too long in bytes (possible with many non-ASCII characters): shown in red
	// and not stored, the session keeps the last text that fits
	GtkStyleContext *style = gtk_widget_get_style_context(GTK_WIDGET(e));
	if(!project_text_valid(text, PROJECT_FIELD_MAX)) {
		gtk_style_context_add_class(style, "error");
		return;
	}
	if(i != ENTRY_DATE)
		gtk_style_context_remove_class(style, "error");
	if(set_text_field(entry_field(np, i), text))
		session_mark_changed(np->w);
}

static char *buffer_text(GtkTextBuffer *b)
{
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(b, &start, &end);
	return gtk_text_buffer_get_text(b, &start, &end, FALSE);
}

/* Notes are limited to PROJECT_NOTES_MAX bytes: refuse what would not fit */
static void handle_insert_text(GtkTextBuffer *b, GtkTextIter *at, gchar *text, gint len,
	GtkWidget *view)
{
	UNUSED(at);
	char *now = buffer_text(b);
	size_t size = strlen(now);
	g_free(now);
	if(size + (size_t)(len < 0 ? strlen(text) : (size_t)len) > PROJECT_NOTES_MAX) {
		g_signal_stop_emission_by_name(b, "insert-text");
		gtk_widget_error_bell(view);
	}
}

static void handle_project_notes_changed(GtkTextBuffer *b, struct notes_panel *np)
{
	if(np->updating) return;
	char *text = buffer_text(b);
	if(set_text_field(&np->w->project.notes, text))
		session_mark_changed(np->w);
	g_free(text);
}

static void handle_measurement_notes_changed(GtkTextBuffer *b, struct notes_panel *np)
{
	if(np->updating || !np->target) return;
	char *text = buffer_text(b);
	if(set_text_field(np->target, text))
		session_mark_changed(np->w);
	g_free(text);
}

static void set_buffer(struct notes_panel *np, GtkTextBuffer *b, const char *text)
{
	np->updating++;
	gtk_text_buffer_set_text(b, text ? text : "", -1);
	np->updating--;
}

static GtkWidget *notes_view(GtkTextBuffer **buffer, GCallback changed, struct notes_panel *np)
{
	GtkWidget *view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 4);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 4);
	*buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	g_signal_connect(*buffer, "insert-text", G_CALLBACK(handle_insert_text), view);
	g_signal_connect(*buffer, "changed", changed, np);
	return view;
}

static GtkWidget *scrolled(GtkWidget *child)
{
	GtkWidget *s = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(s), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(s), GTK_SHADOW_IN);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(s), 90);
	gtk_container_add(GTK_CONTAINER(s), child);
	return s;
}

static GtkWidget *left_label(const char *text)
{
	GtkWidget *l = gtk_label_new(text);
	gtk_label_set_xalign(GTK_LABEL(l), 0);
	return l;
}

static void handle_hide(GtkButton *b, struct notes_panel *np)
{
	UNUSED(b);
	notes_panel_set_visible(np, 0);
}

static void handle_toggle(GtkCheckMenuItem *item, struct notes_panel *np)
{
	if(np->updating) return;
	notes_panel_set_visible(np, gtk_check_menu_item_get_active(item));
}

/* The panel covers the graphs, so it needs a solid background and a visible
 * left edge; the grip is a darker strip along that edge */
static void apply_panel_css(void)
{
	static int done = 0;
	if(done) return;
	done = 1;
	static const gchar css[] =
		".notes-panel { background-color: @theme_bg_color; }\n"
		".notes-grip { background-color: @borders; }\n"
		".notes-grip:hover { background-color: @theme_selected_bg_color; }\n";
	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, css, -1, NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);
}

static int remembered_width(struct main_window *w)
{
	GError *err = NULL;
	int width = g_key_file_get_integer(w->config_file, "tg", CONFIG_KEY_WIDTH, &err);
	if(err) {
		g_error_free(err);
		return PANEL_DEFAULT_WIDTH;
	}
	return width < PANEL_MIN_WIDTH ? PANEL_MIN_WIDTH : width;
}

/* Keep the width usable, and leave part of the tabs uncovered */
static int clamp_width(struct notes_panel *np, int width)
{
	GtkWidget *parent = gtk_widget_get_parent(np->box);
	int room = parent ? gtk_widget_get_allocated_width(parent) - PANEL_UNCOVERED : 0;
	if(room > PANEL_MIN_WIDTH && width > room) width = room;
	if(width < PANEL_MIN_WIDTH) width = PANEL_MIN_WIDTH;
	return width;
}

static void set_width(struct notes_panel *np, int width)
{
	np->width = clamp_width(np, width);
	gtk_widget_set_size_request(np->box, np->width, -1);
}

static void handle_grip_realize(GtkWidget *grip, gpointer data)
{
	UNUSED(data);
	GdkCursor *c = gdk_cursor_new_from_name(gtk_widget_get_display(grip), "ew-resize");
	if(c) {
		gdk_window_set_cursor(gtk_widget_get_window(grip), c);
		g_object_unref(c);
	}
}

static gboolean handle_grip_press(GtkWidget *grip, GdkEventButton *e, struct notes_panel *np)
{
	UNUSED(grip);
	if(e->button != 1) return FALSE;
	np->dragging = 1;
	np->drag_x = e->x_root;
	np->drag_width = np->width;
	return TRUE;
}

/* The panel is at the right: moving the grip left widens it. Shown live. */
static gboolean handle_grip_motion(GtkWidget *grip, GdkEventMotion *e, struct notes_panel *np)
{
	UNUSED(grip);
	if(!np->dragging) return FALSE;
	set_width(np, np->drag_width + (int)(np->drag_x - e->x_root));
	return TRUE;
}

static gboolean handle_grip_release(GtkWidget *grip, GdkEventButton *e, struct notes_panel *np)
{
	UNUSED(grip);
	if(e->button != 1 || !np->dragging) return FALSE;
	np->dragging = 0;
	g_key_file_set_integer(np->w->config_file, "tg", CONFIG_KEY_WIDTH, np->width);
	return TRUE;
}

struct notes_panel *notes_panel_new(struct main_window *w)
{
	struct notes_panel *np = g_new0(struct notes_panel, 1);
	np->w = w;

	apply_panel_css();
	/* np->box floats over the right part of the tabs (GtkOverlay): a grip at
	 * its left edge to change the width, then the contents, which scroll if
	 * they do not fit. */
	np->box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_halign(np->box, GTK_ALIGN_END);
	gtk_widget_set_valign(np->box, GTK_ALIGN_FILL);
	gtk_style_context_add_class(gtk_widget_get_style_context(np->box), "notes-panel");
	np->width = remembered_width(w);
	gtk_widget_set_size_request(np->box, np->width, -1);

	GtkWidget *grip = gtk_event_box_new();
	gtk_style_context_add_class(gtk_widget_get_style_context(grip), "notes-grip");
	gtk_widget_set_size_request(grip, 6, -1);
	gtk_widget_set_tooltip_text(grip, _("Drag to change the width of the panel"));
	gtk_widget_add_events(grip, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		GDK_POINTER_MOTION_MASK);
	g_signal_connect(grip, "realize", G_CALLBACK(handle_grip_realize), NULL);
	g_signal_connect(grip, "button-press-event", G_CALLBACK(handle_grip_press), np);
	g_signal_connect(grip, "motion-notify-event", G_CALLBACK(handle_grip_motion), np);
	g_signal_connect(grip, "button-release-event", G_CALLBACK(handle_grip_release), np);
	gtk_box_pack_start(GTK_BOX(np->box), grip, FALSE, FALSE, 0);

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
	gtk_box_pack_start(GTK_BOX(np->box), scroll, TRUE, TRUE, 0);
	GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(content), 6);
	gtk_container_add(GTK_CONTAINER(scroll), content);

	// Page selector, and a button to hide the panel
	GtkWidget *head = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(content), head, FALSE, FALSE, 0);
	np->stack = gtk_stack_new();
	GtkWidget *switcher = gtk_stack_switcher_new();
	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(np->stack));
	gtk_box_pack_start(GTK_BOX(head), switcher, FALSE, FALSE, 0);
	GtkWidget *hide = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
	gtk_button_set_relief(GTK_BUTTON(hide), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(hide, _("Hide the panel"));
	g_signal_connect(hide, "clicked", G_CALLBACK(handle_hide), np);
	gtk_box_pack_end(GTK_BOX(head), hide, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content), np->stack, TRUE, TRUE, 0);

	// Project page: identification on top (foldable), notes below
	GtkWidget *project = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	/* TRANSLATORS: page of the notes panel with the project details and notes. Short */
	gtk_stack_add_titled(GTK_STACK(np->stack), project, "project", _("Project"));

	/* TRANSLATORS: foldable group of the project page: name, reference, owner... */
	GtkWidget *expander = gtk_expander_new(_("Identification"));
	gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
	gtk_box_pack_start(GTK_BOX(project), expander, FALSE, FALSE, 0);
	// Each label above its field, so that the panel stays narrow
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
	gtk_widget_set_margin_top(grid, 4);
	gtk_container_add(GTK_CONTAINER(expander), grid);

	const char *labels[ENTRY_COUNT] = {
		_("Project name"), _("Job reference"), _("Owner"), _("Last service")
	};
	for(int i = 0; i < ENTRY_COUNT; i++) {
		GtkWidget *e = gtk_entry_new();
		// In characters, so that any stored text (at most PROJECT_FIELD_MAX bytes)
		// fits without being cut; the byte limit is checked on every change
		gtk_entry_set_max_length(GTK_ENTRY(e), i == ENTRY_DATE ? 10 : PROJECT_FIELD_MAX);
		gtk_entry_set_width_chars(GTK_ENTRY(e), i == ENTRY_DATE ? 12 : 20);
		gtk_widget_set_hexpand(e, i != ENTRY_DATE);
		g_object_set_data(G_OBJECT(e), "project-entry", GINT_TO_POINTER(i));
		g_signal_connect(e, "changed", G_CALLBACK(handle_entry_changed), np);
		GtkWidget *l = left_label(labels[i]);
		if(i) gtk_widget_set_margin_top(l, 4);
		gtk_grid_attach(GTK_GRID(grid), l, 0, 2 * i, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), e, 0, 2 * i + 1, 1, 1);
		np->entries[i] = e;
	}
	gtk_entry_set_placeholder_text(GTK_ENTRY(np->entries[ENTRY_DATE]),
		_("YYYY, YYYY-MM or YYYY-MM-DD"));
	gtk_widget_set_halign(np->entries[ENTRY_DATE], GTK_ALIGN_START);
	show_date_validity(np->entries[ENTRY_DATE], 1);

	/* TRANSLATORS: label of the watch of the session on the project page */
	GtkWidget *watch = left_label(_("Watch"));
	gtk_widget_set_margin_top(watch, 4);
	gtk_grid_attach(GTK_GRID(grid), watch, 0, 2 * ENTRY_COUNT, 1, 1);
	np->watch_label = left_label("");
	gtk_label_set_ellipsize(GTK_LABEL(np->watch_label), PANGO_ELLIPSIZE_END);
	gtk_widget_set_tooltip_text(np->watch_label,
		_("The watch of the session: the one chosen in the saved watches list."));
	gtk_grid_attach(GTK_GRID(grid), np->watch_label, 0, 2 * ENTRY_COUNT + 1, 1, 1);

	GtkWidget *general = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start(GTK_BOX(project), general, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(general), left_label(_("General notes")), FALSE, FALSE, 0);
	np->project_view = notes_view(&np->project_buffer,
		G_CALLBACK(handle_project_notes_changed), np);
	gtk_box_pack_start(GTK_BOX(general), scrolled(np->project_view), TRUE, TRUE, 0);

	// Measurement page: the notes of the run or snapshot of the selected tab
	// (on the real-time tab, of the run being recorded), or why there are none
	np->measurement_stack = gtk_stack_new();
	/* TRANSLATORS: page of the notes panel with the notes of the selected
	 * run or snapshot. Short */
	gtk_stack_add_titled(GTK_STACK(np->stack), np->measurement_stack, "measurement", _("Measurement"));
	np->none_label = left_label("");
	gtk_label_set_line_wrap(GTK_LABEL(np->none_label), TRUE);
	gtk_widget_set_valign(np->none_label, GTK_ALIGN_START);
	gtk_widget_set_sensitive(np->none_label, FALSE);
	gtk_stack_add_named(GTK_STACK(np->measurement_stack), np->none_label, "none");
	GtkWidget *notes = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	np->measurement_title = left_label("");
	gtk_label_set_ellipsize(GTK_LABEL(np->measurement_title), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start(GTK_BOX(notes), np->measurement_title, FALSE, FALSE, 0);
	np->measurement_view = notes_view(&np->measurement_buffer,
		G_CALLBACK(handle_measurement_notes_changed), np);
	gtk_box_pack_start(GTK_BOX(notes), scrolled(np->measurement_view), TRUE, TRUE, 0);
	gtk_stack_add_named(GTK_STACK(np->measurement_stack), notes, "notes");

	gtk_widget_show_all(np->box);
	notes_panel_show_measurement(np, NULL, NULL, NULL);
	gtk_stack_set_visible_child_name(GTK_STACK(np->stack), "project");
	notes_panel_show_project(np);
	return np;
}

GtkWidget *notes_panel_widget(struct notes_panel *np)
{
	return np->box;
}

void notes_panel_free(struct notes_panel *np)
{
	g_free(np);
}

void notes_panel_show_watch(struct notes_panel *np)
{
	if(!np) return;
	const char *label = np->w->project.watch_label;
	gtk_label_set_text(GTK_LABEL(np->watch_label), label ? label : _("(none)"));
}

void notes_panel_show_project(struct notes_panel *np)
{
	if(!np) return;
	np->updating++;
	for(int i = 0; i < ENTRY_COUNT; i++) {
		const char *text = *entry_field(np, i);
		gtk_entry_set_text(GTK_ENTRY(np->entries[i]), text ? text : "");
		gtk_style_context_remove_class(gtk_widget_get_style_context(np->entries[i]), "error");
	}
	np->updating--;
	show_date_validity(np->entries[ENTRY_DATE], 1);
	set_buffer(np, np->project_buffer, np->w->project.notes);
	notes_panel_show_watch(np);
}

void notes_panel_show_measurement(struct notes_panel *np, char **target, const char *title,
	const char *none)
{
	if(!np) return;
	if(!target) {
		np->target = NULL;
		set_buffer(np, np->measurement_buffer, NULL);
		gtk_label_set_text(GTK_LABEL(np->none_label),
			none ? none : _("Select a run or snapshot tab to write its notes."));
		gtk_stack_set_visible_child_name(GTK_STACK(np->measurement_stack), "none");
		return;
	}
	/* TRANSLATORS: title of the notes of a run or snapshot; %s is its title,
	 * such as "Run 2", "2.3 (DU)" or "Run 3 (in progress)" */
	gchar *text = g_strdup_printf(_("Notes of: %s"), title ? title : _("Snapshot"));
	gtk_label_set_text(GTK_LABEL(np->measurement_title), text);
	g_free(text);
	// Another measurement, or its notes changed elsewhere (a run that ended
	// took them): show its own. A rename keeps the text and the cursor.
	char *now = buffer_text(np->measurement_buffer);
	if(np->target != target || g_strcmp0(now, *target ? *target : ""))
		set_buffer(np, np->measurement_buffer, *target);
	g_free(now);
	np->target = target;
	gtk_stack_set_visible_child_name(GTK_STACK(np->measurement_stack), "notes");
}

void notes_panel_forget(struct notes_panel *np, char **target)
{
	if(np && np->target == target)
		notes_panel_show_measurement(np, NULL, NULL, NULL);
}

void notes_panel_attach_toggle(struct notes_panel *np, GtkWidget *check_item)
{
	np->toggle = check_item;
	g_signal_connect(check_item, "toggled", G_CALLBACK(handle_toggle), np);
}

int notes_panel_remembered_visible(struct main_window *w)
{
	GError *err = NULL;
	gboolean v = g_key_file_get_boolean(w->config_file, "tg", CONFIG_KEY_VISIBLE, &err);
	if(err) {
		g_error_free(err);
		return 0; // hidden until the user shows it
	}
	return v;
}

/* Show or hide the panel; remembered in the configuration file */
void notes_panel_set_visible(struct notes_panel *np, int visible)
{
	if(visible)
		set_width(np, np->width); // the window may be smaller than last time
	gtk_widget_set_visible(np->box, visible);
	g_key_file_set_boolean(np->w->config_file, "tg", CONFIG_KEY_VISIBLE, visible);
	if(np->toggle) {
		np->updating++;
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(np->toggle), visible);
		np->updating--;
	}
}
