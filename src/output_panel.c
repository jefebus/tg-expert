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

/* Colour semantics, the same in every panel:
 *   tic = grey, toc = gold, amplitude = magenta, rate/reference = azure,
 *   beat error, axes and grids = white (solid = major, dotted = minor),
 *   error = red, status OK (watch icon, calibration done) = green.
 * A stale reading keeps its colour but is drawn faded.
 * The graphs (waveforms, paper strip, Trend, correlations) take their colours
 * from the screen theme in render.c, which holds the same values; the ones
 * below are for the results bar and the status icons drawn here. */
cairo_pattern_t *black,*white,*red,*green;

struct color { double r, g, b; };
static const struct color COL_AMP  = {1, .310, .816}; // magenta #FF4FD0
static const struct color COL_RATE = {.302, .651, 1};
static const struct color COL_BE   = {.710, .486, 1}; // purple #B57CFF
#ifdef DEBUG
static const struct color COL_WAVE = {.902, .902, .902}; // neutral, for both beats
#endif
static const struct color COL_AXIS = {1, 1, 1};

#define ALPHA_STALE       0.4

static void define_color(cairo_pattern_t **gc,double r,double g,double b)
{
	*gc = cairo_pattern_create_rgb(r,g,b);
}

void initialize_palette()
{
	define_color(&black,0,0,0);
	define_color(&white,1,1,1);
	define_color(&red,1,.231,.188);
	define_color(&green,0,0.8,0);
}

static void set_color(cairo_t *c, struct color col, double alpha)
{
	cairo_set_source_rgba(c, col.r, col.g, col.b, alpha);
}

/* Colour of measured data: faded when the reading is stale */
static void set_signal_color(cairo_t *c, struct color col, int stale)
{
	set_color(c, col, stale ? ALPHA_STALE : 1);
}

#ifdef DEBUG
static void draw_debug_graph(double a, double b, cairo_t *c, struct processing_buffers *p, GtkWidget *da)
{
	if(!p->debug) return;

	GtkAllocation temp;
	gtk_widget_get_allocation (da, &temp);
	int width = temp.width;
	int height = temp.height;

	int i;
	float max = 0;

	int ai = round(a);
	int bi = 1+round(b);
	if(ai < 0) ai = 0;
	if(bi > p->sample_count) bi = p->sample_count;
	for(i=ai; i<bi; i++)
		if(p->debug[i] > max)
			max = p->debug[i];

	int first = 1;
	for(i=0; i<width; i++) {
		if( round(a + i*(b-a)/width) != round(a + (i+1)*(b-a)/width) ) {
			int j = round(a + i*(b-a)/width);
			if(j < 0) j = 0;
			if(j >= p->sample_count) j = p->sample_count-1;

			int k = round((0.1+p->debug[j]/max)*0.8*height);

			if(first) {
				cairo_move_to(c,i+.5,height-k-.5);
				first = 0;
			} else
				cairo_line_to(c,i+.5,height-k-.5);
		}
	}
}
#endif

static double amplitude_to_time(double lift_angle, double amp)
{
	return asin(lift_angle / (2 * amp)) / M_PI;
}

/* ms shown before the tic or toc in the waveform panels: enough to see the
 * lift pulse of the smallest amplitude the algorithm accepts */
int waveform_negative_span(const struct snapshot *s)
{
	double rate = s->sample_rate > 0 ? s->sample_rate : s->nominal_sr;
	double period = s->pb && rate > 0 ? s->pb->period / rate :
		s->guessed_bph > 0 ? 7200. / s->guessed_bph : 0;
	double min_amp = algorithm_parameters[ALG_AMP_MIN];
	double required = 0;
	if(period > 0 && min_amp > 0 && s->la > 0 && s->la <= 2 * min_amp)
		required = 1000 * period * amplitude_to_time(s->la, min_amp);
	if(!isfinite(required) || required <= NEGATIVE_SPAN)
		return NEGATIVE_SPAN;
	return (int)ceil(required);
}

static double draw_watch_icon(cairo_t *c, int signal, int happy, int light)
{
	happy = !!happy;
	cairo_set_line_width(c,3);
	cairo_set_source(c,happy?green:red);
	cairo_move_to(c, OUTPUT_WINDOW_HEIGHT * 0.5, OUTPUT_WINDOW_HEIGHT * 0.5);
	cairo_line_to(c, OUTPUT_WINDOW_HEIGHT * 0.75, OUTPUT_WINDOW_HEIGHT * (0.75 - 0.5*happy));
	cairo_move_to(c, OUTPUT_WINDOW_HEIGHT * 0.5, OUTPUT_WINDOW_HEIGHT * 0.5);
	cairo_line_to(c, OUTPUT_WINDOW_HEIGHT * 0.35, OUTPUT_WINDOW_HEIGHT * (0.65 - 0.3*happy));
	cairo_stroke(c);
	cairo_arc(c, OUTPUT_WINDOW_HEIGHT * 0.5, OUTPUT_WINDOW_HEIGHT * 0.5, OUTPUT_WINDOW_HEIGHT * 0.4, 0, 2*M_PI);
	cairo_stroke(c);
	int l = OUTPUT_WINDOW_HEIGHT * 0.8 / (2*NSTEPS - 1);
	int i;
	cairo_set_line_width(c,1);
	for(i = 0; i < signal; i++) {
		cairo_move_to(c, OUTPUT_WINDOW_HEIGHT + 0.5*l, OUTPUT_WINDOW_HEIGHT * 0.9 - 2*i*l);
		cairo_line_to(c, OUTPUT_WINDOW_HEIGHT + 1.5*l, OUTPUT_WINDOW_HEIGHT * 0.9 - 2*i*l);
		cairo_line_to(c, OUTPUT_WINDOW_HEIGHT + 1.5*l, OUTPUT_WINDOW_HEIGHT * 0.9 - (2*i+1)*l);
		cairo_line_to(c, OUTPUT_WINDOW_HEIGHT + 0.5*l, OUTPUT_WINDOW_HEIGHT * 0.9 - (2*i+1)*l);
		cairo_line_to(c, OUTPUT_WINDOW_HEIGHT + 0.5*l, OUTPUT_WINDOW_HEIGHT * 0.9 - 2*i*l);
		cairo_stroke_preserve(c);
		cairo_fill(c);
	}
	if(light) {
		int l = OUTPUT_WINDOW_HEIGHT * 0.15;
		cairo_set_line_width(c,2);
		cairo_move_to(c, OUTPUT_WINDOW_HEIGHT * 0.5 - 0.5*l, OUTPUT_WINDOW_HEIGHT * 0.2);
		cairo_line_to(c, OUTPUT_WINDOW_HEIGHT * 0.5 - 0.5*l, OUTPUT_WINDOW_HEIGHT * 0.2 + l);
		cairo_line_to(c, OUTPUT_WINDOW_HEIGHT * 0.5        , OUTPUT_WINDOW_HEIGHT * 0.2 + l);
		cairo_move_to(c, OUTPUT_WINDOW_HEIGHT * 0.5 - 0.2*l, OUTPUT_WINDOW_HEIGHT * 0.2 + 1);
		cairo_line_to(c, OUTPUT_WINDOW_HEIGHT * 0.5 + 0.8*l, OUTPUT_WINDOW_HEIGHT * 0.2 + 1);
		cairo_move_to(c, OUTPUT_WINDOW_HEIGHT * 0.5 + 0.3*l, OUTPUT_WINDOW_HEIGHT * 0.2 + 1);
		cairo_line_to(c, OUTPUT_WINDOW_HEIGHT * 0.5 + 0.3*l, OUTPUT_WINDOW_HEIGHT * 0.2 + l + 1);
		cairo_stroke(c);
	}
	return OUTPUT_WINDOW_HEIGHT + 3*l;
}

