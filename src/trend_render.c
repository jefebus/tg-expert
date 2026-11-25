/*
    tg-expert
    Copyright (C) 2015 Marcello Mamino
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

/* TG-31: the Trend page (statistics since the start and per position, and
 * the rate, amplitude and beat error graphs), drawn from the data alone, so
 * that the screen (output_panel.c) and the PDF report share it. */

#include "tg.h"

static void set_dotted(cairo_t *c, int dotted)
{
	static const double dots[2] = {2, 4};
	if(dotted)
		cairo_set_dash(c, dots, 2, 0);
	else
		cairo_set_dash(c, NULL, 0, 0);
}

/* Text with its left edge (align 0), centre (0.5) or right edge (1) at x */
static void trend_text(cairo_t *c, double x, double y, double align, const char *s)
{
	render_text(c, x, y, align, s);
}

static void trend_message(cairo_t *c, const struct render_theme *th, double width, double height,
	double font, const char *line1, const char *line2)
{
	cairo_set_font_size(c, font);
	render_set_rgb(c, th->axis, 0.85);
	trend_text(c, width / 2.0, height / 2.0 - font, 0.5, line1);
	if(line2) {
		cairo_set_font_size(c, font * 0.8);
		render_set_rgb(c, th->axis, 0.6);
		trend_text(c, width / 2.0, height / 2.0 + font * 0.6, 0.5, line2);
	}
}

/* Line style of a grid line of the theme: major = solid, minor = dotted and
 * fainter. Call set_dotted(c, 0) after stroking. */
static void theme_axis_style(cairo_t *c, const struct render_theme *th, int major)
{
	render_set_rgb(c, th->axis, major ? th->axis_major_alpha : th->axis_minor_alpha);
	set_dotted(c, !major);
}

/* The readings that fall on one pixel column of a graph */
struct trend_column {
	double x, min, max, sum;
	int n;
	int gap_before;	// no reading for a while before it: the line is broken
};

enum trend_series {
	TREND_SERIES_RATE,
	TREND_SERIES_AMPLITUDE,
	TREND_SERIES_BEAT_ERROR
};

/* Collapse the readings between t0 and t1 into columns of one unit (pixel or
 * point), at most max of them. Each column keeps its minimum and maximum, so
 * that no extreme reading is hidden. Series: rate, amplitude (readings
 * without one are skipped), or beat error. */
static int trend_columns(const struct trend *t, double t0, double t1, double gx, double gw,
	enum trend_series series, struct trend_column *cols, int max)
{
	int n = 0;
	double prev_t = -1;
	for(int i = 0; i < t->count; i++) {
		const struct trend_sample *s = &t->samples[i];
		if(s->t < t0 || s->t > t1) continue;
		double v = series == TREND_SERIES_RATE ? s->rate :
			series == TREND_SERIES_AMPLITUDE ? s->amp : s->be;
		if(series == TREND_SERIES_AMPLITUDE && v <= 0) continue;
		double x = floor(gx + (s->t - t0) / (t1 - t0) * gw) + .5;
		int gap = prev_t >= 0 && s->t - prev_t > 3 * TREND_INTERVAL_S;
		prev_t = s->t;
		if(n && !gap && cols[n-1].x == x) {
			struct trend_column *col = &cols[n-1];
			if(v < col->min) col->min = v;
			if(v > col->max) col->max = v;
			col->sum += v;
			col->n++;
		} else if(n < max) {
			struct trend_column *col = &cols[n++];
			col->x = x;
			col->min = col->max = col->sum = v;
			col->n = 1;
			col->gap_before = gap;
		}
	}
	return n;
}

/* A series between y_top (v_max) and y_bottom (v_min): the spread of each
 * column faint, its mean as a line. A mean beyond the scale is not drawn on
 * the border as if read there: the line breaks, and the readings are counted
 * and returned. The spread is cut at the edges of the graph. */
static int trend_draw_series(cairo_t *c, const struct trend_column *cols, int n,
	double gx, double gw, double y_top, double y_bottom, double v_min, double v_max,
	struct render_rgb col, const struct render_theme *th)
{
#define VY(v) (y_bottom - ((v) - v_min) / (v_max - v_min) * (y_bottom - y_top))
#define MEAN(i) (cols[i].sum / cols[i].n)
#define INSIDE(i) (MEAN(i) >= v_min && MEAN(i) <= v_max)
	int off = 0;
	cairo_save(c);
	cairo_rectangle(c, gx - 2, y_top - 2, gw + 4, y_bottom - y_top + 4);
	cairo_clip(c);
	cairo_set_line_width(c, th->line_scale);
	for(int i = 0; i < n; i++) {
		if(cols[i].n < 2) continue;
		cairo_move_to(c, cols[i].x, VY(cols[i].min));
		cairo_line_to(c, cols[i].x, VY(cols[i].max));
	}
	render_set_rgb(c, col, 0.35);
	cairo_stroke(c);

