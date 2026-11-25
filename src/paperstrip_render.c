/*
    tg-expert
    Copyright (C) 2015 Marcello Mamino
    Copyright (C) 2026 jefebus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

/* TG-31: the paper strip of a snapshot, drawn from the snapshot and its
 * settings alone, so that the screen (output_panel.c) and the PDF report
 * share it. What only the live display has (centring the trace when it
 * appears, the filtered rate of the guide lines) is worked out by the caller
 * and passed in the options.
 *
 * Layout (agreed with the user): newest beats at the bottom, older ones move
 * up; amplitude and ms scales on top, the ms ruler mirrored so that a fast
 * watch leans right like "/"; the legend at the bottom. */

#include "tg.h"

#define ALPHA_STALE 0.4

/* Paperstrip zoom levels (trace_zoom). At 1x the window is PAPERSTRIP_WINDOW_MS
 * wide (PAPERSTRIP_WINDOW_MS_CAL in calibration), at 2x half of it, and so on. */
const double paperstrip_zoom_levels[] = {0.25, 0.5, 1, 2, 4, 8, 0};

/* The level nearest to a saved trace_zoom (older versions stored other factors) */
int paperstrip_zoom_index(double trace_zoom)
{
	double z = trace_zoom > 0 ? trace_zoom : 1.0;
	int best = 0;
	for(int i = 1; paperstrip_zoom_levels[i]; i++)
		if(fabs(log(paperstrip_zoom_levels[i] / z)) < fabs(log(paperstrip_zoom_levels[best] / z)))
			best = i;
	return best;
}

/* Beat period (a second in calibration) divided by the strip width. The width
 * is the same for every bph; beats wrap at its edges. It cannot be
 * wider than one beat, so at 0.25x a fast beat fills the strip. */
double paperstrip_zoom_factor(const struct snapshot *snst)
{
	double zoom = paperstrip_zoom_levels[paperstrip_zoom_index(snst->trace_zoom)];
	int bph = snst->guessed_bph > 0 ? snst->guessed_bph : (snst->bph ? snst->bph : DEFAULT_BPH);
	double period_ms = snst->calibrate ? 1000. : 3600000. / bph;
	double wanted_ms = (snst->calibrate ? PAPERSTRIP_WINDOW_MS_CAL : PAPERSTRIP_WINDOW_MS) / zoom;
	double n = period_ms / wanted_ms;
	return n < 1 ? 1 : n;
}

static void set_dotted(cairo_t *c, int dotted)
{
	static const double dots[2] = {2, 4};
	if(dotted)
		cairo_set_dash(c, dots, 2, 0);
	else
		cairo_set_dash(c, NULL, 0, 0);
}

static void axis_style(cairo_t *c, const struct render_theme *th, int major)
{
	render_set_rgb(c, th->axis, major ? th->axis_major_alpha : th->axis_minor_alpha);
	set_dotted(c, !major);
}

static void signal_color(cairo_t *c, struct render_rgb col, int stale)
{
	render_set_rgb(c, col, stale ? ALPHA_STALE : 1);
}

#define LEGEND_DOT  0
#define LEGEND_LINE 1
#define LEGEND_DASH 2

/* One legend entry (marker and text) at baseline y; returns where the next
 * one starts. The markers keep their screen size in pixels; in a document
 * they follow the text size. */
static double legend_item(cairo_t *c, const struct render_theme *th, double k, double x, double y,
	int style, struct render_rgb col, const char *text)
{
	static const double dashes[2] = {4, 3};
	cairo_text_extents_t e;

	cairo_new_path(c);
	render_set_rgb(c, col, 1);
	if(style == LEGEND_DOT) {
		cairo_arc(c, x + 3 * k, y - 3.5 * k, 3 * k, 0, 2 * M_PI);
		cairo_fill(c);
		x += 10 * k;
	} else {
		cairo_set_line_width(c, 1.5 * th->line_scale);
		if(style == LEGEND_DASH)
			cairo_set_dash(c, dashes, 2, 0);
		cairo_move_to(c, x, y - 3.5 * k);
		cairo_line_to(c, x + 16 * k, y - 3.5 * k);
		cairo_stroke(c);
		set_dotted(c, 0);
		x += 20 * k;
	}
	render_set_rgb(c, th->axis, 0.85);
	cairo_text_extents(c, text, &e);
	cairo_move_to(c, x, y);
	cairo_show_text(c, text);
	return x + e.x_advance + 14 * k;
}

