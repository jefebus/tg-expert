/*
    tg-expert
    Copyright (C) 2015 Marcello Mamino
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

/* TG-31: the averaged waveforms of a snapshot (tic, toc and full period),
 * drawn from the snapshot alone, so that the screen (output_panel.c) and the
 * PDF report share them. The averaged sound shown is that of the snapshot's
 * own analysis window: it is not an average of a whole test. */

#include "tg.h"

#define ALPHA_STALE 0.4	// a stale reading keeps its colour, faded
#define ALPHA_BAND  0.18	// tic / toc windows in the period panel

static void set_dotted(cairo_t *c, int dotted)
{
	static const double dots[2] = {2, 4};
	if(dotted)
		cairo_set_dash(c, dots, 2, 0);
	else
		cairo_set_dash(c, NULL, 0, 0);
}

static void axis_style(cairo_t *c, const struct render_theme *th, struct render_rgb col, int major)
{
	render_set_rgb(c, col, major ? th->axis_major_alpha : th->axis_minor_alpha);
	set_dotted(c, !major);
}

static void signal_color(cairo_t *c, struct render_rgb col, int stale)
{
	render_set_rgb(c, col, stale ? ALPHA_STALE : 1);
}

/* On screen, lines sit on pixel centres to stay sharp; in a document the
 * coordinates are kept exact */
static double snap(const struct render_theme *th, double v)
{
	return th->id == RENDER_SCREEN ? round(v) + .5 : v;
}

/* The middle line: on screen, the pixel row the panels always used */
static double half(const struct render_theme *th, double height)
{
	return th->id == RENDER_SCREEN ? floor(height / 2) + .5 : height / 2;
}

static double amplitude_to_time(double lift_angle, double amp)
{
	return asin(lift_angle / (2 * amp)) / M_PI;
}

/* The envelope between samples a and b of the folded waveform, mirrored
 * around the middle line, as one closed path. Each column takes the largest
 * value of the samples it covers, so that no peak is lost when there are
 * more samples than columns. */
static void waveform_path(cairo_t *c, const struct render_theme *th, double a, double b,
	const struct processing_buffers *p, double width, double height)
{
	int columns = (int)ceil(width);
	if(columns < 1 || p->period <= 0 || p->waveform_max <= 0) return;
	double per_column = (b - a) / columns;
	double *ys = malloc(columns * sizeof(*ys));
	if(!ys) return;
	for(int i = 0; i < columns; i++) {
		double from = a + i * per_column;
		int count = per_column > 1 ? (int)ceil(per_column) : 1;
		float v = 0;
		for(int k = 0; k < count; k++) {
			double x = fmod(from + k, p->period);
			if(x < 0) x += p->period;
			int j = (int)floor(x);
			if(j >= 0 && j < p->sample_count && p->waveform[j] > v)
				v = p->waveform[j];
		}
		ys[i] = v * 0.4 / p->waveform_max * height;
		if(th->id == RENDER_SCREEN)
			ys[i] = round(ys[i]);
	}
	double mid = half(th, height);
	for(int n = 0; n < 2 * columns; n++) {
		int i = n < columns ? n : 2 * columns - 1 - n;
		double x = (i + .5) * width / columns;
		double y = mid + (n < columns ? -ys[i] : ys[i]);
		if(n == 0)
			cairo_move_to(c, x, y);
		else
			cairo_line_to(c, x, y);
	}
	free(ys);
}

/* Vertical grid with ms labels for a window starting at t_left ms and span_ms wide.
 * Major (solid, labelled) lines use a round step giving at most ~10 labels; minor
 * (dotted) lines divide each major step in 5. */