static enum audio_input_status get_input_status(struct snapshot *snst)
{
	if(snst->pb && !snst->is_old)
		return AUDIO_STATUS_OK;
	return snst->bph ? AUDIO_STATUS_BPH_UNCONFIRMED : AUDIO_STATUS_BPH_UNKNOWN;
}

static const char *detection_status_message(enum detection_status status)
{
	switch(status) {
	case DETECTION_STATUS_OK:
		return NULL;
#ifdef DEBUG
	case DETECTION_STATUS_NO_WAVEFORM:
		return _("No watch detected: no valid beat period found.");
	case DETECTION_STATUS_PERIOD_TOO_LONG:
		return _("No watch detected: the detected period is too long.");
	case DETECTION_STATUS_NO_BEAT:
		return _("No watch detected: tic and toc could not be separated.");
	case DETECTION_STATUS_UNSTABLE:
		return _("No watch detected: the beat period is too unstable.");
	case DETECTION_STATUS_RATE_ONLY:
		return _("Rate only: tic and toc could not be separated; beat error and amplitude are unavailable.");
	case DETECTION_STATUS_AMPLITUDE_NOT_FOUND:
		return _("Amplitude unavailable: lift pulses could not be measured.");
	case DETECTION_STATUS_AMPLITUDE_TOO_LOW:
		return _("Amplitude unavailable: the measured amplitude is too low.");
	case DETECTION_STATUS_AMPLITUDE_TOO_HIGH:
		return _("Amplitude unavailable: the measured amplitude is too high.");
	case DETECTION_STATUS_AMPLITUDE_INCONSISTENT:
		return _("Amplitude unavailable: tic and toc amplitudes differ too much.");
#else
	case DETECTION_STATUS_NO_WAVEFORM:
	case DETECTION_STATUS_PERIOD_TOO_LONG:
	case DETECTION_STATUS_NO_BEAT:
	case DETECTION_STATUS_UNSTABLE:
		return _("No watch detected.");
	case DETECTION_STATUS_RATE_ONLY:
		return _("Rate only: beat error and amplitude unavailable.");
	case DETECTION_STATUS_AMPLITUDE_NOT_FOUND:
	case DETECTION_STATUS_AMPLITUDE_TOO_LOW:
	case DETECTION_STATUS_AMPLITUDE_TOO_HIGH:
	case DETECTION_STATUS_AMPLITUDE_INCONSISTENT:
		return _("Amplitude unavailable.");
#endif
	}
	g_warning("Unknown detection status: %d", status);
	return _("No watch detected.");
}

static void update_detection_status(struct output_panel *op)
{
	if(!op->detection_status_label || !op->snst)
		return;

	enum detection_status status = op->snst->detection_status;
#ifndef DEBUG
	switch(status) {
	case DETECTION_STATUS_NO_WAVEFORM:
	case DETECTION_STATUS_PERIOD_TOO_LONG:
	case DETECTION_STATUS_NO_BEAT:
	case DETECTION_STATUS_UNSTABLE:
		status = DETECTION_STATUS_NO_WAVEFORM;
		break;
	case DETECTION_STATUS_AMPLITUDE_NOT_FOUND:
	case DETECTION_STATUS_AMPLITUDE_TOO_LOW:
	case DETECTION_STATUS_AMPLITUDE_TOO_HIGH:
	case DETECTION_STATUS_AMPLITUDE_INCONSISTENT:
		status = DETECTION_STATUS_AMPLITUDE_NOT_FOUND;
		break;
	default:
		break;
	}
#endif
	gint64 now = g_get_monotonic_time();
	if(status == op->shown_detection_status ||
			(op->detection_status_until && now < op->detection_status_until))
		return;

	op->shown_detection_status = status;
	op->detection_status_until = status == DETECTION_STATUS_OK ? 0 : now + 2000000;
	const char *message = detection_status_message(status);
	/* An empty label has no height; a space keeps the status line in place. */
	gtk_label_set_text(GTK_LABEL(op->detection_status_label), message ? message : " ");
	gtk_widget_set_tooltip_text(op->detection_status_label, message);
}

static void update_audio_presence(struct output_panel *op)
{
	gint64 now = g_get_monotonic_time();
	if(op->audio_meter_time && now - op->audio_meter_time < 500000)
		return;
	op->audio_meter_time = now;
	op->audio_meter_level = 0.6f * op->audio_meter_level + 0.4f * get_audio_level();

	if(!op->audio_meter_active) {
		if(op->audio_meter_level >= 0.008f)
			op->audio_meter_transition_count++;
		else
			op->audio_meter_transition_count = 0;
		if(op->audio_meter_transition_count >= 2) {
			op->audio_meter_active = 1;
			op->audio_meter_transition_count = 0;
		}
	} else {
		if(op->audio_meter_level <= 0.004f)
			op->audio_meter_transition_count++;
		else
			op->audio_meter_transition_count = 0;
		if(op->audio_meter_transition_count >= 3) {
			op->audio_meter_active = 0;
			op->audio_meter_transition_count = 0;
		}
	}
}

static double draw_input_status(cairo_t *c, double x, enum audio_input_status status)
{
	static const struct color status_colors[] = {
		{.55, .55, .55},
		{1, .72, .12},
		{1, .31, .25},
		{.15, .85, .35}
	};
	static const char *status_marks[] = {"0", "?", "!", ""};
	set_color(c, status_colors[status], 1);
	cairo_set_font_size(c, OUTPUT_WINDOW_HEIGHT * 0.28);
	cairo_move_to(c, x, OUTPUT_WINDOW_HEIGHT * 0.76);
	cairo_show_text(c, status_marks[status]);
	return x + 24;
}

static void cairo_init(cairo_t *c)
{
	cairo_set_line_width(c,1);

	cairo_set_source(c,black);
	cairo_paint(c);
}

static double print_s(cairo_t *c, double x, double y, char *s)
{
	cairo_text_extents_t extents;
	cairo_move_to(c,x,y);
	cairo_show_text(c,s);
	cairo_text_extents(c,s,&extents);
	x += extents.x_advance;
	return x;
}

static double print_number(cairo_t *c, double x, double y, char *s)
{
	cairo_text_extents_t extents;
	cairo_text_extents(c,"0",&extents);
	double z = extents.x_advance;
	char t[2];
	t[1] = 0;
	while((t[0] = *s++)) {
		cairo_text_extents(c,t,&extents);
		cairo_move_to(c, x + (z - extents.x_advance) / 2, y);
		cairo_show_text(c,t);
		x += z;
	}
	return x;
}

/* Two-row results table, like a hardware timegrapher: column titles on top,
 * values below. The columns share the width to the right of the watch icon. */
#define RESULT_COLUMNS 5
#define RESULT_TITLE_ROW  0.22	// vertical centre of each row, as a fraction of the height
#define RESULT_LINE       0.44
#define RESULT_VALUE_ROW  0.72

static void draw_results_table(cairo_t *c, double x0, double width, double height,
		struct snapshot *snst, struct processing_buffers *p, int show_candidate_bph)
{
	static const char *titles[RESULT_COLUMNS] = {"RATE", "AMP.", "B.E.", "L.A.", "BEAT"};
	char num[RESULT_COLUMNS][16];
	const char *unit[RESULT_COLUMNS];
	cairo_text_extents_t e;
	int i;

	if(width <= 0) return;