/* The legend lists the rate guide only when it is drawn (guide = 1) */
double render_paperstrip_legend(cairo_t *c, const struct render_theme *th, double x, double y,
	double font, int calibrate, int guide)
{
	cairo_save(c);
	cairo_set_font_size(c, font);
	double k = th->id == RENDER_SCREEN ? 1 : font / 10.8;
	if(calibrate) {
		/* TRANSLATORS: legend of the paper strip, short, shown in a narrow row */
		x = legend_item(c, th, k, x, y, LEGEND_DOT, th->wave, _("reference tick"));
		/* TRANSLATORS: legend of the paper strip, short, shown in a narrow row. "cal" is the calibration value and stays as is */
		x = legend_item(c, th, k, x, y, LEGEND_DASH, th->rate, _("current cal"));
	} else {
		/* TRANSLATORS: legend of the paper strip: the first sound of a watch beat (tic-toc). Short, shown in a narrow row */
		x = legend_item(c, th, k, x, y, LEGEND_DOT, th->tic, _("tic"));
		/* TRANSLATORS: legend of the paper strip: the second sound of a watch beat (tic-toc). Short, shown in a narrow row */
		x = legend_item(c, th, k, x, y, LEGEND_DOT, th->toc, _("toc"));
		/* TRANSLATORS: legend of the paper strip, short, shown in a narrow row */
		x = legend_item(c, th, k, x, y, LEGEND_LINE, th->amp, _("amplitude"));
		/* TRANSLATORS: legend of the paper strip: dashed guide line showing the rate. Short, shown in a narrow row */
		if(guide)
			x = legend_item(c, th, k, x, y, LEGEND_DASH, th->rate, _("rate guide"));
	}
	cairo_restore(c);
	return x;
}