static void draw_ms_axis(cairo_t *c, const struct render_theme *th, double width, double height,
	double font, double t_left, double span_ms)
{
	static const int majors[] = {5, 10, 20, 25, 50, 100, 200, 250, 500, 0};
	int major = 1000;
	int i;
	if(span_ms <= 0 || width <= 0) return;
	for(i = 0; majors[i]; i++) {
		if(span_ms / majors[i] <= 10) {
			major = majors[i];
			break;
		}
	}
	double minor = major / 5.0;
	long k0 = (long)ceil(t_left / minor);
	long k1 = (long)floor((t_left + span_ms) / minor);
	long k;

	cairo_set_line_width(c, th->line_scale);
	for(k = k0; k <= k1; k++) {
		double x = (k * minor - t_left) * width / span_ms;
		if(x <= 0 || x >= width) continue;
		cairo_move_to(c, snap(th, x), snap(th, 0));
		cairo_line_to(c, snap(th, x), height - (th->id == RENDER_SCREEN ? .5 : 0));
		axis_style(c, th, th->axis, !(k % 5));
		cairo_stroke(c);
	}
	set_dotted(c, 0);

	cairo_set_font_size(c, font);
	cairo_text_extents_t extents;
	cairo_text_extents(c, "ms", &extents);
	double unit_x = width - extents.x_advance - font/4;

	render_set_rgb(c, th->axis, 1);
	for(k = k0; k <= k1; k++) {
		if(k % 5) continue;
		double x = (k * minor - t_left) * width / span_ms;
		if(x <= 0 || x >= width) continue;
		char s[16];
		snprintf(s, sizeof(s), "%ld", lround(k * minor));
		cairo_text_extents(c, s, &extents);
		// keep labels clear of the "ms" unit at the right edge
		if(round(x) + font/4 + extents.x_advance > unit_x - font/2) continue;
		cairo_move_to(c, round(x) + font/4, height - font/2);
		cairo_show_text(c, s);
	}

	cairo_move_to(c, unit_x, height - font/2);
	cairo_show_text(c, "ms");
}

/* Tic or toc panel: the averaged sound of one beat, the amplitude scale on
 * top, the ms axis below and the detected lift pulse */