	// Digits and unit are kept apart: digits go in fixed-width cells so that
	// the values do not jitter, the unit follows them.
	if(p) {
		int rate = round(snst->rate);
		snprintf(num[0], sizeof(num[0]), "%s%d", rate > 0 ? "+" : rate < 0 ? "-" : "", abs(rate));
		unit[0] = "s/d";
		if(!snst->rate_only && snst->amp > 0) {
			snprintf(num[1], sizeof(num[1]), "%.0f", snst->amp);
			unit[1] = "\xC2\xB0";
		} else {
			strcpy(num[1], "---");
			unit[1] = "";
		}
		if(snst->rate_only) {
			strcpy(num[2], "---");
			unit[2] = "";
		} else {
			snprintf(num[2], sizeof(num[2]), "%.1f", snst->be);
			unit[2] = "ms";
		}
	} else {
		for(i = 0; i < 3; i++) {
			strcpy(num[i], "---");
			unit[i] = "";
		}
	}
	snprintf(num[3], sizeof(num[3]), "%.0f", snst->la); // whole degrees only
	unit[3] = "\xC2\xB0";
	if(show_candidate_bph && snst->candidate_bph && snst->bph && snst->is_old)
		snprintf(num[4], sizeof(num[4]), "~%d", snst->candidate_bph);
	else
		snprintf(num[4], sizeof(num[4]), "%d", snst->guessed_bph);
	unit[4] = "";

	// One font size for all values, reduced when the window is too narrow
	double col_w = width / RESULT_COLUMNS;
	double fv = OUTPUT_FONT * 0.85;
	double cell, widest = 0;
	cairo_set_font_size(c, fv);
	cairo_text_extents(c, "0", &e);
	cell = e.x_advance;
	for(i = 0; i < RESULT_COLUMNS; i++) {
		cairo_text_extents(c, unit[i], &e);
		double wd = cell * strlen(num[i]) + e.x_advance;
		if(wd > widest) widest = wd;
	}
	if(widest > col_w * 0.92) {
		fv *= col_w * 0.92 / widest;
		cairo_set_font_size(c, fv);
		cairo_text_extents(c, "0", &e);
		cell = e.x_advance;
	}

	// Column titles
	cairo_save(c);
	cairo_select_font_face(c, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(c, fv * 0.5);
	cairo_text_extents(c, "RATE", &e);
	double y_title = RESULT_TITLE_ROW * height - e.y_bearing - e.height / 2;
	cairo_set_source(c, white);
	for(i = 0; i < RESULT_COLUMNS; i++) {
		cairo_text_extents(c, titles[i], &e);
		cairo_move_to(c, x0 + col_w * (i + 0.5) - e.x_advance / 2, y_title);
		cairo_show_text(c, titles[i]);
	}
	cairo_restore(c);

	// Line between titles and values
	cairo_set_source_rgb(c, .55, .55, .55);
	cairo_set_line_width(c, 1);
	cairo_move_to(c, x0, round(RESULT_LINE * height) + .5);
	cairo_line_to(c, x0 + width, round(RESULT_LINE * height) + .5);
	cairo_stroke(c);

	// Values: rate in the rate colour, amplitude in the amplitude colour, the
	// rest white. Measured values are faded while the reading is stale.
	cairo_set_font_size(c, fv);
	cairo_text_extents(c, "0", &e);
	double y_value = RESULT_VALUE_ROW * height - e.y_bearing - e.height / 2;
	for(i = 0; i < RESULT_COLUMNS; i++) {
		if(i < 3 && !p)
			set_color(c, COL_AXIS, ALPHA_STALE);
		else if(i == 0)
			set_signal_color(c, COL_RATE, snst->is_old);
		else if(i == 1)
			set_signal_color(c, COL_AMP, snst->is_old);
		else if(i == 2)
			set_signal_color(c, COL_BE, snst->is_old);
		else
			set_color(c, COL_AXIS, 1);
		cairo_text_extents(c, unit[i], &e);
		double digits = cell * strlen(num[i]);
		double x = x0 + col_w * (i + 0.5) - (digits + e.x_advance) / 2;
		x = print_number(c, x, y_value, num[i]);
		print_s(c, x, y_value, (char *)unit[i]);
	}
}

static gboolean output_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	cairo_init(c);

	struct snapshot *snst = op->snst;
	struct processing_buffers *p = snst->pb;
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	double x = draw_watch_icon(c,snst->signal,snst->calibrate ? snst->signal==NSTEPS : snst->signal, snst->is_light);
	int show_audio_status = 0;
	enum audio_input_status status = AUDIO_STATUS_UNAVAILABLE;
	if(op->computer && !snst->calibrate) {
		update_audio_presence(op);
		status = op->audio_meter_active ? get_input_status(snst) : AUDIO_STATUS_NO_INPUT;
		snst->audio_status = status;
		show_audio_status = 1;
	} else if(!op->computer && snst->audio_status >= AUDIO_STATUS_NO_INPUT &&
		snst->audio_status <= AUDIO_STATUS_OK) {
		status = snst->audio_status;
		show_audio_status = 1;
	}
	if(show_audio_status) {
		const char *message[] = {
			/* TRANSLATORS: status tooltip. Keep the leading "0:" */
			N_("0: no audio input. The four squares by the clock show analysis windows with a valid period."),
			/* TRANSLATORS: status tooltip. Keep the leading "?:" */
			N_("?: audio is present, but no beat rate has been recognized yet. The four squares show analysis windows with a valid period."),
			/* TRANSLATORS: status tooltip. Keep the leading "!:" and the column name BEAT, it is not translated */
			N_("!: no valid rhythm at the selected beat rate. A free estimate, when available, is marked with ~ in the BEAT column. The four squares show analysis windows with a valid period."),
			/* TRANSLATORS: status tooltip */
			N_("Audio input and a valid beat period are detected. The four squares show analysis windows with a valid period.")
		};
		x = draw_input_status(c, x + 3, status);
		if(op->audio_status != status) {
			gtk_widget_set_tooltip_text(widget, _(message[status]));
			op->audio_status = status;
		}
	}

	cairo_text_extents_t extents;

	cairo_set_font_size(c, OUTPUT_FONT);
	cairo_text_extents(c,"0",&extents);
	double y = (double)OUTPUT_WINDOW_HEIGHT/2 - extents.y_bearing - extents.height/2;

	if(snst->calibrate) {
		cairo_set_source(c, white);
		x = print_s(c,x,y,"cal");
		cairo_set_font_size(c, OUTPUT_FONT*2/3);
		x = print_s(c,x,y," (");
		cairo_move_to(c,x,y);
		{
			double a = 0;
			char *s[] = {"wait", "acq.", "done", "fail", NULL}, **t = s;
			for(;*t;t++) {
				cairo_text_extents(c,*t,&extents);
				if(a < extents.x_advance) a = extents.x_advance;
			}
			x += a;
		}
		switch(snst->cal_state) {
			case 1:
				cairo_set_source(c,green);
				cairo_show_text(c,"done");
				break;
			case 0:
				if(snst->signal == NSTEPS)
					cairo_set_source(c, white);
				else
					set_color(c, COL_AXIS, ALPHA_STALE);
				cairo_show_text(c, snst->signal == NSTEPS ? "acq." : "wait");
				break;
			case -1:
				cairo_set_source(c,red);
				cairo_show_text(c,"fail");
				break;
		}
		cairo_set_source(c, white);
		x = print_s(c,x,y,")");
		cairo_set_font_size(c, OUTPUT_FONT);
		char s[20];
		switch(snst->cal_state) {
			case 1:
				sprintf(s, " %s%d.%d",
						snst->cal_result < 0 ? "-" : "+",
						abs(snst->cal_result) / 10,
						abs(snst->cal_result) % 10 );
				x = print_s(c,x,y,s);
				cairo_set_font_size(c, OUTPUT_FONT*2/3);
				x = print_s(c,x,y," s/d");
				break;
			case 0:
				sprintf(s, " %d", snst->cal_percent);
				x = print_number(c,x,y,s);
				x = print_s(c,x,y," %");
				break;
		}
	} else {
		draw_results_table(c, x + 10, width - x - 20, height, snst, p,
			snst->audio_status == AUDIO_STATUS_BPH_UNCONFIRMED);
	}
#ifdef DEBUG
	{
		// Frame rate, small and out of the way in the top right corner
		static GTimer *timer = NULL;
		if (!timer) timer = g_timer_new();
		else {
			char s[100];
			sprintf(s,"%.2f fps",1./g_timer_elapsed(timer, NULL));
			cairo_set_source(c, white);
			cairo_set_font_size(c, 12);
			print_s(c, width - 90, 14, s);
			g_timer_reset(timer);
		}
	}
#endif