	cairo_set_line_width(c, 1.5 * th->line_scale);
	for(int i = 0; i < n; i++) {
		if(!INSIDE(i)) {
			off += cols[i].n;
			continue;
		}
		double y = VY(MEAN(i));
		int starts = i == 0 || cols[i].gap_before || !INSIDE(i-1);
		int ends = i + 1 == n || cols[i+1].gap_before || !INSIDE(i+1);
		// A reading on its own is drawn as a short dash, so that it shows
		if(starts && ends) {
			cairo_move_to(c, cols[i].x - 1, y);
			cairo_line_to(c, cols[i].x + 1, y);
		} else if(starts)
			cairo_move_to(c, cols[i].x, y);
		else
			cairo_line_to(c, cols[i].x, y);
	}
	render_set_rgb(c, col, 1);
	cairo_stroke(c);
	cairo_restore(c);
	return off;
#undef INSIDE
#undef MEAN
#undef VY
}

/* Horizontal grid line with its label left of the graph */
static void trend_hline(cairo_t *c, const struct render_theme *th, double gx, double gw,
	double y, int major, const char *label)
{
	cairo_set_line_width(c, th->line_scale);
	cairo_move_to(c, gx, round(y) + .5);
	cairo_line_to(c, gx + gw, round(y) + .5);
	theme_axis_style(c, th, major);
	cairo_stroke(c);
	set_dotted(c, 0);
	render_set_rgb(c, th->axis, 0.7);
	trend_text(c, gx - 4, y + 4, 1, label);
}

static void trend_draw_average(cairo_t *c, const struct render_theme *th, double gx, double gw,
	double y_top, double y_bottom, double v_min, double v_max,
	const struct trend_stats *stats, struct render_rgb col)
{
	double mean = trend_stats_mean(stats);
	if(!stats->n || mean < v_min || mean > v_max)
		return;

	static const double dashes[2] = {6, 4};
	double y = round(y_bottom - (mean - v_min) / (v_max - v_min) *
		(y_bottom - y_top)) + .5;
	cairo_set_line_width(c, th->line_scale);
	cairo_set_dash(c, dashes, 2, 0);
	cairo_move_to(c, gx, y);
	cairo_line_to(c, gx + gw, y);
	render_set_rgb(c, col, 0.8);
	cairo_stroke(c);
	set_dotted(c, 0);
}

/* Title of a graph at the left and, at the right, how many readings are
 * beyond its scale */
static void trend_graph_title(cairo_t *c, const struct render_theme *th, double gx, double gw,
	double y, double font, struct render_rgb col, const char *title, int off)
{
	cairo_set_font_size(c, font * 0.9);
	render_set_rgb(c, col, 1);
	trend_text(c, gx, y, 0, title);
	if(off) {
		char *s = g_strdup_printf(_("Off scale: %d"), off);
		cairo_set_font_size(c, font * 0.8);
		render_set_rgb(c, th->axis, 0.7);
		trend_text(c, gx + gw, y, 1, s);
		g_free(s);
	}
}

/* Height that the statistics (TREND_RENDER_STATS alone) take with this text
 * size: the same steps as render_trend(), for a trend with readings */
double render_trend_stats_height(const struct trend *t, double font)
{
	const double line = font * 1.35;
	double y = font + 4;
	y += line;		// column heads
	y += 3 * line;		// rate, amplitude, beat error
	y += line * 0.85;	// duration and coverage
	y += line * 1.2;	// settings
	int set_positions = 0, rows = 0;
	for(int p = POS_NONE; p < POS_COUNT; p++) {
		if(!t || !t->pos_rate[p].n) continue;
		rows++;
		if(p != POS_NONE) set_positions++;
	}
	if(set_positions)
		y += line + rows * line + line * 1.2;
	else
		y += line * 1.2;
	return y;
}