static void render_beat(cairo_t *c, double width, double height, const struct snapshot *snst,
	const struct waveform_options *o, const struct render_theme *th)
{
	const double font = o->font;
	int i;
	const struct processing_buffers *p = snst->rate_only ? NULL : snst->pb;
	int negative_span = o->negative_span > 0 ? o->negative_span : NEGATIVE_SPAN;
	int total_span = negative_span + POSITIVE_SPAN;
	int is_toc = o->kind == WAVEFORM_TOC;
	static const int steps[] = {5, 10, 20, 25, 50, 100, 200, 500};
	int step = 0;
	// Labelled lines at least 40 px apart on screen; in a document, in
	// proportion to the text size
	double min_gap = th->id == RENDER_SCREEN ? 40 : 40 * font / 12;
	while(step < (int)G_N_ELEMENTS(steps) - 1 &&
			steps[step] * width / total_span < min_gap)
		step++;
	int major = steps[step];
	int minor = major / 5;

	cairo_set_font_size(c, font);
	cairo_set_line_width(c, th->line_scale);

	// Time axis (bottom half)
	for(i = -(negative_span / minor) * minor; i < POSITIVE_SPAN; i += minor) {
		if(i <= -negative_span) continue;
		double x = (negative_span + i) * width / total_span;
		if(th->id == RENDER_SCREEN) x = (int)x;
		cairo_move_to(c, snap(th, x), half(th, height));
		cairo_line_to(c, snap(th, x), height - (th->id == RENDER_SCREEN ? .5 : 0));
		axis_style(c, th, th->axis, !(i % major));
		cairo_stroke(c);
	}
	set_dotted(c, 0);
	render_set_rgb(c, th->axis, 1);
	for(i = -(negative_span / major) * major; i < POSITIVE_SPAN; i += major) {
		if(i > -negative_span) {
			double x = (negative_span + i) * width / total_span;
			if(th->id == RENDER_SCREEN) x = (int)x;
			char s[10];
			snprintf(s, sizeof(s), "%d", i);
			cairo_move_to(c, x + font/4, height - font/2);
			cairo_show_text(c, s);
		}
	}

	cairo_text_extents_t extents;
	cairo_text_extents(c, "ms", &extents);
	cairo_move_to(c, width - extents.x_advance - font/4, height - font/2);
	cairo_show_text(c, "ms");

	int old = snst->is_old;
	double period = p ? p->period / snst->sample_rate : 7200. / snst->guessed_bph;

	// Amplitude scale (top half): solid every 50 deg, dotted every 10 deg
	for(i = 10; i < 360; i += 10) {
		if(2 * i < snst->la) continue;
		double t = period * amplitude_to_time(snst->la, i);
		if(t > .001 * negative_span) continue;
		double x = width * (negative_span - 1000 * t) / total_span;
		if(th->id == RENDER_SCREEN) x = round(x);
		cairo_move_to(c, snap(th, x), snap(th, 0));
		cairo_line_to(c, snap(th, x), half(th, height));
		axis_style(c, th, th->amp, !(i % 50));
		cairo_stroke(c);
	}
	set_dotted(c, 0);

	double last_x = 0;
	render_set_rgb(c, th->amp, 1);
	for(i = 50; i < 360; i += 50) {
		double t = period * amplitude_to_time(snst->la, i);
		if(t > .001 * negative_span) continue;
		double x = width * (negative_span - 1000 * t) / total_span;
		if(th->id == RENDER_SCREEN) x = round(x);
		if(x > last_x) {
			char s[10];
			snprintf(s, sizeof(s), "%d", abs(i));
			cairo_move_to(c, x + font/4, font * 3 / 2);
			cairo_show_text(c, s);
			cairo_text_extents(c, s, &extents);
			last_x = x + font/4 + extents.x_advance;
		}
	}

	cairo_text_extents(c, "deg", &extents);
	cairo_move_to(c, width - extents.x_advance - font/4, font * 3 / 2);
	cairo_show_text(c, "deg");

	if(p) {
		double span = 0.001 * snst->sample_rate;
		int offset = is_toc ? p->toc : p->tic;

		double a = offset - span * negative_span;
		double b = offset + span * POSITIVE_SPAN;

		waveform_path(c, th, a, b, p, width, height);
		cairo_set_line_width(c, th->line_scale);
		signal_color(c, is_toc ? th->toc : th->tic, old);
		cairo_stroke_preserve(c);
		cairo_fill(c);

		// Detected pulse position: reference colour
		double pulse = is_toc ? p->toc_pulse : p->tic_pulse;
		if(pulse > 0) {
			double x = (negative_span - pulse / span) * width / total_span;
			if(th->id == RENDER_SCREEN) x = round(x);
			cairo_move_to(c, x, 1);
			cairo_line_to(c, x, height - 1);
			signal_color(c, th->rate, old);
			cairo_set_line_width(c, 2 * th->line_scale);
			cairo_stroke(c);
		}
	} else {
		cairo_move_to(c, snap(th, 0), half(th, height));
		cairo_line_to(c, width - (th->id == RENDER_SCREEN ? .5 : 0), half(th, height));
		render_set_rgb(c, th->axis, ALPHA_STALE);
		cairo_stroke(c);
	}
}

/* Full period panel: tic and toc, with their windows tinted */
static void render_period(cairo_t *c, double width, double height, const struct snapshot *snst,
	const struct waveform_options *o, const struct render_theme *th)
{
	const struct processing_buffers *p = snst->rate_only ? NULL : snst->pb;
	int old = snst->is_old;
	double toc, a = 0, b = 0;
	int negative_span = o->negative_span > 0 ? o->negative_span : NEGATIVE_SPAN;

	if(p) {
		toc = p->tic < p->toc ? p->toc : p->toc + p->period;
		a = ((double)p->tic + toc)/2 - p->period/2;
		b = ((double)p->tic + toc)/2 + p->period/2;
		double before = negative_span * .001 * snst->sample_rate;
		double after = POSITIVE_SPAN * .001 * snst->sample_rate;

		cairo_rectangle(c, (p->tic - a - before) * width / p->period, 0,
			(before + after) * width / p->period, height);
		render_set_rgb(c, th->tic, ALPHA_BAND); // tic window
		cairo_fill(c);

		cairo_rectangle(c, (toc - a - before) * width / p->period, 0,
			(before + after) * width / p->period, height);
		render_set_rgb(c, th->toc, ALPHA_BAND); // toc window
		cairo_fill(c);
	}

	// Time axis in ms, 0 at the tic (same convention as the tic/toc panels)
	double sr = snst->sample_rate > 0 ? snst->sample_rate : snst->nominal_sr;
	double span_ms = p ? 1000. * p->period / sr : 1000. * 7200. / snst->guessed_bph;
	double t_left = p ? 1000. * (a - p->tic) / sr : 0;
	draw_ms_axis(c, th, width, height, o->font, t_left, span_ms);

	cairo_set_line_width(c, th->line_scale);
	if(p) {
		waveform_path(c, th, a, b, p, width, height);
		signal_color(c, th->wave, old);
		cairo_stroke_preserve(c);
		cairo_fill(c);
	} else {
		cairo_move_to(c, snap(th, 0), half(th, height));
		cairo_line_to(c, width - (th->id == RENDER_SCREEN ? .5 : 0), half(th, height));
		render_set_rgb(c, th->axis, ALPHA_STALE);
		cairo_stroke(c);
	}
}