	return FALSE;
}

/* The waveform panels are drawn by render_waveform() (waveform_render.c);
 * these only gather the size, the text size and the snapshot of the panel */
static void draw_waveform_panel(GtkWidget *da, cairo_t *c, struct output_panel *op,
	enum waveform_kind kind)
{
	GtkAllocation a, top;
	gtk_widget_get_allocation(da, &a);
	gtk_widget_get_allocation(gtk_widget_get_toplevel(da), &top);
	int font = top.width / 90;
	if(font < 12)
		font = 12;
	struct waveform_options o = {
		.kind = kind,
		.negative_span = waveform_negative_span(op->snst),
		.font = font
	};
	render_waveform(c, a.width, a.height, op->snst, &o, render_theme_get(RENDER_SCREEN));
}

static gboolean tic_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	UNUSED(widget);
	draw_waveform_panel(op->tic_drawing_area, c, op, WAVEFORM_TIC);
	return FALSE;
}

static gboolean toc_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	UNUSED(widget);
	draw_waveform_panel(op->toc_drawing_area, c, op, WAVEFORM_TOC);
	return FALSE;
}

static gboolean period_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	UNUSED(widget);
	draw_waveform_panel(op->period_drawing_area, c, op, WAVEFORM_PERIOD);
	return FALSE;
}

/* The measured rate changes a little with every reading, which would make the
 * rate guide lines swing all the time. For the live display the guide follows a
 * low-pass filtered rate instead; a big change is a new situation (another
 * watch, a different bph) and is followed at once. Saved snapshots are static
 * and use their own rate. */
#define GUIDE_RATE_TAU  10.0	// filter time constant, seconds
#define GUIDE_RATE_JUMP 5.0	// s/d

static double guide_rate(struct output_panel *op)
{
	double rate = op->snst->rate;
	if(!op->computer)
		return rate;

	gint64 now = g_get_monotonic_time();
	if(!op->guide_time || fabs(rate - op->guide_rate) > GUIDE_RATE_JUMP) {
		op->guide_rate = rate;
	} else {
		// Time based, so that the number of redraws makes no difference
		double dt = (now - op->guide_time) / 1e6;
		op->guide_rate += (1 - exp(-dt / GUIDE_RATE_TAU)) * (rate - op->guide_rate);
	}
	op->guide_time = now;
	return op->guide_rate;
}

static int center_trace(struct output_panel *op);

/* The paper strip is drawn by render_paperstrip() (paperstrip_render.c). The
 * live display adds two things to the snapshot data: the trace is centred
 * once when it appears, and the rate guide follows a filtered rate. */
static gboolean paperstrip_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	struct snapshot *snst = op->snst;
	const int valid_events_wp =
		snst->events &&
		snst->events_count > 0 &&
		snst->events_wp >= 0 &&
		snst->events_wp < snst->events_count;
	// The strip is a fixed ms range, so a new trace would usually start outside
	// it: centre the live trace once when it appears (start, Clear, audio restart)
	if(!valid_events_wp || !snst->events[snst->events_wp])
		op->trace_centered = 0;
	else if(op->computer && !op->trace_centered)
		op->trace_centered = center_trace(op);

	struct paperstrip_options o = {
		.tick_seconds = op->paperstrip_tick_seconds,
		.window_seconds = op->paperstrip_window_seconds,
		.guide_rate = NAN,
		.now = snst->timestamp ? snst->timestamp : get_timestamp(snst->is_light),
		.legend = 1
	};
	if(!snst->calibrate) {
		if(valid_events_wp && snst->events[snst->events_wp])
			o.guide_rate = guide_rate(op);
		else
			op->guide_time = 0; // no trace: start over when it comes back
	}

	GtkAllocation a, toplevel;
	gtk_widget_get_allocation(op->paperstrip_drawing_area, &a);
	gtk_widget_get_allocation(gtk_widget_get_toplevel(widget), &toplevel);
	int font = toplevel.width / 90;
	if(font < 12)
		font = 12;
	o.font = font;
	render_paperstrip(c, a.width, a.height, snst, &o, render_theme_get(RENDER_SCREEN));
	return FALSE;
}

#ifdef DEBUG
static gboolean debug_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	UNUSED(widget);
	cairo_init(c);

	struct snapshot *snst = op->snst;
	struct processing_buffers *p;
	if(snst->calibrate)
		p = &op->computer->pdata->buffers[0];
	else
		p = snst->pb;

	if(p) {
		double a = snst->nominal_sr / 10;
		double b = snst->nominal_sr * 2;

		draw_debug_graph(a,b,c,p,op->debug_drawing_area);

		set_signal_color(c, COL_WAVE, snst->is_old);
		cairo_stroke(c);
	}

	return FALSE;
}
#endif

static void handle_clear_trace(GtkButton *b, struct output_panel *op)
{
	UNUSED(b);
	if(op->computer) {
		lock_computer(op->computer);
		if(!op->snst->calibrate) {
			memset(op->snst->events,0,op->snst->events_count*sizeof(uint64_t));
			memset(op->snst->events_tictoc,0,op->snst->events_count*sizeof(unsigned char));
			memset(op->snst->amps,0,op->snst->amps_count*sizeof(*op->snst->amps));
			memset(op->snst->amps_time,0,op->snst->amps_count*sizeof(*op->snst->amps_time));
			op->snst->trace_zoom = 1.0;
			op->computer->clear_trace = 1;
		}
		unlock_computer(op->computer);
		gtk_widget_queue_draw(op->paperstrip_drawing_area);
	}
}

/* Put the newest beat in the centre of the strip. Returns 0 if there is no beat yet. */
static int center_trace(struct output_panel *op)
{
	struct snapshot *snst = op->snst;
	if(!snst || !snst->events || snst->events_count <= 0)
		return 0;
	if(snst->events_wp < 0 || snst->events_wp >= snst->events_count)
		return 0;
	uint64_t last_ev = snst->events[snst->events_wp];
	if(!last_ev)
		return 0;
	double sweep;
	double time = snst->timestamp ? snst->timestamp : get_timestamp(snst->is_light);
	if(snst->calibrate) {
		sweep = (double) snst->nominal_sr;
	} else {
		if(snst->sample_rate <= 0 || snst->guessed_bph <= 0)
			return 0;
		sweep = snst->sample_rate * 3600. / snst->guessed_bph;
	}
	if(!isfinite(sweep) || sweep <= 0)
		return 0;
	double now = sweep * ceil(time / sweep);
	if(!isfinite(now))
		return 0;
	// The drawing puts the beat at fmod(age + trace_centering, sweep); the centre is sweep / 2
	double new_centering = fmod(0.5 * sweep - fmod(now - last_ev, sweep), sweep);
	if(new_centering < 0)
		new_centering += sweep;
	if(!isfinite(new_centering))
		return 0;
	snst->trace_centering = new_centering;
	gtk_widget_queue_draw(op->paperstrip_drawing_area);
	return 1;
}

static void handle_center_trace(GtkButton *b, struct output_panel *op)
{
	UNUSED(b);
	center_trace(op);
}