/* The Trend page in a width x height area at the origin of c: the statistics
 * since the start and per position, then the rate, amplitude and beat error
 * graphs on a shared time axis (each part only if asked for in o->sections) */
void render_trend(cairo_t *c, double width, double height, const struct trend *t,
	const struct trend_render_options *o, const struct render_theme *th)
{
	const double font = o->font > 0 ? o->font : 12;
	cairo_save(c);
	cairo_rectangle(c, 0, 0, width, height);
	cairo_clip(c);
	render_set_rgb(c, th->background, 1);
	cairo_paint(c);
	cairo_set_line_width(c, th->line_scale);

	if(!t) {
		trend_message(c, th, width, height, font, _("No long-term statistics"),
			_("This snapshot was saved without them."));
		goto done;
	}
	if(!t->count) {
		if(o->paused)
			/* TRANSLATORS: Trend page of the real-time tab when not recording */
			trend_message(c, th, width, height, font, _("Paused"),
				_("Press Record to start a new run."));
		else if(o->calibrating)
			trend_message(c, th, width, height, font, _("Not recorded during calibration"), NULL);
		else if(t->frozen)
			trend_message(c, th, width, height, font, _("No readings were recorded"), NULL);
		else
			trend_message(c, th, width, height, font, _("Waiting for a steady signal"),
				_("A reading is recorded every 2 s while the signal is good."));
		goto done;
	}

	char s[96];
	int i;
	const double line = font * 1.35;
	const double elapsed = trend_elapsed(t);
	const struct trend_sample *last = &t->samples[t->count - 1];
	double y = font + 4;
	char d[32];

	if(o->sections & TREND_RENDER_STATS) {
	// Statistics since the start, in a table of limited width, centred: on a
	// wide area (or a page) the columns do not drift apart
	const double table_w = fmin(width - 8, font * 42);
	const double x0 = (width - table_w) / 2;
	const double label_w = table_w * 0.24;
	double col_w = (table_w - label_w - 6) / 6;
	/* The last head is the sigma symbol: not translated, it is not marked */
	static const char *heads[6] = {
		/* TRANSLATORS: Trend page column head: the latest reading. Short, shown in a narrow column */
		N_("now"),
		/* TRANSLATORS: Trend page column head: average. Short, shown in a narrow column */
		N_("avg"),
		/* TRANSLATORS: Trend page column head: minimum. Short, shown in a narrow column */
		N_("min"),
		/* TRANSLATORS: Trend page column head: maximum. Short, shown in a narrow column */
		N_("max"),
		/* TRANSLATORS: Trend page column head: difference between maximum and minimum. Short, shown in a narrow column */
		N_("spread"),
		"\xCF\x83"
	};
	cairo_set_font_size(c, font * 0.8);
	render_set_rgb(c, th->axis, 0.6);
	for(i = 0; i < 6; i++)
		trend_text(c, x0 + label_w + (i + 1) * col_w, y, 1, _(heads[i]));
	y += line;

	const struct {
		const char *name;
		struct render_rgb col;
		const struct trend_stats *st;
		double now;
		int digits, sign;
	} rows[3] = {
		/* TRANSLATORS: Trend page row: rate in seconds per day (s/d). Short, shown in a narrow column */
		{N_("Rate s/d"), th->rate, &t->rate, last->rate, 1, 1},
		/* TRANSLATORS: Trend page row: amplitude in degrees. Keep the degree sign. Short, shown in a narrow column */
		{N_("Ampl. \xC2\xB0"), th->amp, &t->amp, last->amp, 0, 0},
		/* TRANSLATORS: Trend page row: beat error in milliseconds (ms). Short, shown in a narrow column */
		{N_("Beat err. ms"), th->be, &t->be, last->be, 1, 0},
	};
	cairo_set_font_size(c, font);
	for(int r = 0; r < 3; r++) {
		const struct trend_stats *st = rows[r].st;
		render_set_rgb(c, rows[r].col, 1);
		trend_text(c, x0 + 4, y, 0, _(rows[r].name));
		render_set_rgb(c, th->axis, 0.9);
		double values[6] = {rows[r].now, trend_stats_mean(st), st->min, st->max,
			st->max - st->min, trend_stats_sd(st)};
		for(i = 0; i < 6; i++) {
			// The amplitude is not always available
			if((i != 0 && !st->n) || (i == 0 && r == 1 && rows[r].now <= 0))
				snprintf(s, sizeof(s), "\xE2\x80\x94");
			else if(i < 4 && rows[r].sign)
				snprintf(s, sizeof(s), "%+.*f", rows[r].digits, values[i]);
			else
				snprintf(s, sizeof(s), "%.*f", rows[r].digits + (i == 5), values[i]);
			trend_text(c, x0 + label_w + (i + 1) * col_w, y, 1, s);
		}
		y += line;
	}

	// How long, how much of it with a signal, and the settings used
	format_duration(d, sizeof(d), elapsed);
	double coverage = elapsed > 0 ? fmin(100, t->count * TREND_INTERVAL_S * 100.0 / elapsed) : 100;
	cairo_set_font_size(c, font * 0.8);
	render_set_rgb(c, th->axis, 0.7);
	/* TRANSLATORS: Trend page. %s is a duration such as 2:15:03, the number is a percentage */
	snprintf(s, sizeof(s), t->full ? _("%s since the start, %.0f %% with signal (100 h limit reached)")
		: _("%s since the start, %.0f %% with signal"), d, coverage);
	trend_text(c, x0 + 4, y, 0, s);
	y += line * 0.85;
	/* TRANSLATORS: Trend page: the settings used for the readings. "cal" is the calibration value and stays as is */
	snprintf(s, sizeof(s), _("%d bph, lift angle %.0f\xC2\xB0, cal %+.1f s/d"), t->bph, t->la, t->cal / 10.0);
	trend_text(c, x0 + 4, y, 0, s);
	y += line * 1.2;

	// Per position, if positions were set
	int set_positions = 0;
	for(i = POS_NONE + 1; i < POS_COUNT; i++)
		if(t->pos_rate[i].n)
			set_positions++;
	if(set_positions) {
		col_w = (table_w - label_w - 6) / 4;
		static const char *pos_heads[4] = {
		/* TRANSLATORS: Trend page column head: time spent in this position (duration). Short, shown in a narrow column */
		N_("time"),
		/* TRANSLATORS: Trend page column head: average rate in this position. Short, shown in a narrow column */
		N_("rate"),
		/* TRANSLATORS: Trend page column head: average amplitude in this position. Short, shown in a narrow column */
		N_("ampl."),
		/* TRANSLATORS: Trend page column head: beat error in this position. Short, shown in a narrow column */
		N_("BE")
	};
		render_set_rgb(c, th->axis, 0.6);
		/* TRANSLATORS: Trend page column head over the list of watch positions. Short, shown in a narrow column */
		trend_text(c, x0 + 4, y, 0, _("Position"));
		for(i = 0; i < 4; i++)
			trend_text(c, x0 + label_w + (i + 1) * col_w, y, 1, _(pos_heads[i]));
		y += line;
		cairo_set_font_size(c, font);
		double best = 0, worst = 0;
		int first = 1;
		for(int p = POS_NONE; p < POS_COUNT; p++) {
			if(!t->pos_rate[p].n) continue;
			const char *code = watch_position_code(p);
			render_set_rgb(c, th->axis, code ? 0.9 : 0.6);
			/* TRANSLATORS: Trend page: readings taken while no watch position was set. Short, shown in a narrow column */
			trend_text(c, x0 + 4, y, 0, code ? code : _("not set"));
			format_duration(d, sizeof(d), t->pos_rate[p].n * TREND_INTERVAL_S);
			trend_text(c, x0 + label_w + col_w, y, 1, d);
			double rate = trend_stats_mean(&t->pos_rate[p]);
			snprintf(s, sizeof(s), "%+.1f", rate);
			render_set_rgb(c, th->rate, 1);
			trend_text(c, x0 + label_w + 2 * col_w, y, 1, s);
			if(t->pos_amp[p].n)
				snprintf(s, sizeof(s), "%.0f", trend_stats_mean(&t->pos_amp[p]));
			else
				snprintf(s, sizeof(s), "\xE2\x80\x94");
			render_set_rgb(c, th->amp, 1);
			trend_text(c, x0 + label_w + 3 * col_w, y, 1, s);
			snprintf(s, sizeof(s), "%.1f", trend_stats_mean(&t->pos_be[p]));
			render_set_rgb(c, th->be, 1);
			trend_text(c, x0 + label_w + 4 * col_w, y, 1, s);
			y += line;
			if(code) {
				if(first || rate > best) best = rate;
				if(first || rate < worst) worst = rate;
				first = 0;
			}
		}
		cairo_set_font_size(c, font * 0.8);
		render_set_rgb(c, th->axis, 0.7);
		if(set_positions >= 2)
			snprintf(s, sizeof(s), _("Positional error (largest difference in rate): %.1f s/d"), best - worst);
		else
			snprintf(s, sizeof(s), "%s", _("Measure more positions to see the positional error."));
		trend_text(c, x0 + 4, y, 0, s);
		y += line * 1.2;
	} else {
		cairo_set_font_size(c, font * 0.8);
		render_set_rgb(c, th->axis, 0.6);
		trend_text(c, x0 + 4, y, 0, _("Set the watch position to also get statistics per position."));
		y += line * 1.2;
	}
	} else
		y = font * 0.5;	// graphs only: they start at the top

	if(!(o->sections & TREND_RENDER_GRAPHS))
		goto done;

	// Graphs share a time axis: rate, amplitude, then beat error
	const double area_top = y;
	const double area_bottom = height - (font + 6);
	if(area_bottom - area_top < 11 * font)
		goto done;	// not enough room
	const double gx = 3.5 * font, gw = width - gx - 8;
	const double graph_height = (area_bottom - area_top) / 3;
	const double split1 = area_top + graph_height;
	const double split2 = area_top + 2 * graph_height;
	const double rate_top = area_top + font * 1.2, rate_bottom = split1 - font * 0.5;
	const double amp_top = split1 + font * 1.2, amp_bottom = split2 - font * 0.5;
	const double be_top = split2 + font * 1.2, be_bottom = area_bottom;

	double t1 = elapsed, t0 = 0;
	if(o->span_s > 0) {
		t1 = fmax(elapsed, o->span_s);
		t0 = t1 - o->span_s;
	} else if(t1 < 60)
		t1 = 60;

	int max_cols = (int)gw * 4 + 16;
	struct trend_column *cols = malloc(max_cols * sizeof(*cols));
	if(!cols)
		goto done;

	// Time grid and labels, a round step giving labels at least 5 characters apart
	static const int time_steps[] = {10, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200,
		10800, 21600, 43200, 86400, 0};
	int step = 86400;
	for(i = 0; time_steps[i]; i++)
		if((t1 - t0) / time_steps[i] <= gw / (font * 4.5)) {
			step = time_steps[i];
			break;
		}
	cairo_set_font_size(c, font * 0.8);
	// Settling after a position change: kept, but tinted and not in the statistics
	cairo_save(c);
	cairo_rectangle(c, gx, rate_top, gw, be_bottom - rate_top);
	cairo_clip(c);
	render_set_rgb(c, th->toc, 0.14);
	for(i = 0; i < t->count; i++) {
		const struct trend_sample *sample = &t->samples[i];
		if(!sample->settling || sample->t + TREND_INTERVAL_S / 2.0 < t0 ||
				sample->t - TREND_INTERVAL_S / 2.0 > t1)
			continue;
		double x = gx + (sample->t - TREND_INTERVAL_S / 2.0 - t0) / (t1 - t0) * gw;
		double sample_width = fmax(1, TREND_INTERVAL_S / (t1 - t0) * gw);
		cairo_rectangle(c, x, rate_top, sample_width, be_bottom - rate_top);
	}
	cairo_fill(c);
	cairo_restore(c);
	cairo_set_line_width(c, th->line_scale);
	for(double tm = ceil(t0 / step) * step; tm <= t1 + 1e-6; tm += step) {
		double x = round(gx + (tm - t0) / (t1 - t0) * gw) + .5;
		cairo_move_to(c, x, rate_top);
		cairo_line_to(c, x, rate_bottom);
		cairo_move_to(c, x, amp_top);
		cairo_line_to(c, x, amp_bottom);
		cairo_move_to(c, x, be_top);
		cairo_line_to(c, x, be_bottom);
		theme_axis_style(c, th, 0);
		cairo_stroke(c);
		set_dotted(c, 0);
		format_duration(d, sizeof(d), tm);
		render_set_rgb(c, th->axis, 0.7);
		trend_text(c, x, height - 4, 0.5, d);
	}

	// Rate: fixed range or the smallest round one that holds the readings shown
	int n = trend_columns(t, t0, t1, gx, gw, TREND_SERIES_RATE, cols, max_cols);
	struct trend_plot_ranges ranges;
	trend_get_plot_ranges(t, t0, t1, o->rate_range, o->be_range, &ranges);
	double range = ranges.rate_max;
	for(int k = -2; k <= 2; k++) {
		double v = range * k / 2;
		snprintf(s, sizeof(s), k ? "%+g" : "0", v);
		trend_hline(c, th, gx, gw, rate_bottom - (v + range) / (2 * range) * (rate_bottom - rate_top), k == 0, s);
	}
	double mean = trend_stats_mean(&t->rate);
	trend_draw_average(c, th, gx, gw, rate_top, rate_bottom, -range, range, &t->rate, th->rate);
	int off = trend_draw_series(c, cols, n, gx, gw, rate_top, rate_bottom, -range, range, th->rate, th);
	if(t->rate.n)
		snprintf(s, sizeof(s), _("Rate (s/d), dashed: average %+.1f"), mean);
	else
		snprintf(s, sizeof(s), "%s", _("Rate (s/day)"));
	trend_graph_title(c, th, gx, gw, rate_top - font * 0.4, font, th->rate, s, off);

	// Expand the amplitude scale for readings below the usual 120 degrees.
	cairo_set_font_size(c, font * 0.8);
	int amp_floor = (int)ranges.amp_min;
	for(int v = amp_floor; v <= 360; v += 60) {
		snprintf(s, sizeof(s), "%d", v);
		trend_hline(c, th, gx, gw, amp_bottom - (v - amp_floor) / (double)(360 - amp_floor) * (amp_bottom - amp_top), 0, s);
	}
	n = trend_columns(t, t0, t1, gx, gw, TREND_SERIES_AMPLITUDE, cols, max_cols);
	double amp_mean = trend_stats_mean(&t->amp);
	trend_draw_average(c, th, gx, gw, amp_top, amp_bottom, amp_floor, 360, &t->amp, th->amp);
	off = trend_draw_series(c, cols, n, gx, gw, amp_top, amp_bottom, amp_floor, 360, th->amp, th);
	if(t->amp.n)
		snprintf(s, sizeof(s), _("Amplitude (\xC2\xB0), dashed: average %.1f"), amp_mean);
	else
		snprintf(s, sizeof(s), "%s", _("Amplitude (degrees)"));
	trend_graph_title(c, th, gx, gw, amp_top - font * 0.4, font, th->amp, s, off);

	// Beat error: fixed range or the smallest round one that holds the readings shown
	n = trend_columns(t, t0, t1, gx, gw, TREND_SERIES_BEAT_ERROR, cols, max_cols);
	double be_range = ranges.be_max;
	cairo_set_font_size(c, font * 0.8);
	for(int k = 0; k <= 4; k++) {
		double v = be_range * k / 4;
		snprintf(s, sizeof(s), "%g", v);
		trend_hline(c, th, gx, gw, be_bottom - v / be_range * (be_bottom - be_top), k == 0, s);
	}
	double be_mean = trend_stats_mean(&t->be);
	trend_draw_average(c, th, gx, gw, be_top, be_bottom, 0, be_range, &t->be, th->be);
	off = trend_draw_series(c, cols, n, gx, gw, be_top, be_bottom, 0, be_range, th->be, th);
	if(t->be.n)
		snprintf(s, sizeof(s), _("Beat error (ms), dashed: average %.1f"), be_mean);
	else
		snprintf(s, sizeof(s), "%s", _("Beat error (ms)"));
	trend_graph_title(c, th, gx, gw, be_top - font * 0.4, font, th->be, s, off);
	free(cols);

	// Position changes: a line through all three graphs, with the new position
	cairo_set_font_size(c, font * 0.8);
	cairo_set_line_width(c, th->line_scale);
	int prev_pos = -1;
	for(i = 0; i < t->count; i++) {
		const struct trend_sample *x = &t->samples[i];
		if(x->t < t0 || x->t > t1) continue;
		if(x->position != prev_pos) {
			double px = round(gx + (x->t - t0) / (t1 - t0) * gw) + .5;
			if(prev_pos >= 0) {
				cairo_move_to(c, px, rate_top);
				cairo_line_to(c, px, be_bottom);
				render_set_rgb(c, th->toc, 0.8);
				cairo_stroke(c);
			}
			const char *code = watch_position_code(x->position);
			if(code || prev_pos >= 0) {
				render_set_rgb(c, th->toc, 1);
				trend_text(c, px + 3, rate_top + font, 0, code ? code : "\xE2\x80\x94");
			}
			prev_pos = x->position;
		}
	}
done:
	cairo_restore(c);
}