void render_paperstrip(cairo_t *c, double area_width, double area_height,
	const struct snapshot *snst, const struct paperstrip_options *o,
	const struct render_theme *th)
{
	int i;
	const int valid_events_wp =
		snst->events &&
		snst->events_count > 0 &&
		snst->events_wp >= 0 &&
		snst->events_wp < snst->events_count;
	uint64_t time = o->now;
	double sweep;
	double zoom_factor;
	double safe_sample_rate = snst->sample_rate > 0 ? snst->sample_rate : snst->nominal_sr;
	int safe_bph = snst->guessed_bph > 0 ? snst->guessed_bph : (snst->bph ? snst->bph : DEFAULT_BPH);
	double slope = 1000; // detected rate: 1000 -> do not display
	if(snst->calibrate) {
		sweep = snst->nominal_sr;
		zoom_factor = paperstrip_zoom_factor(snst);
		slope = (double) snst->cal * zoom_factor / (10 * 3600 * 24);
	} else {
		sweep = safe_sample_rate * 3600. / safe_bph;
		zoom_factor = paperstrip_zoom_factor(snst);
		if(isfinite(o->guide_rate))
			slope = - o->guide_rate * zoom_factor / (3600. * 24.);
	}
	if(!isfinite(sweep) || sweep <= 0 || !isfinite(zoom_factor) || zoom_factor <= 0)
		return;

	cairo_save(c);
	cairo_rectangle(c, 0, 0, area_width, area_height);
	cairo_clip(c);
	render_set_rgb(c, th->background, 1);
	cairo_paint(c);
	cairo_set_line_width(c, th->line_scale);

	// Whole units, as on screen, where the strip has always been laid out in pixels
	int width = (int)area_width;
	int height = (int)area_height;

	int stopped = 0;
	if(valid_events_wp && snst->events[snst->events_wp] &&
		time > 5 * snst->nominal_sr + snst->events[snst->events_wp])
		stopped = 1;

	int strip_width = round(width / (1 + PAPERSTRIP_MARGIN));
	if(strip_width <= 0)
		goto done;
	int left_margin = (width - strip_width) / 2;
	int right_margin = (width + strip_width) / 2;

	const double font = o->font > 0 ? o->font : 12;

	/* Layout: scales on top, the data in the middle with the newest beats at
	 * its bottom edge (older ones move up), legend at the bottom. */
	const int scale_rows = snst->calibrate ? 1 : 2;
	const double plot_top = scale_rows * (font + 4) + 4;
	const double plot_bottom = height - (o->legend ? font + 8 : 4);
	if(plot_bottom - plot_top < 20)
		goto done;
	double y_sample_rate = snst->calibrate ? snst->nominal_sr : safe_sample_rate;
	double row_samples = y_sample_rate * o->window_seconds /
		(plot_bottom - plot_top);
	if(!isfinite(row_samples) || row_samples <= 0)
		goto done;

	// Width of the strip in ms (a beat in normal mode, a second in calibration, divided by the zoom)
	const double window_ms = snst->calibrate ? 1000. / zoom_factor : 3600000. / (safe_bph * zoom_factor);
	double amp_min = 110.0;
	if(snst->amp > 0 && snst->amp < amp_min) amp_min = 80.0;
	for(int j = 0; snst->amps && j < snst->amps_count; j++)
		if(snst->amps[j] > 0 && snst->la * snst->amps[j] < 110) {
			amp_min = 80.0;
			break;
		}
	const double amp_max = 360.0;
	// The beat position runs from right to left, so that a fast watch (positive
	// rate) leans to the right with the newest beats at the bottom
#define RULER_X(t) (right_margin - (t) * strip_width / window_ms)

	// Ruler in ms. Normal mode: PAPERSTRIP_DIVISIONS equal divisions, shared with
	// the amplitude scale (25 deg each), so one set of grid lines serves both.
	// Calibration (no amplitude): the smallest round step giving at most 10 divisions.
	double ruler = window_ms / PAPERSTRIP_DIVISIONS;
	if(snst->calibrate) {
		static const double ruler_steps[] = {0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 0};
		ruler = 100;
		for(i = 0; ruler_steps[i]; i++) {
			if(window_ms / ruler_steps[i] <= 10) {
				ruler = ruler_steps[i];
				break;
			}
		}
	}
	const double amp_step = (amp_max - amp_min) / PAPERSTRIP_DIVISIONS;

	char s[32];
	int k;
	cairo_text_extents_t extents;

	double now = sweep*ceil(time/sweep);
	double mark_samples = o->tick_seconds * y_sample_rate;
	double mark_rows = mark_samples / row_samples;
	if(!isfinite(mark_rows) || mark_rows <= 0)
		goto done;
	int64_t last_mark_index = (int64_t)floor(now / mark_samples);
	int64_t mark_stride = (int64_t)ceil(1.0 / mark_rows);
	if(mark_stride < 1) mark_stride = 1;
	int64_t label_stride = (int64_t)ceil((font + 8.0) / (mark_rows * mark_stride));
	if(label_stride < 1) label_stride = 1;

	// Everything below is clipped to the data area
	cairo_save(c);
	cairo_rectangle(c, 0, plot_top, width, plot_bottom - plot_top);
	cairo_clip(c);
	cairo_set_line_width(c, th->line_scale);

	// Vertical grid (dotted): the ruler lines, which are also the amplitude lines
	for(k = 1; k * ruler < window_ms - ruler / 2; k++) {
		double x = round(RULER_X(k * ruler)) + .5;
		cairo_move_to(c, x, plot_top);
		cairo_line_to(c, x, plot_bottom);
	}
	axis_style(c, th, 0);
	cairo_stroke(c);

	// Window edges: a beat leaving one edge comes back at the other
	cairo_move_to(c, left_margin + .5, plot_top);
	cairo_line_to(c, left_margin + .5, plot_bottom);
	cairo_move_to(c, right_margin + .5, plot_top);
	cairo_line_to(c, right_margin + .5, plot_bottom);
	axis_style(c, th, 1);
	cairo_stroke(c);

	// Custom y marks are dotted; minute marks are drawn separately as solid guides.
	cairo_set_font_size(c, font * 0.8);
	for(int64_t mark_offset = 0; mark_offset <= last_mark_index; mark_offset += mark_stride) {
		int64_t mark_index = last_mark_index - mark_offset;
		double mark_seconds = mark_index * o->tick_seconds;
		double mark_time = mark_seconds * y_sample_rate;
		double y = plot_bottom - 0.5 - round((now - mark_time) / row_samples);
		if(y < plot_top) break;
		cairo_move_to(c, .5, y);
		cairo_line_to(c, width-.5, y);
		axis_style(c, th, 0);
		cairo_stroke(c);
		set_dotted(c, 0);

		if((mark_offset / mark_stride) % label_stride == 0) {
			unsigned long long minutes = (unsigned long long)(mark_seconds / 60.0);
			double seconds = fmod(mark_seconds, 60.0);
			if(o->tick_seconds < 10.0)
				snprintf(s, sizeof(s), "%llu:%05.2f", minutes, seconds);
			else
				snprintf(s, sizeof(s), "%llu:%02d", minutes, (int)round(seconds));
			cairo_text_extents(c, s, &extents);
			render_set_rgb(c, th->axis, 0.7);
			cairo_move_to(c, fmax(1, left_margin - extents.x_advance - 3), y - 2);
			cairo_show_text(c, s);
		}
	}
	int64_t last_minute_index = (int64_t)floor(now / (60.0 * y_sample_rate));
	for(int64_t minute_index = last_minute_index; minute_index >= 0; minute_index--) {
		double minute_time = minute_index * 60.0 * y_sample_rate;
		double y = plot_bottom - 0.5 - round((now - minute_time) / row_samples);
		if(y < plot_top) break;
		cairo_move_to(c, .5, y);
		cairo_line_to(c, width-.5, y);
		axis_style(c, th, 1);
		cairo_stroke(c);
		set_dotted(c, 0);
		snprintf(s, sizeof(s), "%llu:00", (unsigned long long)minute_index);
		cairo_text_extents(c, s, &extents);
		render_set_rgb(c, th->axis, 0.7);
		cairo_move_to(c, fmax(1, left_margin - extents.x_advance - 3), y - 2);
		cairo_show_text(c, s);
	}

	// Rate guide: lines with the slope of the measured rate (in calibration, of
	// the current cal value), dashed in the reference colour. The trace should
	// run parallel to them. Age is counted in rows from the bottom edge.
	cairo_set_line_width(c, 1.2 * th->line_scale);
	slope *= -strip_width * row_samples / sweep;
	if(slope <= 2 && slope >= -2) {
		static const double guide_dashes[2] = {6, 4};
		const double rows = plot_bottom;
		cairo_set_dash(c, guide_dashes, 2, 0);
		for(i=0; i<4; i++) {
			double y = 0;
			cairo_move_to(c, (double)width * (i+.5) / 4, plot_bottom);
			for(;;) {
				double x = y * slope + (double)width * (i+.5) / 4;
				x = fmod(x, width);
				if(x < 0) x += width;
				double nx = x + slope * (rows - y);
				if(nx >= 0 && nx <= width) {
					cairo_line_to(c, nx, plot_bottom - rows);
					break;
				} else {
					double d = slope > 0 ? width - x : x;
					y += d / fabs(slope);
					cairo_line_to(c, slope > 0 ? width : 0, plot_bottom - y);
					y += 1;
					if(y > rows) break;
					cairo_move_to(c, slope > 0 ? 0 : width, plot_bottom - y);
				}
			}
		}
		render_set_rgb(c, th->rate, 0.9);
		cairo_stroke(c);
		set_dotted(c, 0);
	}

	// Amplitude history; higher amplitude is further right, like in the tic and
	// toc panels. A reading beyond the scale is not drawn on its edge as if it
	// had been read there: the line breaks instead.
	if(!snst->calibrate && snst->amps_count && snst->amps && snst->amps_time) {
		int path_started = 0;
		cairo_set_line_width(c, 1.5 * th->line_scale);
		for(i = snst->amps_wp;;) {
			if(!snst->amps_time[i]) break;
			double event_age = now - snst->amps_time[i];
			double row = floor(event_age / row_samples);
			if(row >= plot_bottom) break;

			double amp_deg = snst->la * snst->amps[i];
			double amp_norm = (amp_deg - amp_min) / (amp_max - amp_min);
			if(amp_norm < 0 || amp_norm > 1) {
				path_started = 0;
				cairo_new_sub_path(c);
			} else {
				double column = left_margin + amp_norm * (strip_width - 1);
				double y = plot_bottom - row - .5;
				if(!path_started) {
					cairo_move_to(c, column, y);
					path_started = 1;
				} else {
					cairo_line_to(c, column, y);
				}
			}

			if(--i < 0) i = snst->amps_count - 1;
			if(i == snst->amps_wp) break;
		}
		signal_color(c, th->amp, stopped);
		cairo_stroke(c);
	}

	// Beats: one dot per beat, tic and toc in their colours (neutral for the
	// reference ticks of a calibration). Faded when the signal is lost.
	// Repeat the fixed ms window between the margins, including copies in
	// the outer margins so dots remain visible on both sides of a crossing.
	const double dot_size = th->id == RENDER_SCREEN ? PAPERSTRIP_DOT_SIZE : 1.5;
	for(i = snst->events_wp; valid_events_wp;) {
		if(!snst->events_count || !snst->events[i]) break;
		double age = now - snst->events[i];
		int row = floor(age / row_samples);
		if(row >= plot_bottom) break;

		// Position in the beat, relative to the centre: -sweep/2 to sweep/2.
		// Consecutive beats are one sweep apart, so they fall on the same column.
		double phase = fmod(age + snst->trace_centering, sweep);
		if(phase < 0) phase += sweep;
		// Mirrored horizontally: later in the beat is further left, so a
		// fast watch (positive rate) leans right, parallel to the rate guide
		double x = floor(width / 2.0 - (phase - sweep / 2) * zoom_factor * strip_width / sweep);
		x = fmod(x - left_margin, strip_width);
		if(x < 0) x += strip_width;
		x += left_margin;
		struct render_rgb dot = th->wave;
		if(!snst->calibrate && snst->events_tictoc)
			dot = snst->events_tictoc[i] ? th->tic : th->toc;
		signal_color(c, dot, stopped);
		const double dot_left = th->id == RENDER_SCREEN ? (PAPERSTRIP_DOT_SIZE - 1) / 2 : dot_size / 2;
		for(x -= strip_width; x - dot_left < width; x += strip_width) {
			if(x - dot_left + dot_size <= 0) continue;
			// A square of dot_size: around the beat position across,
			// from its row upwards, so that the newest row stays inside the plot
			cairo_rectangle(c, x - dot_left, plot_bottom - row - dot_size,
				dot_size, dot_size);
			cairo_fill(c);
		}
		if(--i < 0) i = snst->events_count - 1;
		if(i == snst->events_wp) break;
	}
	cairo_restore(c);

	// Scales on top: amplitude in degrees, position in the beat in ms
	cairo_set_line_width(c, th->line_scale);
	cairo_set_font_size(c, font);
	const double y_amp = font + 2;
	const double y_ms = scale_rows == 2 ? 2 * font + 6 : font + 2;
	// Equal divisions give ms values that are not round: fewer decimals for wider steps
	const int ms_digits = ruler >= 10 ? 0 : ruler >= 1 ? 1 : 2;
#define MS_LABEL(t) (snst->calibrate ? snprintf(s, sizeof(s), "%g", (t)) \
                                     : snprintf(s, sizeof(s), "%.*f", ms_digits, (t)))
	// Label every scale_stride-th line, so that the labels do not overlap
	double widest = 0;
	MS_LABEL(window_ms);
	cairo_text_extents(c, s, &extents);
	widest = extents.x_advance;
	if(!snst->calibrate) {
		cairo_text_extents(c, "360\xC2\xB0", &extents);
		widest = fmax(widest, extents.x_advance);
	}
	const double spacing = strip_width * ruler / window_ms;
	int scale_stride = spacing > 0 ? (int)ceil((widest + font / 2.0) / spacing) : 1;
	if(scale_stride < 1) scale_stride = 1;

	// Amplitude labels on the grid lines: line k from the right is amp_max - k * amp_step
	if(!snst->calibrate) {
		render_set_rgb(c, th->amp, 1);
		for(k = 0; k <= PAPERSTRIP_DIVISIONS; k += scale_stride) {
			snprintf(s, sizeof(s), "%.0f\xC2\xB0", amp_max - k * amp_step);
			cairo_text_extents(c, s, &extents);
			cairo_move_to(c, RULER_X(k * ruler) - extents.x_advance / 2, y_amp);
			cairo_show_text(c, s);
		}
	}
	// The ruler counts from the right edge, so its unit goes where the numbers grow
	cairo_text_extents(c, "ms", &extents);
	double unit_x = font / 4;
	double unit_end = unit_x + extents.x_advance;
	render_set_rgb(c, th->axis, 1);
	for(k = 0; k * ruler <= window_ms + 1e-9; k += scale_stride) {
		MS_LABEL(k * ruler);
		cairo_text_extents(c, s, &extents);
		double x = RULER_X(k * ruler) - extents.x_advance / 2;
		if(x < unit_end + font / 2 || x + extents.x_advance > width) continue; // keep clear of the unit
		cairo_move_to(c, x, y_ms);
		cairo_show_text(c, s);
	}
#undef MS_LABEL
	cairo_move_to(c, unit_x, y_ms);
	cairo_show_text(c, "ms");

	if(o->legend)
		render_paperstrip_legend(c, th, left_margin, height - 5, font * 0.9, snst->calibrate,
			isfinite(o->guide_rate));

#undef RULER_X
done:
	cairo_restore(c);
}