static void shift_trace(struct output_panel *op, double direction)
{
	struct snapshot *snst = op->snst;
	if(!snst)
		return;
	double chart_width;
	double zoom_factor;
	if(snst->calibrate) {
		chart_width = (double) snst->nominal_sr;
		zoom_factor = paperstrip_zoom_factor(snst);
	} else {
		if(snst->sample_rate <= 0 || snst->guessed_bph <= 0)
			return;
		chart_width = snst->sample_rate * 3600. / snst->guessed_bph;
		zoom_factor = paperstrip_zoom_factor(snst);
	}
	if(!isfinite(chart_width) || chart_width <= 0 || !isfinite(zoom_factor) || zoom_factor <= 0)
		return;
	// One grid division of the strip; the position is kept within one beat
	double step = chart_width / zoom_factor / PAPERSTRIP_DIVISIONS;
	snst->trace_centering = fmod(snst->trace_centering + step * direction, chart_width);
	if(snst->trace_centering < 0)
		snst->trace_centering += chart_width;
	gtk_widget_queue_draw(op->paperstrip_drawing_area);
}

static void zoom_trace(struct output_panel *op, double factor)
{
	struct snapshot *snst = op->snst;
	if(!snst)
		return;
	/* One level of paperstrip_zoom_levels in the direction of factor */
	int i = paperstrip_zoom_index(snst->trace_zoom);
	if(factor > 1 && paperstrip_zoom_levels[i + 1])
		i++;
	else if(factor < 1 && i > 0)
		i--;
	snst->trace_zoom = paperstrip_zoom_levels[i];
	gtk_widget_queue_draw(op->paperstrip_drawing_area);
}

static void handle_paperstrip_setting_change(GtkSpinButton *spin, struct output_panel *op)
{
	UNUSED(spin);
	op->paperstrip_tick_seconds = gtk_spin_button_get_value(GTK_SPIN_BUTTON(op->paperstrip_tick_spin));
	op->paperstrip_window_seconds = gtk_spin_button_get_value(GTK_SPIN_BUTTON(op->paperstrip_window_spin));
	gtk_widget_queue_draw(op->paperstrip_drawing_area);
	if(!op->updating_paperstrip_settings && op->paperstrip_settings_changed)
		op->paperstrip_settings_changed(op->paperstrip_settings_data,
			op->paperstrip_tick_seconds, op->paperstrip_window_seconds);
}

static void handle_zoom_out(GtkButton *b, struct output_panel *op)
{
	UNUSED(b);
	zoom_trace(op, 0.5);
}

static void handle_zoom_in(GtkButton *b, struct output_panel *op)
{
	UNUSED(b);
	zoom_trace(op, 2);
}

static void handle_zoom_reset(GtkButton *b, struct output_panel *op)
{
	UNUSED(b);
	if(!op->snst)
		return;
	op->snst->trace_zoom = 1.0;
	gtk_widget_queue_draw(op->paperstrip_drawing_area);
}

static void handle_left(GtkButton *b, struct output_panel *op)
{
	UNUSED(b);
	shift_trace(op,-1);
}

static void handle_right(GtkButton *b, struct output_panel *op)
{
	UNUSED(b);
	shift_trace(op,1);
}

/* While calibrating, the waveform panels are meaningless: show instructions instead */
/* TRANSLATORS: title of the paper strip panel */
#define TITLE_PAPERSTRIP  N_("Paper strip: beat timing over time")
/* TRANSLATORS: title of the paper strip panel while calibrating */
#define TITLE_PAPERSTRIP_CAL N_("Paper strip: reference ticks")

/* ---- Trend page: long-term statistics recorded by trend.c ---- */

void format_duration(char *s, size_t n, double seconds)
{
	long t = seconds > 0 ? (long)seconds : 0;
	if(t >= 3600)
		snprintf(s, n, "%ld:%02ld:%02ld", t / 3600, t / 60 % 60, t % 60);
	else
		snprintf(s, n, "%ld:%02ld", t / 60, t % 60);
}

static gboolean trend_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	GtkAllocation a;
	gtk_widget_get_allocation(widget, &a);
	GtkAllocation toplevel;
	gtk_widget_get_allocation(gtk_widget_get_toplevel(widget), &toplevel);
	int font = toplevel.width / 90;
	if(font < 12)
		font = 12;
	struct trend_render_options o = {
		.sections = TREND_RENDER_STATS | TREND_RENDER_GRAPHS,
		.span_s = op->trend_span_s,
		.rate_range = op->trend_rate_range,
		.be_range = op->trend_be_range,
		.font = font,
		.calibrating = op->snst && op->snst->calibrate,
		.paused = op->paused
	};
	render_trend(c, a.width, a.height, op->trend, &o, render_theme_get(RENDER_SCREEN));
	return FALSE;
}

static void handle_trend_reset(GtkButton *b, struct output_panel *op)
{
	UNUSED(b);
	trend_reset(op->trend);
	gtk_widget_queue_draw(op->trend_drawing_area);
	correlations_queue_draw(op);
}

static void handle_settling_setting(GtkSpinButton *spin, struct output_panel *op)
{
	if(op->settling_setting_changed)
		op->settling_setting_changed(op->settling_setting_data,
			gtk_spin_button_get_value_as_int(spin));
}

void op_set_settling_setting(struct output_panel *op, int seconds,
	void (*callback)(void *, int), void *data)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(op->settling_spin), seconds);
	op->settling_setting_changed = callback;
	op->settling_setting_data = data;
}

void op_update_settling_status(struct output_panel *op)
{
	if(!op || !op->settling_status || !op->trend) return;
	gint64 remaining = op->trend->settling_until_us - g_get_monotonic_time();
	char text[80];
	if(remaining > 0)
		snprintf(text, sizeof(text), _("Settling: %d s remaining"),
			(int)((remaining + 999999) / 1000000));
	else
		text[0] = '\0';
	if(strcmp(gtk_label_get_text(GTK_LABEL(op->settling_status)), text))
		gtk_label_set_text(GTK_LABEL(op->settling_status), text);
}

static void handle_trend_rate_range(GtkComboBox *b, struct output_panel *op)
{
	const gchar *id = gtk_combo_box_get_active_id(b);
	op->trend_rate_range = id ? atoi(id) : 0;
	gtk_widget_queue_draw(op->trend_drawing_area);
	correlations_queue_draw(op);
}

static void handle_trend_be_range(GtkComboBox *b, struct output_panel *op)
{
	const gchar *id = gtk_combo_box_get_active_id(b);
	op->trend_be_range = id ? atoi(id) : 0;
	if(op->trend_be_combo && g_strcmp0(gtk_combo_box_get_active_id(
			GTK_COMBO_BOX(op->trend_be_combo)), id))
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(op->trend_be_combo), id);
	if(op->correlation_be_combo && g_strcmp0(gtk_combo_box_get_active_id(
			GTK_COMBO_BOX(op->correlation_be_combo)), id))
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(op->correlation_be_combo), id);
	gtk_widget_queue_draw(op->trend_drawing_area);
	correlations_queue_draw(op);
}

GtkWidget *op_create_trend_be_combo(struct output_panel *op)
{
	GtkWidget *combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), "0", _("All"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), "1", "< 1 ms");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), "2", "< 2 ms");
	char id[16];
	snprintf(id, sizeof(id), "%d", op->trend_be_range);
	gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), id);
	g_signal_connect(combo, "changed", G_CALLBACK(handle_trend_be_range), op);
	return combo;
}

static void handle_trend_span(GtkComboBox *b, struct output_panel *op)
{
	const gchar *id = gtk_combo_box_get_active_id(b);
	op->trend_span_s = id ? atoi(id) : 0;
	if(op->trend_span_combo && g_strcmp0(gtk_combo_box_get_active_id(
			GTK_COMBO_BOX(op->trend_span_combo)), id))
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(op->trend_span_combo), id);
	if(op->correlation_span_combo && g_strcmp0(gtk_combo_box_get_active_id(
			GTK_COMBO_BOX(op->correlation_span_combo)), id))
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(op->correlation_span_combo), id);
	gtk_widget_queue_draw(op->trend_drawing_area);
	correlations_queue_draw(op);
}