/* One item of the waveform legend: a filled swatch or a line, then its text */
static double legend_item(cairo_t *c, double x, double y, double font, int line,
	struct render_rgb col, double alpha, const char *text, const struct render_theme *th)
{
	double w = font * 1.4, h = font * 0.7;
	cairo_new_path(c);
	if(line) {
		cairo_set_line_width(c, 2 * th->line_scale);
		cairo_move_to(c, x, y - h / 2);
		cairo_line_to(c, x + w, y - h / 2);
		render_set_rgb(c, col, alpha);
		cairo_stroke(c);
	} else {
		cairo_rectangle(c, x, y - h, w, h);
		render_set_rgb(c, col, alpha);
		cairo_fill(c);
	}
	render_set_rgb(c, th->text, 1);
	render_text(c, x + w + font * 0.4, y, 0, text);
	cairo_text_extents_t e;
	cairo_text_extents(c, text, &e);
	return x + w + font * 0.4 + e.x_advance + font * 1.5;
}

/* Legend of the waveform panels on one line from (x, y) (baseline), in the
 * colours of the theme. Returns the x after the last item. */
double render_waveform_legend(cairo_t *c, const struct render_theme *th, double x, double y,
	double font)
{
	cairo_save(c);
	cairo_set_font_size(c, font);
	/* TRANSLATORS: legend of the waveform graphs in the report: the first sound of a beat. Short */
	x = legend_item(c, x, y, font, 0, th->tic, 1, _("tic"), th);
	/* TRANSLATORS: legend of the waveform graphs in the report: the second sound of a beat. Short */
	x = legend_item(c, x, y, font, 0, th->toc, 1, _("toc"), th);
	/* TRANSLATORS: legend of the waveform graphs: the sound of a whole period (tic and toc). Short */
	x = legend_item(c, x, y, font, 0, th->wave, 1, _("full period"), th);
	/* TRANSLATORS: legend of the waveform graphs: the line at the detected lift pulse. Short */
	x = legend_item(c, x, y, font, 1, th->rate, 1, _("lift pulse"), th);
	/* TRANSLATORS: legend of the waveform graphs: the amplitude scale in degrees at the top. Short */
	x = legend_item(c, x, y, font, 1, th->amp, th->axis_major_alpha, _("amplitude scale (deg)"), th);
	cairo_restore(c);
	return x;
}

void render_waveform(cairo_t *c, double width, double height, const struct snapshot *s,
	const struct waveform_options *o, const struct render_theme *th)
{
	struct waveform_options opt = *o;
	if(opt.font <= 0) opt.font = 12;
	cairo_save(c);
	cairo_rectangle(c, 0, 0, width, height);
	cairo_clip(c);
	render_set_rgb(c, th->background, 1);
	cairo_paint(c);
	cairo_set_line_width(c, th->line_scale);
	if(s) {
		if(opt.kind == WAVEFORM_PERIOD)
			render_period(c, width, height, s, &opt, th);
		else
			render_beat(c, width, height, s, &opt, th);
	}
	cairo_restore(c);
}
