/*
    tg-expert
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

/* The scatter plots are drawn by render_correlation(), which only takes data,
 * a size, options and a theme (TG-31), so that the PDF report uses the same
 * code as the screen. The GTK callbacks below only gather those from the
 * output panel. */

#include "tg.h"

/* Legend of the screen page (GTK labels): the colours of the screen theme
 * and the characters closest to the symbols drawn by render_position_symbol() */
struct correlation_legend {
	const char *hex;
	const char *symbol;
};

static const struct correlation_legend legend_styles[POS_COUNT] = {
	{"#a6a6a6", "◆"},
	{"#4da6ff", "●"},
	{"#ff4fd1", "■"},
	{"#fac205", "▲"},
	{"#59d973", "▼"},
	{"#b57dff", "+"},
	{"#ff7a40", "×"}
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

void correlation_time_range(const struct trend *t, int span_s, double *t0, double *t1)
{
	*t1 = t ? trend_elapsed(t) : 0;
	*t0 = 0;
	if(span_s > 0) {
		*t1 = fmax(*t1, span_s);
		*t0 = *t1 - span_s;
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

static void correlation_ranges(struct correlation_plot *plot, const struct trend *t,
	const struct correlation_options *o)
{
	struct trend_plot_ranges ranges;
	trend_get_plot_ranges(t, o->t0, o->t1, o->rate_range, o->be_range, &ranges);
	plot->xmin = o->kind == CORRELATION_AMP_RATE ? ranges.amp_min : ranges.be_min;
	plot->xmax = o->kind == CORRELATION_AMP_RATE ? ranges.amp_max : ranges.be_max;
	plot->ymin = o->kind == CORRELATION_BE_AMP ? ranges.amp_min : ranges.rate_min;
	plot->ymax = o->kind == CORRELATION_BE_AMP ? ranges.amp_max : ranges.rate_max;
}

/* A square plot area; the margins hold the scales and grow with the font
 * (the screen uses 10, which gives the margins it always had) */
static void correlation_geometry(struct correlation_plot *plot, double width, double height,
	double font)
{
	double k = font / 10;
	double left = 58 * k, right = 15 * k, top = 30 * k, bottom = 42 * k;
	double available_width = width - left - right;
	double available_height = height - top - bottom;
	double side = fmin(available_width, available_height);
	plot->left = left + (available_width - side) / 2;
	plot->top = top + (available_height - side) / 2;
	plot->width = plot->height = side;
}

/* Position of a reading in the plot. 0 if it is outside the scales: it is
 * then not drawn, rather than drawn on the border as if it had been read
 * there (with a fixed rate or beat error range). */
static int correlation_point(const struct correlation_plot *plot, double x, double y,
	double *px, double *py)
{
	if(x < plot->xmin || x > plot->xmax || y < plot->ymin || y > plot->ymax)
		return 0;
	*px = plot->left + (x - plot->xmin) / (plot->xmax - plot->xmin) * plot->width;
	*py = plot->top + (plot->ymax - y) / (plot->ymax - plot->ymin) * plot->height;
	return 1;
}

static int axis_subdivisions(double major_step, int decimals)
{
	long units = lround(major_step * (decimals == 2 ? 100 : 10));
	int best = 0, best_zeros = -1;
	for(int divisions = 5; divisions <= 10; divisions++) {
		if(units % divisions)
			continue;
		long step = units / divisions;
		if(step <= 0)
			continue;
		int zeros = 0;
		while(step % 10 == 0) {
			zeros++;
			step /= 10;
		}
		if(zeros > best_zeros) {
			best = divisions;
			best_zeros = zeros;
		}
	}
	g_return_val_if_fail(best != 0, 0);
	return best;
}

static void draw_axis_grid(cairo_t *cr, const struct correlation_plot *plot,
	enum correlation_kind kind, const struct render_theme *th)
{
	int xdiv = axis_subdivisions((plot->xmax - plot->xmin) / 2,
		kind == CORRELATION_AMP_RATE ? 1 : 2);
	int ydiv = axis_subdivisions((plot->ymax - plot->ymin) / 4, 1);
	if(!xdiv || !ydiv)
		return;
	cairo_save(cr);
	cairo_set_line_width(cr, th->line_scale);
	render_set_rgb(cr, th->grid, th->grid_alpha);
	cairo_move_to(cr, plot->left + plot->width / 2, plot->top);
	cairo_line_to(cr, plot->left + plot->width / 2, plot->top + plot->height);
	cairo_stroke(cr);

	static const double dashes[] = {3, 3};
	cairo_set_dash(cr, dashes, 2, 0);
	render_set_rgb(cr, th->grid, th->grid_minor_alpha);
	for(int i = 1; i < 2 * xdiv; i++) {
		if(i % xdiv == 0)
			continue;
		double x = plot->left + plot->width * i / (2 * xdiv);
		cairo_move_to(cr, x, plot->top);
		cairo_line_to(cr, x, plot->top + plot->height);
	}
	for(int i = 1; i < 4 * ydiv; i++) {
		if(i % ydiv == 0)
			continue;
		double y = plot->top + plot->height * i / (4 * ydiv);
		cairo_move_to(cr, plot->left, y);
		cairo_line_to(cr, plot->left + plot->width, y);
	}
	cairo_stroke(cr);
	cairo_restore(cr);
}

static enum correlation_kind widget_kind(GtkWidget *widget)
{
	return (enum correlation_kind)(GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(widget), "correlation-kind")) - 1);
}

void render_correlation(cairo_t *cr, double width, double height, const struct trend *t,
	const struct correlation_options *o, const struct render_theme *th)
{
	const double font = o->font > 0 ? o->font : 10;
	const double k = font / 10;
	cairo_save(cr);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_clip(cr);
	render_set_rgb(cr, th->background, 1);
	cairo_paint(cr);
	if(width < 100 * k || height < 100 * k) goto done;
	if(!t) {
		render_message(cr, th, width, height, _("No long-term statistics"));
		goto done;
	}
	if(!t->count) {
		render_message(cr, th, width, height, _("No readings were recorded"));
		goto done;
	}

	struct correlation_plot plot;
	correlation_measure(&plot, t, o->kind, o->position, o->t0, o->t1);
	if(!plot.valid) {
		render_message(cr, th, width, height, _("No valid points for this graph in the selected time range"));
		goto done;
	}
	if(!plot.selected) {
		render_message(cr, th, width, height, _("No points for the selected position"));
		goto done;
	}
	correlation_ranges(&plot, t, o);
	correlation_geometry(&plot, width, height, font);
	// The font family is the caller's: the screen's default sans, or the
	// report's DejaVu Sans
	cairo_set_font_size(cr, font);
	cairo_set_line_width(cr, th->line_scale);
	for(int i = 0; i <= 4; i++) {
		double y = plot.top + plot.height * i / 4;
		double value = plot.ymax - (plot.ymax - plot.ymin) * i / 4;
		render_set_rgb(cr, th->grid, th->grid_alpha);
		cairo_move_to(cr, plot.left, y);
		cairo_line_to(cr, plot.left + plot.width, y);
		cairo_stroke(cr);
		char text[32];
		snprintf(text, sizeof(text), "%.1f", value);
		render_set_rgb(cr, th->label, 1);
		render_text(cr, plot.left - 5 * k, y + 3 * k, 1, text);
	}
	for(int i = 0; i <= 2; i++) {
		double x = plot.left + plot.width * i / 2;
		double value = plot.xmin + (plot.xmax - plot.xmin) * i / 2;
		char text[32];
		snprintf(text, sizeof(text), o->kind == CORRELATION_AMP_RATE ? "%.1f" : "%.2f", value);
		render_set_rgb(cr, th->label, 1);
		render_text(cr, x, plot.top + plot.height + 14 * k, .5, text);
	}
	render_set_rgb(cr, th->grid, th->grid_alpha);
	cairo_rectangle(cr, plot.left, plot.top, plot.width, plot.height);
	cairo_stroke(cr);
	draw_axis_grid(cr, &plot, o->kind, th);
	render_set_rgb(cr, th->text, 1);
	render_text(cr, plot.left, plot.top - 13 * k, 0,
		o->kind == CORRELATION_BE_AMP ? _("Amplitude (°)") : _("Rate (s/d)"));
	render_text(cr, plot.left + plot.width / 2, plot.top + plot.height + 38 * k, .5,
		o->kind == CORRELATION_AMP_RATE ? _("Amplitude (°)") : _("Beat error (ms)"));

	int outside = 0;
	cairo_save(cr);
	cairo_rectangle(cr, plot.left, plot.top, plot.width, plot.height);
	cairo_clip(cr);
	for(int i = 0; i < t->count; i++) {
		const struct trend_sample *sample = &t->samples[i];
		int position = sample_position(sample);
		double x, y, px, py;
		if(!in_time_range(sample, o->t0, o->t1) ||
				(o->position >= 0 && o->position != position) ||
				!correlation_values(sample, o->kind, &x, &y))
			continue;
		if(!correlation_point(&plot, x, y, &px, &py)) {
			outside++;
			continue;
		}
		render_position_symbol(cr, th, position, px, py, 3.5 * k);
	}
	cairo_restore(cr);
	// Readings beyond a fixed scale are counted, not drawn on the border
	if(outside) {
		/* TRANSLATORS: correlation graph: number of readings beyond the scales,
		 * which are not drawn. Short, shown above the plot at the right */
		char *text = g_strdup_printf(_("Off scale: %d"), outside);
		render_set_rgb(cr, th->label, 1);
		render_text(cr, plot.left + plot.width, plot.top - 13 * k, 1, text);
		g_free(text);
	}
done:
	cairo_restore(cr);
}

/* What the screen shows, taken from the output panel */
static void correlation_screen_options(struct correlation_options *o, GtkWidget *widget,
	const struct output_panel *op)
{
	o->kind = widget_kind(widget);
	o->position = op->correlation_position;
	correlation_time_range(op->trend, op->trend_span_s, &o->t0, &o->t1);
	o->rate_range = op->trend_rate_range;
	o->be_range = op->trend_be_range;
	o->font = 10;
}

static gboolean correlation_draw(GtkWidget *widget, cairo_t *cr, struct output_panel *op)
{
	struct correlation_options o;
	correlation_screen_options(&o, widget, op);
	render_correlation(cr, gtk_widget_get_allocated_width(widget),
		gtk_widget_get_allocated_height(widget), op->trend, &o,
		render_theme_get(RENDER_SCREEN));
	return FALSE;
}

static gboolean correlation_tooltip(GtkWidget *widget, gint x, gint y,
	gboolean keyboard_mode, GtkTooltip *tooltip, struct output_panel *op)
{
	if(keyboard_mode || !op->trend || !op->trend->count)
		return FALSE;
	struct correlation_options o;
	correlation_screen_options(&o, widget, op);
	struct correlation_plot plot;
	correlation_measure(&plot, op->trend, o.kind, o.position, o.t0, o.t1);
	correlation_ranges(&plot, op->trend, &o);
	correlation_geometry(&plot, gtk_widget_get_allocated_width(widget),
		gtk_widget_get_allocated_height(widget), o.font);
	if(!plot.selected || plot.width <= 0 || plot.height <= 0)
		return FALSE;

	const struct trend_sample *nearest = NULL;
	double best = 64;
	for(int i = 0; i < op->trend->count; i++) {
		const struct trend_sample *sample = &op->trend->samples[i];
		int position = sample_position(sample);
		double vx, vy, px, py;
		if(!in_time_range(sample, o.t0, o.t1) ||
				(o.position >= 0 && o.position != position) ||
				!correlation_values(sample, o.kind, &vx, &vy) ||
				!correlation_point(&plot, vx, vy, &px, &py))
			continue;
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
	gtk_box_pack_start(GTK_BOX(controls), gtk_label_new(_("B.E.")), FALSE, FALSE, 0);
	op->correlation_be_combo = op_create_trend_be_combo(op);
	gtk_box_pack_start(GTK_BOX(controls), op->correlation_be_combo, FALSE, FALSE, 0);

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
			legend_styles[i].hex, legend_styles[i].symbol, code ? code : watch_position_label(i));
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