GtkWidget *op_create_trend_span_combo(struct output_panel *op)
{
	GtkWidget *combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), "0", _("All"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), "3600", _("Last 1 h"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), "21600", _("Last 6 h"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), "86400", _("Last 24 h"));
	char id[16];
	snprintf(id, sizeof(id), "%d", op->trend_span_s);
	gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), id);
	g_signal_connect(combo, "changed", G_CALLBACK(handle_trend_span), op);
	return combo;
}

/* Wrap a panel in a frame with a small title on top */
static GtkWidget *titled_frame(GtkWidget *child, const char *title)
{
	GtkWidget *frame = gtk_frame_new(title);
	gtk_frame_set_label_align(GTK_FRAME(frame), 0.01, 0.5);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(frame), child);
	return frame;
}

static void set_frame_title(GtkWidget *frame, const char *title)
{
	const gchar *current = gtk_frame_get_label(GTK_FRAME(frame));
	if(!current || strcmp(current, title))
		gtk_frame_set_label(GTK_FRAME(frame), title);
}

void op_update_trend_pages(struct output_panel *op)
{
	if(!op->trend_page || !op->correlations_page) return;
	int show = op->computer || op->trend;
	gtk_widget_set_visible(op->trend_page, show);
	gtk_widget_set_visible(op->correlations_page, show);
	const gchar *current = gtk_stack_get_visible_child_name(GTK_STACK(op->right_stack));
	if(!show && current && (!strcmp(current, "trend") || !strcmp(current, "correlations")))
		gtk_stack_set_visible_child_name(GTK_STACK(op->right_stack), "panels");
}

static void update_right_side(struct output_panel *op)
{
	int cal = op->snst && op->snst->calibrate;
	const gchar *current = gtk_stack_get_visible_child_name(GTK_STACK(op->right_stack));
	// Out of calibration, keep the page the user chose (waveforms or trend)
	const char *page = cal ? "calibration" :
		current && strcmp(current, "calibration") ? current : "panels";
	if(!current || strcmp(current, page))
		gtk_stack_set_visible_child_name(GTK_STACK(op->right_stack), page);
	gtk_widget_set_sensitive(op->right_switcher, !cal);

	set_frame_title(op->paperstrip_frame, cal ? _(TITLE_PAPERSTRIP_CAL) : _(TITLE_PAPERSTRIP));
}

static GtkWidget *make_calibration_instructions(struct output_panel *op)
{
	const char *markup =
		/* TRANSLATORS: keep cal (wait), cal (acq.), cal (done), cal (fail) in English, they are shown like that in the results bar.
		 * Also keep the word "done" in English where it is mentioned as a status, and keep the markup tags. */
		_("<span size='x-large' weight='bold'>Calibration in progress</span>\n"
		"\n"
		"Calibration measures how fast your sound card clock really runs, so that "
		"rates are not skewed by the computer. With default settings it takes about 15 minutes (900 ticks).\n"
		"\n"
		"<b>1. Use a quartz reference</b>\n"
		"Place a quartz watch or clock that ticks once per second (an analog one with "
		"a seconds hand) very close to the microphone, ideally touching it. "
		"Its accuracy becomes your calibration accuracy.\n"
		"\n"
		"<b>2. Keep it quiet</b>\n"
		"Work in a quiet room, switch off nearby fans and speakers, and do not touch "
		"the desk while calibrating. Raise the microphone gain only as far as needed to "
		"hear the tick clearly without distortion.\n"
		"\n"
		"<b>3. Watch the status at the top</b>\n"
		"<b>cal (wait)</b>: no steady 1 tick per second found yet, check the setup.\n"
		"<b>cal (acq.) NN %</b>: collecting ticks, leave everything untouched.\n"
		"<b>cal (done) \302\261X.X s/d</b>: finished. The value is copied to the "
		"<b>cal</b> field and saved.\n"
		"<b>cal (fail)</b>: the signal was too irregular. Improve the setup and start again.\n"
		"\n"
		"<b>4. Finish</b>\n"
		"When it shows <b>done</b>, select <b>Stop calibration</b> in the menu to "
		"return to normal measurement. Repeat once to confirm the result is consistent, "
		"and calibrate again whenever you change the audio input or the sample rate.");

	GtkWidget *label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_max_width_chars(GTK_LABEL(label), 80);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_start(label, 20);
	gtk_widget_set_margin_end(label, 20);
	gtk_widget_set_margin_top(label, 20);
	gtk_widget_set_margin_bottom(label, 20);

	// Scroll if the window is too small to show everything
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), label);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	op->calibration_estimate_label = gtk_label_new(_("Provisional estimate: insufficient data"));
	gtk_label_set_xalign(GTK_LABEL(op->calibration_estimate_label), 0);
	gtk_widget_set_margin_start(op->calibration_estimate_label, 20);
	gtk_widget_set_margin_end(op->calibration_estimate_label, 20);
	GtkWidget *notice = gtk_label_new(
		_("The estimate may change; completing calibration is recommended."));
	gtk_label_set_xalign(GTK_LABEL(notice), 0);
	gtk_label_set_line_wrap(GTK_LABEL(notice), TRUE);
	gtk_widget_set_margin_start(notice, 20);
	gtk_widget_set_margin_end(notice, 20);
	gtk_box_pack_start(GTK_BOX(box), op->calibration_estimate_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), notice, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
	return box;
}

void op_set_snapshot(struct output_panel *op, struct snapshot *snst)
{
	op->snst = snst;
	if(snst->cal_state == 1)
		gtk_label_set_text(GTK_LABEL(op->calibration_estimate_label),
			_("Calibration complete"));
	else if(snst->cal_state == -1)
		gtk_label_set_text(GTK_LABEL(op->calibration_estimate_label),
			_("Calibration fit failed; improve the signal and try again."));
	else if(snst->cal_estimate_available) {
		char value[32];
		snprintf(value, sizeof(value), "%s%d.%d",
			snst->cal_estimate_result < 0 ? "-" : "+",
			abs(snst->cal_estimate_result) / 10,
			abs(snst->cal_estimate_result) % 10);
		char *text = g_strdup_printf(_("Provisional estimate: ~%s s/d"), value);
		gtk_label_set_text(GTK_LABEL(op->calibration_estimate_label), text);
		g_free(text);
	} else
		gtk_label_set_text(GTK_LABEL(op->calibration_estimate_label),
			_("Provisional estimate: insufficient data"));
	if(op->snst && op->snst->trace_zoom <= 0)
		op->snst->trace_zoom = 1.0;
	update_detection_status(op);
	gtk_widget_set_sensitive(op->clear_button, !snst->calibrate);
	gtk_widget_set_sensitive(op->paperstrip_tick_spin, !snst->calibrate);
	gtk_widget_set_sensitive(op->paperstrip_window_spin, !snst->calibrate);
	update_right_side(op);
	correlations_queue_draw(op);
}

void op_set_paperstrip_settings(struct output_panel *op, double tick_seconds, double window_seconds)
{
	op->updating_paperstrip_settings = 1;
	op->paperstrip_tick_seconds = tick_seconds;
	op->paperstrip_window_seconds = window_seconds;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(op->paperstrip_tick_spin), tick_seconds);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(op->paperstrip_window_spin), window_seconds);
	op->updating_paperstrip_settings = 0;
	gtk_widget_queue_draw(op->paperstrip_drawing_area);
}

