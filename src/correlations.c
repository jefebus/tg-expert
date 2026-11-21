/*
    tg-expert
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include "tg.h"
#include <pango/pangocairo.h>

enum correlation_kind {
	CORRELATION_AMP_RATE,
	CORRELATION_BE_RATE,
	CORRELATION_BE_AMP,
	CORRELATION_COUNT
};

struct correlation_style {
	double r, g, b;
	const char *hex;
	const char *symbol;
};

static const struct correlation_style styles[POS_COUNT] = {
	{.65, .65, .65, "#a6a6a6", "◆"},
	{.30, .65, 1,   "#4da6ff", "●"},
	{1, .31, .82,   "#ff4fd1", "■"},
	{.98, .76, .02, "#fac205", "▲"},
	{.35, .85, .45, "#59d973", "▼"},
	{.71, .49, 1,   "#b57dff", "+"},
	{1, .48, .25,   "#ff7a40", "×"}
};

struct correlation_plot {
	int valid, selected;
	double xmin, xmax, ymin, ymax;
	double left, top, width, height;
};

static int sample_position(const struct trend_sample *sample)
{
	return sample->position >= POS_NONE && sample->position < POS_COUNT ?
		sample->position : POS_NONE;
}

static int correlation_values(const struct trend_sample *sample,
	enum correlation_kind kind, double *x, double *y)
{
	if(sample->settling)
		return 0;
	switch(kind) {
	case CORRELATION_AMP_RATE:
		if(!isfinite(sample->amp) || sample->amp <= 0 || !isfinite(sample->rate))
			return 0;
		*x = sample->amp;
		*y = sample->rate;
		return 1;
	case CORRELATION_BE_RATE:
		if(!isfinite(sample->be) || sample->be < 0 || !isfinite(sample->rate))
			return 0;
		*x = sample->be;
		*y = sample->rate;
		return 1;
	case CORRELATION_BE_AMP:
		if(!isfinite(sample->be) || sample->be < 0 ||
				!isfinite(sample->amp) || sample->amp <= 0)
			return 0;
		*x = sample->be;
		*y = sample->amp;
		return 1;
	case CORRELATION_COUNT:
		break;
	}
	return 0;
}

static int in_time_range(const struct trend_sample *sample, double t0, double t1)
{
	return isfinite(sample->t) && sample->t >= t0 && sample->t <= t1;
}

static void correlation_time_range(const struct output_panel *op, double *t0, double *t1)
{
	*t1 = trend_elapsed(op->trend);
	*t0 = 0;
	if(op->trend_span_s > 0) {
		*t1 = fmax(*t1, op->trend_span_s);
		*t0 = *t1 - op->trend_span_s;
	} else if(*t1 < 60)
		*t1 = 60;
}

static void correlation_measure(struct correlation_plot *plot, const struct trend *trend,
	enum correlation_kind kind, int position, double t0, double t1)
{
	memset(plot, 0, sizeof(*plot));
	if(!trend) return;
	for(int i = 0; i < trend->count; i++) {
		const struct trend_sample *sample = &trend->samples[i];
		double x, y;
		if(!in_time_range(sample, t0, t1) || !correlation_values(sample, kind, &x, &y))
			continue;
		plot->valid++;
		if(position >= 0 && sample_position(sample) != position)
			continue;
		plot->selected++;
	}
}

static void correlation_ranges(struct correlation_plot *plot, const struct output_panel *op,
	enum correlation_kind kind, double t0, double t1)
{
	struct trend_plot_ranges ranges;
	trend_get_plot_ranges(op->trend, t0, t1, op->trend_rate_range, &ranges);
	plot->xmin = kind == CORRELATION_AMP_RATE ? ranges.amp_min : ranges.be_min;
	plot->xmax = kind == CORRELATION_AMP_RATE ? ranges.amp_max : ranges.be_max;
	plot->ymin = kind == CORRELATION_BE_AMP ? ranges.amp_min : ranges.rate_min;
	plot->ymax = kind == CORRELATION_BE_AMP ? ranges.amp_max : ranges.rate_max;
}

static void correlation_geometry(struct correlation_plot *plot, int width, int height)
{
	double available_width = width - 58 - 15;
	double available_height = height - 30 - 42;
	double side = fmin(available_width, available_height);
	plot->left = 58 + (available_width - side) / 2;
	plot->top = 30 + (available_height - side) / 2;
	plot->width = plot->height = side;
}

static void correlation_point(const struct correlation_plot *plot, double x, double y,
	double *px, double *py)
{
	x = fmin(fmax(x, plot->xmin), plot->xmax);
	y = fmin(fmax(y, plot->ymin), plot->ymax);
	*px = plot->left + (x - plot->xmin) / (plot->xmax - plot->xmin) * plot->width;
	*py = plot->top + (plot->ymax - y) / (plot->ymax - plot->ymin) * plot->height;
}

static void draw_symbol(cairo_t *cr, int position, double x, double y, double radius)
{
	const struct correlation_style *style = &styles[position];
	cairo_set_source_rgb(cr, style->r, style->g, style->b);
	cairo_set_line_width(cr, 1.4);
	switch(position) {
	case POS_NONE:
		cairo_move_to(cr, x, y - radius);
		cairo_line_to(cr, x + radius, y);
		cairo_line_to(cr, x, y + radius);
		cairo_line_to(cr, x - radius, y);
		cairo_close_path(cr);
		cairo_stroke(cr);
		break;
	case POS_DU:
		cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
		cairo_fill(cr);
		break;
	case POS_DD:
		cairo_rectangle(cr, x - radius, y - radius, 2 * radius, 2 * radius);
		cairo_fill(cr);
		break;
	case POS_CU:
	case POS_CD: {
		int sign = position == POS_CU ? 1 : -1;
		cairo_move_to(cr, x, y - sign * radius);
		cairo_line_to(cr, x + radius, y + sign * radius);
		cairo_line_to(cr, x - radius, y + sign * radius);
		cairo_close_path(cr);
		cairo_fill(cr);
		break;
	}
	case POS_CL:
		cairo_move_to(cr, x - radius, y);
		cairo_line_to(cr, x + radius, y);
		cairo_move_to(cr, x, y - radius);
		cairo_line_to(cr, x, y + radius);
		cairo_stroke(cr);
		break;
	case POS_CR:
		cairo_move_to(cr, x - radius, y - radius);
		cairo_line_to(cr, x + radius, y + radius);
		cairo_move_to(cr, x - radius, y + radius);
		cairo_line_to(cr, x + radius, y - radius);
		cairo_stroke(cr);
		break;
	default:
		break;
	}
}

static void draw_text(cairo_t *cr, double x, double y, double align, const char *text)
{
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	cairo_move_to(cr, x - align * extents.x_advance, y);
	cairo_show_text(cr, text);
}

static void draw_message(cairo_t *cr, int width, int height, const char *message)
{
	PangoLayout *layout = pango_cairo_create_layout(cr);
	pango_layout_set_text(layout, message, -1);
	pango_layout_set_width(layout, (width - 20) * PANGO_SCALE);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	int text_height;
	pango_layout_get_pixel_size(layout, NULL, &text_height);
	cairo_set_source_rgb(cr, .75, .75, .75);
	cairo_move_to(cr, 10, (height - text_height) / 2.0);
	pango_cairo_show_layout(cr, layout);
	g_object_unref(layout);
}

static enum correlation_kind widget_kind(GtkWidget *widget)
{
	return (enum correlation_kind)(GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(widget), "correlation-kind")) - 1);
}

static gboolean correlation_draw(GtkWidget *widget, cairo_t *cr, struct output_panel *op)
{
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_paint(cr);
	if(width < 100 || height < 100) return FALSE;
	if(!op->trend) {
		draw_message(cr, width, height, _("No long-term statistics"));
		return FALSE;
	}
	if(!op->trend->count) {
		draw_message(cr, width, height, _("No readings were recorded"));
		return FALSE;
	}

	enum correlation_kind kind = widget_kind(widget);
	double t0, t1;
	correlation_time_range(op, &t0, &t1);
	struct correlation_plot plot;
	correlation_measure(&plot, op->trend, kind, op->correlation_position, t0, t1);
	if(!plot.valid) {
		draw_message(cr, width, height, _("No valid points for this graph in the selected time range"));
		return FALSE;
	}
	if(!plot.selected) {
		draw_message(cr, width, height, _("No points for the selected position"));
		return FALSE;
	}
	correlation_ranges(&plot, op, kind, t0, t1);
	correlation_geometry(&plot, width, height);
	cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 10);
	for(int i = 0; i <= 4; i++) {
		double y = plot.top + plot.height * i / 4;
		double value = plot.ymax - (plot.ymax - plot.ymin) * i / 4;
		cairo_set_source_rgba(cr, 1, 1, 1, .22);
		cairo_move_to(cr, plot.left, y);
		cairo_line_to(cr, plot.left + plot.width, y);
		cairo_stroke(cr);
		char text[32];
		snprintf(text, sizeof(text), "%.1f", value);
		cairo_set_source_rgb(cr, .85, .85, .85);
		draw_text(cr, plot.left - 5, y + 3, 1, text);
	}
	for(int i = 0; i <= 2; i++) {
		double x = plot.left + plot.width * i / 2;
		double value = plot.xmin + (plot.xmax - plot.xmin) * i / 2;
		char text[32];
		snprintf(text, sizeof(text), kind == CORRELATION_AMP_RATE ? "%.1f" : "%.2f", value);
		cairo_set_source_rgb(cr, .85, .85, .85);
		draw_text(cr, x, plot.top + plot.height + 14, .5, text);
	}
	cairo_set_source_rgba(cr, 1, 1, 1, .22);
	cairo_rectangle(cr, plot.left, plot.top, plot.width, plot.height);
	cairo_stroke(cr);
	cairo_set_source_rgb(cr, 1, 1, 1);
	draw_text(cr, plot.left, plot.top - 13, 0,
		kind == CORRELATION_BE_AMP ? _("Amplitude (°)") : _("Rate (s/d)"));
	draw_text(cr, plot.left + plot.width / 2, plot.top + plot.height + 38, .5,
		kind == CORRELATION_AMP_RATE ? _("Amplitude (°)") : _("Beat error (ms)"));

	cairo_save(cr);
	cairo_rectangle(cr, plot.left, plot.top, plot.width, plot.height);
	cairo_clip(cr);
	for(int i = 0; i < op->trend->count; i++) {
		const struct trend_sample *sample = &op->trend->samples[i];
		int position = sample_position(sample);
		double x, y, px, py;
		if(!in_time_range(sample, t0, t1) ||
				(op->correlation_position >= 0 && op->correlation_position != position) ||
				!correlation_values(sample, kind, &x, &y))
			continue;
		correlation_point(&plot, x, y, &px, &py);
		draw_symbol(cr, position, px, py, 3.5);
	}
	cairo_restore(cr);
	return FALSE;
}

static gboolean correlation_tooltip(GtkWidget *widget, gint x, gint y,
	gboolean keyboard_mode, GtkTooltip *tooltip, struct output_panel *op)
{
	if(keyboard_mode || !op->trend || !op->trend->count)
		return FALSE;
	enum correlation_kind kind = widget_kind(widget);
	double t0, t1;
	correlation_time_range(op, &t0, &t1);
	struct correlation_plot plot;
	correlation_measure(&plot, op->trend, kind, op->correlation_position, t0, t1);
	correlation_ranges(&plot, op, kind, t0, t1);
	correlation_geometry(&plot, gtk_widget_get_allocated_width(widget),
		gtk_widget_get_allocated_height(widget));
	if(!plot.selected || plot.width <= 0 || plot.height <= 0)
		return FALSE;

	const struct trend_sample *nearest = NULL;
	double best = 64;
	for(int i = 0; i < op->trend->count; i++) {
		const struct trend_sample *sample = &op->trend->samples[i];
		int position = sample_position(sample);
		double vx, vy, px, py;
		if(!in_time_range(sample, t0, t1) ||
				(op->correlation_position >= 0 && op->correlation_position != position) ||
				!correlation_values(sample, kind, &vx, &vy))
			continue;
		correlation_point(&plot, vx, vy, &px, &py);
		double distance = (px - x) * (px - x) + (py - y) * (py - y);
		if(distance <= best) {
			best = distance;
			nearest = sample;
		}
	}
	if(!nearest) return FALSE;

	char time[32], rate[40], amp[40], be[40];
	format_duration(time, sizeof(time), nearest->t);
	if(isfinite(nearest->rate))
		snprintf(rate, sizeof(rate), "%+.1f s/d", nearest->rate);
	else
		snprintf(rate, sizeof(rate), "%s", _("Unavailable"));
	if(isfinite(nearest->amp) && nearest->amp > 0)
		snprintf(amp, sizeof(amp), "%.1f°", nearest->amp);
	else
		snprintf(amp, sizeof(amp), "%s", _("Unavailable"));
	if(isfinite(nearest->be) && nearest->be >= 0)
		snprintf(be, sizeof(be), "%.2f ms", nearest->be);
	else
		snprintf(be, sizeof(be), "%s", _("Unavailable"));
	char *text = g_strdup_printf(_("Elapsed: %s\nPosition: %s\nRate: %s\nAmplitude: %s\nBeat error: %s"),
		time, watch_position_label(sample_position(nearest)), rate, amp, be);
	gtk_tooltip_set_text(tooltip, text);
	g_free(text);
	return TRUE;
}

void correlations_queue_draw(struct output_panel *op)
{
	for(int i = 0; i < CORRELATION_COUNT; i++)
		if(op->correlation_areas[i])
			gtk_widget_queue_draw(op->correlation_areas[i]);
}

static void correlation_position_changed(GtkComboBox *combo, struct output_panel *op)
{
	op->correlation_position = gtk_combo_box_get_active(combo) - 1;
	correlations_queue_draw(op);
}

GtkWidget *correlations_create_page(struct output_panel *op)
{
	static const char *titles[CORRELATION_COUNT] = {
		N_("Amplitude → rate"), N_("B.E. → rate"), N_("B.E. → amplitude")
	};
	static const char *help[CORRELATION_COUNT] = {
		N_("Within one position, shows how rate changes with amplitude. Isochronism curve."),
		N_("Shows whether beat error and rate change together."),
		N_("Shows whether beat error remains stable or changes at certain amplitudes.")
	};
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_box_pack_start(GTK_BOX(page), controls, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(controls), gtk_label_new(_("Position")), FALSE, FALSE, 0);
	GtkWidget *position = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(position), _("All"));
	for(int i = POS_NONE; i < POS_COUNT; i++)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(position), watch_position_label(i));
	gtk_combo_box_set_active(GTK_COMBO_BOX(position), 0);
	gtk_box_pack_start(GTK_BOX(controls), position, FALSE, FALSE, 0);
	g_signal_connect(position, "changed", G_CALLBACK(correlation_position_changed), op);
	gtk_box_pack_start(GTK_BOX(controls), gtk_label_new(_("Show")), FALSE, FALSE, 0);
	op->correlation_span_combo = op_create_trend_span_combo(op);
	gtk_box_pack_start(GTK_BOX(controls), op->correlation_span_combo, FALSE, FALSE, 0);

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(page), scroll, TRUE, TRUE, 0);
	GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_add(GTK_CONTAINER(scroll), content);

	GtkWidget *legend = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_pack_start(GTK_BOX(content), legend, FALSE, FALSE, 0);
	for(int i = POS_NONE; i < POS_COUNT; i++) {
		const char *code = watch_position_code(i);
		GtkWidget *label = gtk_label_new(NULL);
		char *markup = g_markup_printf_escaped("<span foreground=\"%s\">%s %s</span>",
			styles[i].hex, styles[i].symbol, code ? code : watch_position_label(i));
		gtk_label_set_markup(GTK_LABEL(label), markup);
		g_free(markup);
		gtk_widget_set_tooltip_text(label, watch_position_label(i));
		gtk_box_pack_start(GTK_BOX(legend), label, FALSE, FALSE, 0);
	}

	GtkWidget *graphs = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_box_set_homogeneous(GTK_BOX(graphs), TRUE);
	gtk_box_pack_start(GTK_BOX(content), graphs, TRUE, TRUE, 0);
	GtkSizeGroup *help_heights = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);
	for(int i = 0; i < CORRELATION_COUNT; i++) {
		GtkWidget *column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
		GtkWidget *frame = gtk_frame_new(_(titles[i]));
		gtk_container_add(GTK_CONTAINER(frame), column);
		gtk_box_pack_start(GTK_BOX(graphs), frame, TRUE, TRUE, 0);
		GtkWidget *area = gtk_drawing_area_new();
		op->correlation_areas[i] = area;
		gtk_widget_set_size_request(area, 240, 200);
		g_object_set_data(G_OBJECT(area), "correlation-kind", GINT_TO_POINTER(i + 1));
		gtk_widget_set_has_tooltip(area, TRUE);
		g_signal_connect(area, "draw", G_CALLBACK(correlation_draw), op);
		g_signal_connect(area, "query-tooltip", G_CALLBACK(correlation_tooltip), op);
		gtk_box_pack_start(GTK_BOX(column), area, TRUE, TRUE, 0);
		GtkWidget *description = gtk_label_new(_(help[i]));
		gtk_label_set_line_wrap(GTK_LABEL(description), TRUE);
		gtk_label_set_max_width_chars(GTK_LABEL(description), 32);
		gtk_label_set_xalign(GTK_LABEL(description), 0);
		gtk_label_set_yalign(GTK_LABEL(description), 0);
		gtk_size_group_add_widget(help_heights, description);
		gtk_box_pack_start(GTK_BOX(column), description, FALSE, FALSE, 0);
	}
	g_object_unref(help_heights);
	return page;
}