void op_set_paperstrip_settings_callback(struct output_panel *op,
	void (*callback)(void *, double, double), void *data)
{
	op->paperstrip_settings_changed = callback;
	op->paperstrip_settings_data = data;
}

void op_set_border(struct output_panel *op, int i)
{
	gtk_container_set_border_width(GTK_CONTAINER(op->panel), i);
}

void op_destroy(struct output_panel *op)
{
	trend_free(op->trend);
	snapshot_destroy(op->snst);
	free(op);
}

struct output_panel *init_output_panel(struct computer *comp, struct snapshot *snst, int border)
{
	struct output_panel *op = malloc(sizeof(struct output_panel));

	op->computer = comp;
	op->snst = snst;
	op->guide_rate = 0;
	op->guide_time = 0;
	op->audio_meter_level = 0;
	op->audio_meter_time = 0;
	op->audio_meter_active = 0;
	op->audio_meter_transition_count = 0;
	op->audio_status = -1;
	op->paperstrip_tick_seconds = 10.0;
	op->paperstrip_window_seconds = 60.0;
	op->updating_paperstrip_settings = 0;
	op->paperstrip_settings_changed = NULL;
	op->paperstrip_settings_data = NULL;
	op->trace_centered = 0;
	// Only the live panel records; a snapshot tab gets a frozen copy, if any
	op->trend = comp ? trend_new() : NULL;
	op->trend_rate_range = 0;
	op->trend_be_range = 0;
	op->trend_span_s = 0;
	op->trend_span_combo = NULL;
	op->trend_page = NULL;
	op->correlations_page = NULL;
	op->paused = 0;
	op->correlation_span_combo = NULL;
	op->trend_be_combo = NULL;
	op->correlation_be_combo = NULL;
	memset(op->correlation_areas, 0, sizeof(op->correlation_areas));
	op->correlation_position = -1;
	op->settling_spin = NULL;
	op->settling_status = NULL;
	op->detection_status_label = NULL;
	op->shown_detection_status = DETECTION_STATUS_OK;
	op->detection_status_until = 0;
	op->settling_setting_changed = NULL;
	op->settling_setting_data = NULL;

	op->panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_container_set_border_width(GTK_CONTAINER(op->panel), border);

	// Info area on top
	op->output_drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(op->output_drawing_area, 0, OUTPUT_WINDOW_HEIGHT);
	gtk_box_pack_start(GTK_BOX(op->panel), op->output_drawing_area, FALSE, TRUE, 0);
	g_signal_connect (op->output_drawing_area, "draw", G_CALLBACK(output_draw_event), op);
	gtk_widget_set_events(op->output_drawing_area, GDK_EXPOSURE_MASK);

	GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start(GTK_BOX(op->panel), hbox2, TRUE, TRUE, 0);

	GtkWidget *vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_box_pack_start(GTK_BOX(hbox2), vbox2, FALSE, TRUE, 0);

	// Paperstrip
	op->paperstrip_drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(op->paperstrip_drawing_area, 300, 0);
	op->paperstrip_frame = titled_frame(op->paperstrip_drawing_area, _(TITLE_PAPERSTRIP));
	gtk_box_pack_start(GTK_BOX(vbox2), op->paperstrip_frame, TRUE, TRUE, 0);
	g_signal_connect (op->paperstrip_drawing_area, "draw", G_CALLBACK(paperstrip_draw_event), op);
	gtk_widget_set_events(op->paperstrip_drawing_area, GDK_EXPOSURE_MASK);

	GtkWidget *hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox3, FALSE, TRUE, 0);

	// < button
	GtkWidget *left_button = gtk_button_new_with_label("<");
	gtk_box_pack_start(GTK_BOX(hbox3), left_button, TRUE, TRUE, 0);
	g_signal_connect (left_button, "clicked", G_CALLBACK(handle_left), op);

	// CLEAR button
	if(comp) {
		/* TRANSLATORS: paper strip button: erase the trace. Short, shown in a narrow button */
		op->clear_button = gtk_button_new_with_label(_("Clear"));
		gtk_box_pack_start(GTK_BOX(hbox3), op->clear_button, TRUE, TRUE, 0);
		g_signal_connect (op->clear_button, "clicked", G_CALLBACK(handle_clear_trace), op);
		gtk_widget_set_sensitive(op->clear_button, !snst->calibrate);
	}

	// CENTER button
	/* TRANSLATORS: paper strip button: center the trace. Short, shown in a narrow button */
	GtkWidget *center_button = gtk_button_new_with_label(_("Center"));
	gtk_box_pack_start(GTK_BOX(hbox3), center_button, TRUE, TRUE, 0);
	g_signal_connect (center_button, "clicked", G_CALLBACK(handle_center_trace), op);

	GtkWidget *zoom_out_button = gtk_button_new_with_label("-");
	gtk_box_pack_start(GTK_BOX(hbox3), zoom_out_button, TRUE, TRUE, 0);
	g_signal_connect (zoom_out_button, "clicked", G_CALLBACK(handle_zoom_out), op);

	GtkWidget *zoom_reset_button = gtk_button_new_with_label("1x");
	gtk_box_pack_start(GTK_BOX(hbox3), zoom_reset_button, TRUE, TRUE, 0);
	g_signal_connect (zoom_reset_button, "clicked", G_CALLBACK(handle_zoom_reset), op);

	GtkWidget *zoom_in_button = gtk_button_new_with_label("+");
	gtk_box_pack_start(GTK_BOX(hbox3), zoom_in_button, TRUE, TRUE, 0);
	g_signal_connect (zoom_in_button, "clicked", G_CALLBACK(handle_zoom_in), op);

	// > button
	GtkWidget *right_button = gtk_button_new_with_label(">");
	gtk_box_pack_start(GTK_BOX(hbox3), right_button, TRUE, TRUE, 0);
	g_signal_connect (right_button, "clicked", G_CALLBACK(handle_right), op);

	GtkWidget *settings_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_box_pack_start(GTK_BOX(vbox2), settings_box, FALSE, TRUE, 0);
	/* TRANSLATORS: paper strip setting: time between marks on the vertical (Y) axis. Short */
	GtkWidget *tick_label = gtk_label_new(_("Y mark (s)"));
	gtk_box_pack_start(GTK_BOX(settings_box), tick_label, FALSE, FALSE, 0);
	op->paperstrip_tick_spin = gtk_spin_button_new_with_range(0.01, 3600, 1);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(op->paperstrip_tick_spin), 2);
	// + and - change the mark by 1 s (page keys by 10 s); finer values can be typed
	gtk_spin_button_set_increments(GTK_SPIN_BUTTON(op->paperstrip_tick_spin), 1, 10);
	gtk_entry_set_width_chars(GTK_ENTRY(op->paperstrip_tick_spin), 6);
	gtk_widget_set_tooltip_text(op->paperstrip_tick_spin,
		_("Time between marks on the vertical axis, in seconds."));
	gtk_box_pack_start(GTK_BOX(settings_box), op->paperstrip_tick_spin, FALSE, FALSE, 0);
	g_signal_connect(op->paperstrip_tick_spin, "value_changed",
		G_CALLBACK(handle_paperstrip_setting_change), op);

	/* TRANSLATORS: paper strip setting: time span shown on the vertical (Y) axis. Short */
	GtkWidget *window_label = gtk_label_new(_("Y window (s)"));
	gtk_box_pack_start(GTK_BOX(settings_box), window_label, FALSE, FALSE, 0);
	op->paperstrip_window_spin = gtk_spin_button_new_with_range(10, 86400, 10);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(op->paperstrip_window_spin), 0);
	gtk_entry_set_width_chars(GTK_ENTRY(op->paperstrip_window_spin), 6);
	gtk_widget_set_tooltip_text(op->paperstrip_window_spin,
		_("Fixed time span shown vertically. It does not change automatically."));
	gtk_box_pack_start(GTK_BOX(settings_box), op->paperstrip_window_spin, FALSE, FALSE, 0);
	g_signal_connect(op->paperstrip_window_spin, "value_changed",
		G_CALLBACK(handle_paperstrip_setting_change), op);

	// Right side: waveform panels or the long-term trend, chosen with the
	// switcher above them; calibration instructions while calibrating
	GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_box_pack_start(GTK_BOX(hbox2), right_box, TRUE, TRUE, 0);
	op->right_stack = gtk_stack_new();
	op->right_switcher = gtk_stack_switcher_new();
	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(op->right_switcher), GTK_STACK(op->right_stack));
	gtk_widget_set_halign(op->right_switcher, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(right_box), op->right_switcher, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(right_box), op->right_stack, TRUE, TRUE, 0);

	GtkWidget *vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL,10);
	gtk_stack_add_titled(GTK_STACK(op->right_stack), vbox3, "panels", _("Waveforms"));

	// Trend page: statistics and graphs since the start, controls below
	GtkWidget *trend_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_stack_add_titled(GTK_STACK(op->right_stack), trend_page, "trend", _("Trend"));
	op->trend_drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(op->trend_drawing_area, 300, 0);
	gtk_box_pack_start(GTK_BOX(trend_page),
		titled_frame(op->trend_drawing_area, _("Trend: rate, amplitude, and BE over time")), TRUE, TRUE, 0);
	g_signal_connect(op->trend_drawing_area, "draw", G_CALLBACK(trend_draw_event), op);
	gtk_widget_set_events(op->trend_drawing_area, GDK_EXPOSURE_MASK);

	GtkWidget *trend_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_box_pack_start(GTK_BOX(trend_page), trend_box, FALSE, TRUE, 0);
	if(comp) {
		/* TRANSLATORS: Trend page button: start the statistics over. Short, shown in a narrow button */
		GtkWidget *reset = gtk_button_new_with_label(_("Reset"));
		gtk_widget_set_tooltip_text(reset, _("Start the statistics over.\n"
			"They also start over when bph, lift angle,\n"
			"cal or the audio input change."));
		gtk_box_pack_start(GTK_BOX(trend_box), reset, FALSE, FALSE, 0);
		g_signal_connect(reset, "clicked", G_CALLBACK(handle_trend_reset), op);
	}
	/* TRANSLATORS: Trend page: label of the rate range selector (Auto, +-10 s/d, +-50 s/d). Short */
	gtk_box_pack_start(GTK_BOX(trend_box), gtk_label_new(_("Rate")), FALSE, FALSE, 0);
	GtkWidget *range_combo = gtk_combo_box_text_new();
	/* TRANSLATORS: Trend page rate range: chosen automatically. Short, shown in a narrow combo box */
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(range_combo), "0", _("Auto"));
	/* TRANSLATORS: Trend page rate range; translate the unit s/d as in the other Trend page texts */
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(range_combo), "10", _("\xC2\xB1" "10 s/d"));
	/* TRANSLATORS: Trend page rate range; translate the unit s/d as in the other Trend page texts */
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(range_combo), "50", _("\xC2\xB1" "50 s/d"));
	gtk_combo_box_set_active_id(GTK_COMBO_BOX(range_combo), "0");
	gtk_box_pack_start(GTK_BOX(trend_box), range_combo, FALSE, FALSE, 0);
	g_signal_connect(range_combo, "changed", G_CALLBACK(handle_trend_rate_range), op);
	gtk_box_pack_start(GTK_BOX(trend_box), gtk_label_new(_("B.E.")), FALSE, FALSE, 0);
	op->trend_be_combo = op_create_trend_be_combo(op);
	gtk_box_pack_start(GTK_BOX(trend_box), op->trend_be_combo, FALSE, FALSE, 0);
	/* TRANSLATORS: Trend page: label of the time span selector (All, Last 1 h, ...). Short */
	gtk_box_pack_start(GTK_BOX(trend_box), gtk_label_new(_("Show")), FALSE, FALSE, 0);
	op->trend_span_combo = op_create_trend_span_combo(op);
	gtk_box_pack_start(GTK_BOX(trend_box), op->trend_span_combo, FALSE, FALSE, 0);

	if(comp) {
		gtk_box_pack_start(GTK_BOX(trend_box), gtk_label_new(_("Settling time (s)")),
			FALSE, FALSE, 0);
		op->settling_spin = gtk_spin_button_new_with_range(0, TREND_MAX_SETTLING_S, 1);
		gtk_widget_set_tooltip_text(op->settling_spin,
			_("Seconds to wait at startup and after a position change; 0 disables settling."));
		gtk_box_pack_start(GTK_BOX(trend_box), op->settling_spin, FALSE, FALSE, 0);
		g_signal_connect(op->settling_spin, "value_changed", G_CALLBACK(handle_settling_setting), op);
		op->settling_status = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(trend_box), op->settling_status, FALSE, FALSE, 0);
	}
	GtkWidget *correlations_page = correlations_create_page(op);
	gtk_stack_add_titled(GTK_STACK(op->right_stack), correlations_page,
		"correlations", _("Correlations"));
	op->trend_page = trend_page;
	op->correlations_page = correlations_page;
	// No title: the switcher does not offer it, calibration shows it by itself
	GtkWidget *cal_info = titled_frame(make_calibration_instructions(op), _("Calibration instructions"));
	gtk_stack_add_named(GTK_STACK(op->right_stack), cal_info, "calibration");

	// Tic waveform area
	op->tic_drawing_area = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox3), titled_frame(op->tic_drawing_area,
			_("Tic: averaged sound of the first beat")), TRUE, TRUE, 0);
	g_signal_connect (op->tic_drawing_area, "draw", G_CALLBACK(tic_draw_event), op);
	gtk_widget_set_events(op->tic_drawing_area, GDK_EXPOSURE_MASK);

	// Toc waveform area
	op->toc_drawing_area = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox3), titled_frame(op->toc_drawing_area,
			_("Toc: averaged sound of the second beat")), TRUE, TRUE, 0);
	g_signal_connect (op->toc_drawing_area, "draw", G_CALLBACK(toc_draw_event), op);
	gtk_widget_set_events(op->toc_drawing_area, GDK_EXPOSURE_MASK);

	// Period waveform area
	op->period_drawing_area = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox3), titled_frame(op->period_drawing_area,
			_("Full period: tic and toc (highlighted)")), TRUE, TRUE, 0);
	g_signal_connect (op->period_drawing_area, "draw", G_CALLBACK(period_draw_event), op);
	gtk_widget_set_events(op->period_drawing_area, GDK_EXPOSURE_MASK);

	op->detection_status_label = gtk_label_new(" ");
	gtk_label_set_xalign(GTK_LABEL(op->detection_status_label), 0);
	gtk_label_set_single_line_mode(GTK_LABEL(op->detection_status_label), TRUE);
	gtk_label_set_ellipsize(GTK_LABEL(op->detection_status_label), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start(GTK_BOX(vbox3), op->detection_status_label, FALSE, FALSE, 0);

#ifdef DEBUG
	op->debug_drawing_area = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox3), titled_frame(op->debug_drawing_area,
			"Debug: processed signal"), TRUE, TRUE, 0);
	g_signal_connect (op->debug_drawing_area, "draw", G_CALLBACK(debug_draw_event), op);
	gtk_widget_set_events(op->debug_drawing_area, GDK_EXPOSURE_MASK);
#endif

	// GtkStack only switches to visible children
	gtk_widget_show_all(vbox3);
	gtk_widget_show_all(trend_page);
	gtk_widget_show_all(correlations_page);
	gtk_widget_show_all(cal_info);
	update_right_side(op);
	update_detection_status(op);

	return op;
}
