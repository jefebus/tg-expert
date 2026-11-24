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
 * A stale reading keeps its colour but is drawn faded. */
cairo_pattern_t *black,*white,*red,*green;

struct color { double r, g, b; };
static const struct color COL_TIC  = {.722, .722, .722};
static const struct color COL_TOC  = {.980, .761, .020};
static const struct color COL_AMP  = {1, .310, .816}; // magenta #FF4FD0
static const struct color COL_RATE = {.302, .651, 1};
static const struct color COL_BE   = {.710, .486, 1}; // purple #B57CFF
static const struct color COL_WAVE = {.902, .902, .902}; // neutral, for both beats
static const struct color COL_AXIS = {1, 1, 1};

#define ALPHA_STALE       0.4
#define ALPHA_AXIS_MAJOR  0.55
#define ALPHA_AXIS_MINOR  0.25
#define ALPHA_BAND        0.18	// tic / toc windows in the period panel

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

static void set_dotted(cairo_t *c, int dotted)
{
	static const double dots[2] = {2, 4};
	if(dotted)
		cairo_set_dash(c, dots, 2, 0);
	else
		cairo_set_dash(c, NULL, 0, 0);
}

/* Line style of an axis or grid: major = solid, minor = dotted and fainter.
 * Call set_dotted(c, 0) after stroking to go back to solid lines. */
static void set_axis_style(cairo_t *c, struct color col, int major)
{
	set_color(c, col, major ? ALPHA_AXIS_MAJOR : ALPHA_AXIS_MINOR);
	set_dotted(c, !major);
}

static void draw_graph(double a, double b, cairo_t *c, struct processing_buffers *p, GtkWidget *da)
{
	GtkAllocation temp;
	gtk_widget_get_allocation (da, &temp);
	int width = temp.width;
	int height = temp.height;

	int n;

	int first = 1;
	for(n=0; n<2*width; n++) {
		int i = n < width ? n : 2*width - 1 - n;
		double x = fmod(a + i * (b-a) / width, p->period);
		if(x < 0) x += p->period;
		int j = floor(x);
		double y;

		if(p->waveform[j] <= 0) y = 0;
		else y = p->waveform[j] * 0.4 / p->waveform_max;

		int k = round(y*height);
		if(n < width) k = -k;
		if(first) {
			cairo_move_to(c,i+.5,height/2+k+.5);
			first = 0;
		} else
			cairo_line_to(c,i+.5,height/2+k+.5);
	}
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

static int waveform_negative_span(struct snapshot *s)
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

static void expose_waveform(
			struct output_panel *op,
			GtkWidget *da,
			cairo_t *c,
			struct color wave_color,
			int (*get_offset)(struct processing_buffers*),
			double (*get_pulse)(struct processing_buffers*))
{
	cairo_init(c);

	GtkAllocation temp;
	gtk_widget_get_allocation(da, &temp);

	int width = temp.width;
	int height = temp.height;

	gtk_widget_get_allocation(gtk_widget_get_toplevel(da), &temp);
	int font = temp.width / 90;
	if(font < 12)
		font = 12;
	int i;
	struct snapshot *snst = op->snst;
	struct processing_buffers *p = snst->rate_only ? NULL : snst->pb;
	int negative_span = waveform_negative_span(snst);
	int total_span = negative_span + POSITIVE_SPAN;
	static const int steps[] = {5, 10, 20, 25, 50, 100, 200, 500};
	int step = 0;
	while(step < (int)G_N_ELEMENTS(steps) - 1 &&
			steps[step] * (double)width / total_span < 40)
		step++;
	int major = steps[step];
	int minor = major / 5;

	cairo_set_font_size(c,font);

	// Time axis (bottom half, white)
	for(i = -(negative_span / minor) * minor; i < POSITIVE_SPAN; i += minor) {
		if(i <= -negative_span) continue;
		int x = (negative_span + i) * width / total_span;
		cairo_move_to(c, x + .5, height / 2 + .5);
		cairo_line_to(c, x + .5, height - .5);
		set_axis_style(c, COL_AXIS, !(i%major));
		cairo_stroke(c);
	}
	set_dotted(c, 0);
	cairo_set_source(c,white);
	for(i = -(negative_span / major) * major; i < POSITIVE_SPAN; i += major) {
		if(i > -negative_span) {
			int x = (negative_span + i) * width / total_span;
			char s[10];
			sprintf(s,"%d",i);
			cairo_move_to(c,x+font/4,height-font/2);
			cairo_show_text(c,s);
		}
	}

	cairo_text_extents_t extents;

	cairo_text_extents(c,"ms",&extents);
	cairo_move_to(c,width - extents.x_advance - font/4,height-font/2);
	cairo_show_text(c,"ms");

	int old = snst->is_old;
	double period = p ? p->period / snst->sample_rate : 7200. / snst->guessed_bph;

	// Amplitude scale (top half, green): solid every 50 deg, dotted every 10 deg
	for(i = 10; i < 360; i+=10) {
		if(2*i < snst->la) continue;
		double t = period*amplitude_to_time(snst->la,i);
		if(t > .001 * negative_span) continue;
		int x = round(width * (negative_span - 1000*t) / total_span);
		cairo_move_to(c, x+.5, .5);
		cairo_line_to(c, x+.5, height / 2 + .5);
		set_axis_style(c, COL_AMP, !(i % 50));
		cairo_stroke(c);
	}
	set_dotted(c, 0);

	double last_x = 0;
	set_color(c, COL_AMP, 1);
	for(i = 50; i < 360; i+=50) {
		double t = period*amplitude_to_time(snst->la,i);
		if(t > .001 * negative_span) continue;
		int x = round(width * (negative_span - 1000*t) / total_span);
		if(x > last_x) {
			char s[10];

			sprintf(s,"%d",abs(i));
			cairo_move_to(c, x + font/4, font * 3 / 2);
			cairo_show_text(c,s);
			cairo_text_extents(c,s,&extents);
			last_x = x + font/4 + extents.x_advance;
		}
	}

	cairo_text_extents(c,"deg",&extents);
	cairo_move_to(c,width - extents.x_advance - font/4,font * 3 / 2);
	cairo_show_text(c,"deg");

	if(p) {
		double span = 0.001 * snst->sample_rate;
		int offset = get_offset(p);

		double a = offset - span * negative_span;
		double b = offset + span * POSITIVE_SPAN;

		draw_graph(a,b,c,p,da);

		set_signal_color(c, wave_color, old);
		cairo_stroke_preserve(c);
		cairo_fill(c);

		// Detected pulse position: reference colour
		double pulse = get_pulse(p);
		if(pulse > 0) {
			int x = round((negative_span - pulse / span) * width / total_span);
			cairo_move_to(c, x, 1);
			cairo_line_to(c, x, height - 1);
			set_signal_color(c, COL_RATE, old);
			cairo_set_line_width(c,2);
			cairo_stroke(c);
		}
	} else {
		cairo_move_to(c, .5, height / 2 + .5);
		cairo_line_to(c, width - .5, height / 2 + .5);
		set_color(c, COL_AXIS, ALPHA_STALE);
		cairo_stroke(c);
	}
}

static int get_tic(struct processing_buffers *p)
{
	return p->tic;
}

static int get_toc(struct processing_buffers *p)
{
	return p->toc;
}

static double get_tic_pulse(struct processing_buffers *p)
{
	return p->tic_pulse;
}

static double get_toc_pulse(struct processing_buffers *p)
{
	return p->toc_pulse;
}

static gboolean tic_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	UNUSED(widget);
	expose_waveform(op, op->tic_drawing_area, c, COL_TIC, get_tic, get_tic_pulse);
	return FALSE;
}

static gboolean toc_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	UNUSED(widget);
	expose_waveform(op, op->toc_drawing_area, c, COL_TOC, get_toc, get_toc_pulse);
	return FALSE;
}

/* Vertical grid with ms labels for a window starting at t_left ms and span_ms wide.
 * Major (solid, labelled) lines use a round step giving at most ~10 labels; minor
 * (dotted) lines divide each major step in 5. */
static void draw_ms_axis(cairo_t *c, int width, int height, int font, double t_left, double span_ms)
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

	for(k = k0; k <= k1; k++) {
		double x = (k * minor - t_left) * width / span_ms;
		if(x <= 0 || x >= width) continue;
		cairo_move_to(c, round(x) + .5, .5);
		cairo_line_to(c, round(x) + .5, height - .5);
		set_axis_style(c, COL_AXIS, !(k % 5));
		cairo_stroke(c);
	}
	set_dotted(c, 0);

	cairo_set_font_size(c, font);
	cairo_text_extents_t extents;
	cairo_text_extents(c, "ms", &extents);
	double unit_x = width - extents.x_advance - font/4;

	cairo_set_source(c, white);
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

static gboolean period_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	UNUSED(widget);
	cairo_init(c);

	GtkAllocation temp;
	gtk_widget_get_allocation (op->period_drawing_area, &temp);

	int width = temp.width;
	int height = temp.height;

	struct snapshot *snst = op->snst;
	struct processing_buffers *p = snst->rate_only ? NULL : snst->pb;
	int old = snst->is_old;

	double toc,a=0,b=0;
	int negative_span = waveform_negative_span(snst);

	if(p) {
		toc = p->tic < p->toc ? p->toc : p->toc + p->period;
		a = ((double)p->tic + toc)/2 - p->period/2;
		b = ((double)p->tic + toc)/2 + p->period/2;

		cairo_move_to(c, (p->tic - a - negative_span*.001*snst->sample_rate) * width/p->period, 0);
		cairo_line_to(c, (p->tic - a - negative_span*.001*snst->sample_rate) * width/p->period, height);
		cairo_line_to(c, (p->tic - a + POSITIVE_SPAN*.001*snst->sample_rate) * width/p->period, height);
		cairo_line_to(c, (p->tic - a + POSITIVE_SPAN*.001*snst->sample_rate) * width/p->period, 0);
		set_color(c, COL_TIC, ALPHA_BAND); // tic window: grey tint
		cairo_fill(c);

		cairo_move_to(c, (toc - a - negative_span*.001*snst->sample_rate) * width/p->period, 0);
		cairo_line_to(c, (toc - a - negative_span*.001*snst->sample_rate) * width/p->period, height);
		cairo_line_to(c, (toc - a + POSITIVE_SPAN*.001*snst->sample_rate) * width/p->period, height);
		cairo_line_to(c, (toc - a + POSITIVE_SPAN*.001*snst->sample_rate) * width/p->period, 0);
		set_color(c, COL_TOC, ALPHA_BAND); // toc window: gold tint
		cairo_fill(c);
	}

	// Time axis in ms, 0 at the tic (same convention as the tic/toc panels)
	{
		GtkAllocation top;
		gtk_widget_get_allocation(gtk_widget_get_toplevel(op->period_drawing_area), &top);
		int font = top.width / 90;
		if(font < 12)
			font = 12;

		double sr = snst->sample_rate > 0 ? snst->sample_rate : snst->nominal_sr;
		double span_ms = p ? 1000. * p->period / sr : 1000. * 7200. / snst->guessed_bph;
		double t_left = p ? 1000. * (a - p->tic) / sr : 0;
		draw_ms_axis(c, width, height, font, t_left, span_ms);
	}

	if(p) {
		draw_graph(a,b,c,p,op->period_drawing_area);

		set_signal_color(c, COL_WAVE, old);
		cairo_stroke_preserve(c);
		cairo_fill(c);
	} else {
		cairo_move_to(c, .5, height / 2 + .5);
		cairo_line_to(c, width - .5, height / 2 + .5);
		set_color(c, COL_AXIS, ALPHA_STALE);
		cairo_stroke(c);
	}

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

#define LEGEND_DOT  0
#define LEGEND_LINE 1
#define LEGEND_DASH 2

/* One legend entry (marker and text) at baseline y; returns where the next one starts */
static double legend_item(cairo_t *c, double x, double y, int style, struct color col, const char *text)
{
	static const double dashes[2] = {4, 3};
	cairo_text_extents_t e;

	set_color(c, col, 1);
	if(style == LEGEND_DOT) {
		cairo_arc(c, x + 3, y - 3.5, 3, 0, 2 * M_PI);
		cairo_fill(c);
		x += 10;
	} else {
		cairo_set_line_width(c, 1.5);
		if(style == LEGEND_DASH)
			cairo_set_dash(c, dashes, 2, 0);
		cairo_move_to(c, x, y - 3.5);
		cairo_line_to(c, x + 16, y - 3.5);
		cairo_stroke(c);
		set_dotted(c, 0);
		x += 20;
	}
	set_color(c, COL_AXIS, 0.85);
	cairo_text_extents(c, text, &e);
	cairo_move_to(c, x, y);
	cairo_show_text(c, text);
	return x + e.x_advance + 14;
}

/* Paperstrip zoom levels (trace_zoom). At 1x the window is PAPERSTRIP_WINDOW_MS
 * wide (PAPERSTRIP_WINDOW_MS_CAL in calibration), at 2x half of it, and so on. */
static const double paperstrip_zoom_levels[] = {0.25, 0.5, 1, 2, 4, 8, 0};

/* The level nearest to a saved trace_zoom (older versions stored other factors) */
static int paperstrip_zoom_index(double trace_zoom)
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
static double paperstrip_zoom_factor(const struct snapshot *snst)
{
	double zoom = paperstrip_zoom_levels[paperstrip_zoom_index(snst->trace_zoom)];
	int bph = snst->guessed_bph > 0 ? snst->guessed_bph : (snst->bph ? snst->bph : DEFAULT_BPH);
	double period_ms = snst->calibrate ? 1000. : 3600000. / bph;
	double wanted_ms = (snst->calibrate ? PAPERSTRIP_WINDOW_MS_CAL : PAPERSTRIP_WINDOW_MS) / zoom;
	double n = period_ms / wanted_ms;
	return n < 1 ? 1 : n;
}

static int center_trace(struct output_panel *op);

static gboolean paperstrip_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	int i;
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
	uint64_t time = snst->timestamp ? snst->timestamp : get_timestamp(snst->is_light);
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
		if(valid_events_wp && snst->events[snst->events_wp])
			slope = - guide_rate(op) * zoom_factor / (3600. * 24.);
		else
			op->guide_time = 0; // no trace: start over when it comes back
	}
	if(!isfinite(sweep) || sweep <= 0 || !isfinite(zoom_factor) || zoom_factor <= 0)
		return FALSE;
	cairo_init(c);

	GtkAllocation temp;
	gtk_widget_get_allocation (op->paperstrip_drawing_area, &temp);

	int width = temp.width;
	int height = temp.height;

	int stopped = 0;
	if(valid_events_wp && snst->events[snst->events_wp] &&
		time > 5 * snst->nominal_sr + snst->events[snst->events_wp])
		stopped = 1;

	int strip_width = round(width / (1 + PAPERSTRIP_MARGIN));
	if(strip_width <= 0)
		return FALSE;
	int left_margin = (width - strip_width) / 2;
	int right_margin = (width + strip_width) / 2;

	GtkAllocation toplevel;
	gtk_widget_get_allocation(gtk_widget_get_toplevel(widget), &toplevel);
	int font = toplevel.width / 90;
	if(font < 12)
		font = 12;

	/* Layout: scales on top, the data in the middle with the newest beats at
	 * its bottom edge (older ones move up), legend at the bottom. */
	const int scale_rows = snst->calibrate ? 1 : 2;
	const double plot_top = scale_rows * (font + 4) + 4;
	const double plot_bottom = height - (font + 8);
	if(plot_bottom - plot_top < 20)
		return FALSE;
	double y_sample_rate = snst->calibrate ? snst->nominal_sr : safe_sample_rate;
	double row_samples = y_sample_rate * op->paperstrip_window_seconds /
		(plot_bottom - plot_top);
	if(!isfinite(row_samples) || row_samples <= 0)
		return FALSE;

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
#define AMP_X(a)   (left_margin + ((a) - amp_min) / (amp_max - amp_min) * (strip_width - 1))
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
	double mark_samples = op->paperstrip_tick_seconds * y_sample_rate;
	double mark_rows = mark_samples / row_samples;
	if(!isfinite(mark_rows) || mark_rows <= 0)
		return FALSE;
	int64_t last_mark_index = (int64_t)floor(now / mark_samples);
	int64_t mark_stride = (int64_t)ceil(1.0 / mark_rows);
	if(mark_stride < 1) mark_stride = 1;
	int64_t label_stride = (int64_t)ceil((font + 8.0) / (mark_rows * mark_stride));
	if(label_stride < 1) label_stride = 1;

	// Everything below is clipped to the data area
	cairo_save(c);
	cairo_rectangle(c, 0, plot_top, width, plot_bottom - plot_top);
	cairo_clip(c);
	cairo_set_line_width(c, 1);

	// Vertical grid (white, dotted): the ruler lines, which are also the amplitude lines
	for(k = 1; k * ruler < window_ms - ruler / 2; k++) {
		double x = round(RULER_X(k * ruler)) + .5;
		cairo_move_to(c, x, plot_top);
		cairo_line_to(c, x, plot_bottom);
	}
	set_axis_style(c, COL_AXIS, 0);
	cairo_stroke(c);

	// Window edges: a beat leaving one edge comes back at the other
	cairo_move_to(c, left_margin + .5, plot_top);
	cairo_line_to(c, left_margin + .5, plot_bottom);
	cairo_move_to(c, right_margin + .5, plot_top);
	cairo_line_to(c, right_margin + .5, plot_bottom);
	set_axis_style(c, COL_AXIS, 1);
	cairo_stroke(c);

	// Custom y marks are dotted; minute marks are drawn separately as solid guides.
	cairo_set_font_size(c, font * 0.8);
	for(int64_t mark_offset = 0; mark_offset <= last_mark_index; mark_offset += mark_stride) {
		int64_t mark_index = last_mark_index - mark_offset;
		double mark_seconds = mark_index * op->paperstrip_tick_seconds;
		double mark_time = mark_seconds * y_sample_rate;
		double y = plot_bottom - 0.5 - round((now - mark_time) / row_samples);
		if(y < plot_top) break;
		cairo_move_to(c, .5, y);
		cairo_line_to(c, width-.5, y);
		set_axis_style(c, COL_AXIS, 0);
		cairo_stroke(c);
		set_dotted(c, 0);

		if((mark_offset / mark_stride) % label_stride == 0) {
			unsigned long long minutes = (unsigned long long)(mark_seconds / 60.0);
			double seconds = fmod(mark_seconds, 60.0);
			if(op->paperstrip_tick_seconds < 10.0)
				snprintf(s, sizeof(s), "%llu:%05.2f", minutes, seconds);
			else
				snprintf(s, sizeof(s), "%llu:%02d", minutes, (int)round(seconds));
			cairo_text_extents(c, s, &extents);
			set_color(c, COL_AXIS, 0.7);
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
		set_axis_style(c, COL_AXIS, 1);
		cairo_stroke(c);
		set_dotted(c, 0);
		snprintf(s, sizeof(s), "%llu:00", (unsigned long long)minute_index);
		cairo_text_extents(c, s, &extents);
		set_color(c, COL_AXIS, 0.7);
		cairo_move_to(c, fmax(1, left_margin - extents.x_advance - 3), y - 2);
		cairo_show_text(c, s);
	}

	// Rate guide: lines with the slope of the measured rate (in calibration, of
	// the current cal value), dashed in the reference colour. The trace should
	// run parallel to them. Age is counted in rows from the bottom edge.
	cairo_set_line_width(c, 1.2);
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
		set_color(c, COL_RATE, 0.9);
		cairo_stroke(c);
		set_dotted(c, 0);
	}

	// Amplitude history (green); higher amplitude is further right, like in the tic and toc panels
	if(!snst->calibrate && snst->amps_count && snst->amps && snst->amps_time) {
		int path_started = 0;
		cairo_set_line_width(c, 1.5);
		for(i = snst->amps_wp;;) {
			if(!snst->amps_time[i]) break;
			double event_age = now - snst->amps_time[i];
			double row = floor(event_age / row_samples);
			if(row >= plot_bottom) break;

			double amp_deg = snst->la * snst->amps[i];
			double amp_norm = (amp_deg - amp_min) / (amp_max - amp_min);
			if(amp_norm < 0) amp_norm = 0;
			if(amp_norm > 1) amp_norm = 1;

			double column = left_margin + amp_norm * (strip_width - 1);
			double y = plot_bottom - row - .5;
			if(!path_started) {
				cairo_move_to(c, column, y);
				path_started = 1;
			} else {
				cairo_line_to(c, column, y);
			}

			if(--i < 0) i = snst->amps_count - 1;
			if(i == snst->amps_wp) break;
		}
		if(path_started) {
			set_signal_color(c, COL_AMP, stopped);
			cairo_stroke(c);
		}
	}

	// Beats: one dot per beat, tic in grey and toc in gold (neutral white for
	// the reference ticks of a calibration). Faded when the signal is lost.
	// Repeat the fixed ms window between the margins, including copies in
	// the outer margins so dots remain visible on both sides of a crossing.
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
		struct color dot = COL_WAVE;
		if(!snst->calibrate && snst->events_tictoc)
			dot = snst->events_tictoc[i] ? COL_TIC : COL_TOC;
		set_signal_color(c, dot, stopped);
		const double dot_left = (PAPERSTRIP_DOT_SIZE - 1) / 2;
		for(x -= strip_width; x - dot_left < width; x += strip_width) {
			if(x - dot_left + PAPERSTRIP_DOT_SIZE <= 0) continue;
			// A square of PAPERSTRIP_DOT_SIZE px: around the beat position across,
			// from its row upwards, so that the newest row stays inside the plot
			cairo_rectangle(c, x - dot_left, plot_bottom - row - PAPERSTRIP_DOT_SIZE,
				PAPERSTRIP_DOT_SIZE, PAPERSTRIP_DOT_SIZE);
			cairo_fill(c);
		}
		if(--i < 0) i = snst->events_count - 1;
		if(i == snst->events_wp) break;
	}
	cairo_restore(c);

	// Scales on top: amplitude in degrees (green), position in the beat in ms (white)
	cairo_set_line_width(c, 1);
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
		set_color(c, COL_AMP, 1);
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
	cairo_set_source(c, white);
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

	// Legend
	cairo_set_font_size(c, font * 0.9);
	double lx = left_margin;
	const double ly = height - 5;
	if(snst->calibrate) {
		/* TRANSLATORS: legend of the paper strip, short, shown in a narrow row */
		lx = legend_item(c, lx, ly, LEGEND_DOT, COL_WAVE, _("reference tick"));
		/* TRANSLATORS: legend of the paper strip, short, shown in a narrow row. "cal" is the calibration value and stays as is */
		lx = legend_item(c, lx, ly, LEGEND_DASH, COL_RATE, _("current cal"));
	} else {
		/* TRANSLATORS: legend of the paper strip: the first sound of a watch beat (tic-toc). Short, shown in a narrow row */
		lx = legend_item(c, lx, ly, LEGEND_DOT, COL_TIC, _("tic"));
		/* TRANSLATORS: legend of the paper strip: the second sound of a watch beat (tic-toc). Short, shown in a narrow row */
		lx = legend_item(c, lx, ly, LEGEND_DOT, COL_TOC, _("toc"));
		/* TRANSLATORS: legend of the paper strip, short, shown in a narrow row */
		lx = legend_item(c, lx, ly, LEGEND_LINE, COL_AMP, _("amplitude"));
		/* TRANSLATORS: legend of the paper strip: dashed guide line showing the rate. Short, shown in a narrow row */
		lx = legend_item(c, lx, ly, LEGEND_DASH, COL_RATE, _("rate guide"));
	}

#undef AMP_X
#undef RULER_X
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

/* Text with its left edge (align 0), centre (0.5) or right edge (1) at x */
static void trend_text(cairo_t *c, double x, double y, double align, const char *s)
{
	cairo_text_extents_t e;
	cairo_text_extents(c, s, &e);
	cairo_move_to(c, x - align * e.x_advance, y);
	cairo_show_text(c, s);
}

static void trend_message(cairo_t *c, int width, int height, int font, const char *line1, const char *line2)
{
	cairo_set_font_size(c, font);
	set_color(c, COL_AXIS, 0.85);
	trend_text(c, width / 2.0, height / 2.0 - font, 0.5, line1);
	if(line2) {
		cairo_set_font_size(c, font * 0.8);
		set_color(c, COL_AXIS, 0.6);
		trend_text(c, width / 2.0, height / 2.0 + font * 0.6, 0.5, line2);
	}
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

/* Collapse the readings between t0 and t1 into pixel columns, at most max of
 * them. Series: rate, amplitude (readings without one are skipped), or beat error. */
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
 * column faint, its mean as a line. Values outside the range stay at the edge. */
static void trend_draw_series(cairo_t *c, const struct trend_column *cols, int n,
	double y_top, double y_bottom, double v_min, double v_max, struct color col)
{
#define VY(v) (y_bottom - (fmin(fmax((v), v_min), v_max) - v_min) / (v_max - v_min) * (y_bottom - y_top))
	cairo_set_line_width(c, 1);
	for(int i = 0; i < n; i++) {
		if(cols[i].n < 2) continue;
		cairo_move_to(c, cols[i].x, VY(cols[i].min));
		cairo_line_to(c, cols[i].x, VY(cols[i].max));
	}
	set_color(c, col, 0.35);
	cairo_stroke(c);

	cairo_set_line_width(c, 1.5);
	for(int i = 0; i < n; i++) {
		double y = VY(cols[i].sum / cols[i].n);
		// A reading on its own is drawn as a short dash, so that it shows
		int alone = (i == 0 || cols[i].gap_before) && (i + 1 == n || cols[i+1].gap_before);
		if(alone) {
			cairo_move_to(c, cols[i].x - 1, y);
			cairo_line_to(c, cols[i].x + 1, y);
		} else if(i == 0 || cols[i].gap_before)
			cairo_move_to(c, cols[i].x, y);
		else
			cairo_line_to(c, cols[i].x, y);
	}
	set_color(c, col, 1);
	cairo_stroke(c);
#undef VY
}

/* Horizontal grid line with its label left of the graph */
static void trend_hline(cairo_t *c, double gx, double gw, double y, int major, const char *label)
{
	cairo_move_to(c, gx, round(y) + .5);
	cairo_line_to(c, gx + gw, round(y) + .5);
	set_axis_style(c, COL_AXIS, major);
	cairo_stroke(c);
	set_dotted(c, 0);
	set_color(c, COL_AXIS, 0.7);
	trend_text(c, gx - 4, y + 4, 1, label);
}

static void trend_draw_average(cairo_t *c, double gx, double gw,
	double y_top, double y_bottom, double v_min, double v_max,
	const struct trend_stats *stats, struct color col)
{
	double mean = trend_stats_mean(stats);
	if(!stats->n || mean < v_min || mean > v_max)
		return;

	static const double dashes[2] = {6, 4};
	double y = round(y_bottom - (mean - v_min) / (v_max - v_min) *
		(y_bottom - y_top)) + .5;
	cairo_set_dash(c, dashes, 2, 0);
	cairo_move_to(c, gx, y);
	cairo_line_to(c, gx + gw, y);
	set_color(c, col, 0.8);
	cairo_stroke(c);
	set_dotted(c, 0);
}

static gboolean trend_draw_event(GtkWidget *widget, cairo_t *c, struct output_panel *op)
{
	cairo_init(c);
	GtkAllocation a;
	gtk_widget_get_allocation(widget, &a);
	const int width = a.width, height = a.height;
	GtkAllocation toplevel;
	gtk_widget_get_allocation(gtk_widget_get_toplevel(widget), &toplevel);
	int font = toplevel.width / 90;
	if(font < 12)
		font = 12;

	const struct trend *t = op->trend;
	if(!t) {
		trend_message(c, width, height, font, _("No long-term statistics"),
			_("This snapshot was saved without them."));
		return FALSE;
	}
	if(!t->count) {
		if(op->snst && op->snst->calibrate)
			trend_message(c, width, height, font, _("Not recorded during calibration"), NULL);
		else if(t->frozen)
			trend_message(c, width, height, font, _("No readings were recorded"), NULL);
		else
			trend_message(c, width, height, font, _("Waiting for a steady signal"),
				_("A reading is recorded every 2 s while the signal is good."));
		return FALSE;
	}

	char s[96];
	int i;
	const double line = font * 1.35;
	const double elapsed = trend_elapsed(t);
	const struct trend_sample *last = &t->samples[t->count - 1];
	double y = font + 4;

	// Statistics since the start
	const double label_w = width * 0.24;
	double col_w = (width - label_w - 6) / 6;
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
	set_color(c, COL_AXIS, 0.6);
	for(i = 0; i < 6; i++)
		trend_text(c, label_w + (i + 1) * col_w, y, 1, _(heads[i]));
	y += line;

	const struct {
		const char *name;
		struct color col;
		const struct trend_stats *st;
		double now;
		int digits, sign;
	} rows[3] = {
		/* TRANSLATORS: Trend page row: rate in seconds per day (s/d). Short, shown in a narrow column */
		{N_("Rate s/d"), COL_RATE, &t->rate, last->rate, 1, 1},
		/* TRANSLATORS: Trend page row: amplitude in degrees. Keep the degree sign. Short, shown in a narrow column */
		{N_("Ampl. \xC2\xB0"), COL_AMP, &t->amp, last->amp, 0, 0},
		/* TRANSLATORS: Trend page row: beat error in milliseconds (ms). Short, shown in a narrow column */
		{N_("Beat err. ms"), COL_BE, &t->be, last->be, 1, 0},
	};
	cairo_set_font_size(c, font);
	for(int r = 0; r < 3; r++) {
		const struct trend_stats *st = rows[r].st;
		set_color(c, rows[r].col, 1);
		trend_text(c, 4, y, 0, _(rows[r].name));
		set_color(c, COL_AXIS, 0.9);
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
			trend_text(c, label_w + (i + 1) * col_w, y, 1, s);
		}
		y += line;
	}

	// How long, how much of it with a signal, and the settings used
	char d[32];
	format_duration(d, sizeof(d), elapsed);
	double coverage = elapsed > 0 ? fmin(100, t->count * TREND_INTERVAL_S * 100.0 / elapsed) : 100;
	cairo_set_font_size(c, font * 0.8);
	set_color(c, COL_AXIS, 0.7);
	/* TRANSLATORS: Trend page. %s is a duration such as 2:15:03, the number is a percentage */
	snprintf(s, sizeof(s), t->full ? _("%s since the start, %.0f %% with signal (100 h limit reached)")
		: _("%s since the start, %.0f %% with signal"), d, coverage);
	trend_text(c, 4, y, 0, s);
	y += line * 0.85;
	/* TRANSLATORS: Trend page: the settings used for the readings. "cal" is the calibration value and stays as is */
	snprintf(s, sizeof(s), _("%d bph, lift angle %.0f\xC2\xB0, cal %+.1f s/d"), t->bph, t->la, t->cal / 10.0);
	trend_text(c, 4, y, 0, s);
	y += line * 1.2;

	// Per position, if positions were set
	int set_positions = 0;
	for(i = POS_NONE + 1; i < POS_COUNT; i++)
		if(t->pos_rate[i].n)
			set_positions++;
	if(set_positions) {
		col_w = (width - label_w - 6) / 4;
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
		set_color(c, COL_AXIS, 0.6);
		/* TRANSLATORS: Trend page column head over the list of watch positions. Short, shown in a narrow column */
		trend_text(c, 4, y, 0, _("Position"));
		for(i = 0; i < 4; i++)
			trend_text(c, label_w + (i + 1) * col_w, y, 1, _(pos_heads[i]));
		y += line;
		cairo_set_font_size(c, font);
		double best = 0, worst = 0;
		int first = 1;
		for(int p = POS_NONE; p < POS_COUNT; p++) {
			if(!t->pos_rate[p].n) continue;
			const char *code = watch_position_code(p);
			set_color(c, COL_AXIS, code ? 0.9 : 0.6);
			/* TRANSLATORS: Trend page: readings taken while no watch position was set. Short, shown in a narrow column */
			trend_text(c, 4, y, 0, code ? code : _("not set"));
			format_duration(d, sizeof(d), t->pos_rate[p].n * TREND_INTERVAL_S);
			trend_text(c, label_w + col_w, y, 1, d);
			double rate = trend_stats_mean(&t->pos_rate[p]);
			snprintf(s, sizeof(s), "%+.1f", rate);
			set_color(c, COL_RATE, 1);
			trend_text(c, label_w + 2 * col_w, y, 1, s);
			if(t->pos_amp[p].n)
				snprintf(s, sizeof(s), "%.0f", trend_stats_mean(&t->pos_amp[p]));
			else
				snprintf(s, sizeof(s), "\xE2\x80\x94");
			set_color(c, COL_AMP, 1);
			trend_text(c, label_w + 3 * col_w, y, 1, s);
			snprintf(s, sizeof(s), "%.1f", trend_stats_mean(&t->pos_be[p]));
			set_color(c, COL_BE, 1);
			trend_text(c, label_w + 4 * col_w, y, 1, s);
			y += line;
			if(code) {
				if(first || rate > best) best = rate;
				if(first || rate < worst) worst = rate;
				first = 0;
			}
		}
		cairo_set_font_size(c, font * 0.8);
		set_color(c, COL_AXIS, 0.7);
		if(set_positions >= 2)
			snprintf(s, sizeof(s), _("Positional error (largest difference in rate): %.1f s/d"), best - worst);
		else
			snprintf(s, sizeof(s), "%s", _("Measure more positions to see the positional error."));
		trend_text(c, 4, y, 0, s);
		y += line * 1.2;
	} else {
		cairo_set_font_size(c, font * 0.8);
		set_color(c, COL_AXIS, 0.6);
		trend_text(c, 4, y, 0, _("Set the watch position to also get statistics per position."));
		y += line * 1.2;
	}

	// Graphs share a time axis: rate, amplitude, then beat error
	const double area_top = y;
	const double area_bottom = height - (font + 6);
	if(area_bottom - area_top < 11 * font)
		return FALSE;	// not enough room
	const double gx = 3.5 * font, gw = width - gx - 8;
	const double graph_height = (area_bottom - area_top) / 3;
	const double split1 = area_top + graph_height;
	const double split2 = area_top + 2 * graph_height;
	const double rate_top = area_top + font * 1.2, rate_bottom = split1 - font * 0.5;
	const double amp_top = split1 + font * 1.2, amp_bottom = split2 - font * 0.5;
	const double be_top = split2 + font * 1.2, be_bottom = area_bottom;

	double t1 = elapsed, t0 = 0;
	if(op->trend_span_s > 0) {
		t1 = fmax(elapsed, op->trend_span_s);
		t0 = t1 - op->trend_span_s;
	} else if(t1 < 60)
		t1 = 60;

	int max_cols = (int)gw * 4 + 16;
	struct trend_column *cols = malloc(max_cols * sizeof(*cols));
	if(!cols)
		return FALSE;

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
	cairo_save(c);
	cairo_rectangle(c, gx, rate_top, gw, be_bottom - rate_top);
	cairo_clip(c);
	set_color(c, COL_TOC, 0.14);
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
	for(double tm = ceil(t0 / step) * step; tm <= t1 + 1e-6; tm += step) {
		double x = round(gx + (tm - t0) / (t1 - t0) * gw) + .5;
		cairo_move_to(c, x, rate_top);
		cairo_line_to(c, x, rate_bottom);
		cairo_move_to(c, x, amp_top);
		cairo_line_to(c, x, amp_bottom);
		cairo_move_to(c, x, be_top);
		cairo_line_to(c, x, be_bottom);
		set_axis_style(c, COL_AXIS, 0);
		cairo_stroke(c);
		set_dotted(c, 0);
		format_duration(d, sizeof(d), tm);
		set_color(c, COL_AXIS, 0.7);
		trend_text(c, x, height - 4, 0.5, d);
	}

	// Rate: fixed range or the smallest round one that holds the readings shown
	int n = trend_columns(t, t0, t1, gx, gw, TREND_SERIES_RATE, cols, max_cols);
	struct trend_plot_ranges ranges;
	trend_get_plot_ranges(t, t0, t1, op->trend_rate_range, &ranges);
	double range = ranges.rate_max;
	cairo_set_line_width(c, 1);
	for(int k = -2; k <= 2; k++) {
		double v = range * k / 2;
		snprintf(s, sizeof(s), k ? "%+g" : "0", v);
		trend_hline(c, gx, gw, rate_bottom - (v + range) / (2 * range) * (rate_bottom - rate_top), k == 0, s);
	}
	double mean = trend_stats_mean(&t->rate);
	trend_draw_average(c, gx, gw, rate_top, rate_bottom, -range, range, &t->rate, COL_RATE);
	trend_draw_series(c, cols, n, rate_top, rate_bottom, -range, range, COL_RATE);
	cairo_set_font_size(c, font * 0.9);
	set_color(c, COL_RATE, 1);
	if(t->rate.n)
		snprintf(s, sizeof(s), _("Rate (s/d), dashed: average %+.1f"), mean);
	else
		snprintf(s, sizeof(s), "%s", _("Rate (s/day)"));
	trend_text(c, gx, rate_top - font * 0.4, 0, s);

	// Expand the amplitude scale for readings below the usual 120 degrees.
	cairo_set_font_size(c, font * 0.8);
	cairo_set_line_width(c, 1);
	int amp_floor = (int)ranges.amp_min;
	for(int v = amp_floor; v <= 360; v += 60) {
		snprintf(s, sizeof(s), "%d", v);
		trend_hline(c, gx, gw, amp_bottom - (v - amp_floor) / (double)(360 - amp_floor) * (amp_bottom - amp_top), 0, s);
	}
	n = trend_columns(t, t0, t1, gx, gw, TREND_SERIES_AMPLITUDE, cols, max_cols);
	double amp_mean = trend_stats_mean(&t->amp);
	trend_draw_average(c, gx, gw, amp_top, amp_bottom, amp_floor, 360, &t->amp, COL_AMP);
	trend_draw_series(c, cols, n, amp_top, amp_bottom, amp_floor, 360, COL_AMP);
	cairo_set_font_size(c, font * 0.9);
	set_color(c, COL_AMP, 1);
	if(t->amp.n)
		snprintf(s, sizeof(s), _("Amplitude (\xC2\xB0), dashed: average %.1f"), amp_mean);
	else
		snprintf(s, sizeof(s), "%s", _("Amplitude (degrees)"));
	trend_text(c, gx, amp_top - font * 0.4, 0, s);

	// Beat error: automatic range from zero to the smallest round value that holds the readings
	n = trend_columns(t, t0, t1, gx, gw, TREND_SERIES_BEAT_ERROR, cols, max_cols);
	double be_range = ranges.be_max;
	cairo_set_font_size(c, font * 0.8);
	cairo_set_line_width(c, 1);
	for(int k = 0; k <= 4; k++) {
		double v = be_range * k / 4;
		snprintf(s, sizeof(s), "%g", v);
		trend_hline(c, gx, gw, be_bottom - v / be_range * (be_bottom - be_top), k == 0, s);
	}
	double be_mean = trend_stats_mean(&t->be);
	trend_draw_average(c, gx, gw, be_top, be_bottom, 0, be_range, &t->be, COL_BE);
	trend_draw_series(c, cols, n, be_top, be_bottom, 0, be_range, COL_BE);
	cairo_set_font_size(c, font * 0.9);
	set_color(c, COL_BE, 1);
	if(t->be.n)
		snprintf(s, sizeof(s), _("Beat error (ms), dashed: average %.1f"), be_mean);
	else
		snprintf(s, sizeof(s), "%s", _("Beat error (ms)"));
	trend_text(c, gx, be_top - font * 0.4, 0, s);
	free(cols);

	// Position changes: a line through all three graphs, with the new position
	cairo_set_font_size(c, font * 0.8);
	int prev_pos = -1;
	for(i = 0; i < t->count; i++) {
		const struct trend_sample *x = &t->samples[i];
		if(x->t < t0 || x->t > t1) continue;
		if(x->position != prev_pos) {
			double px = round(gx + (x->t - t0) / (t1 - t0) * gw) + .5;
			if(prev_pos >= 0) {
				cairo_move_to(c, px, rate_top);
				cairo_line_to(c, px, be_bottom);
				set_color(c, COL_TOC, 0.8);
				cairo_stroke(c);
			}
			const char *code = watch_position_code(x->position);
			if(code || prev_pos >= 0) {
				set_color(c, COL_TOC, 1);
				trend_text(c, px + 3, rate_top + font, 0, code ? code : "\xE2\x80\x94");
			}
			prev_pos = x->position;
		}
	}
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
	op->trend_span_s = 0;
	op->trend_span_combo = NULL;
	op->correlation_span_combo = NULL;
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
	/* TRANSLATORS: Trend page: label of the time span selector (All, Last 1 h, ...). Short */
	gtk_box_pack_start(GTK_BOX(trend_box), gtk_label_new(_("Show")), FALSE, FALSE, 0);
	op->trend_span_combo = op_create_trend_span_combo(op);
	gtk_box_pack_start(GTK_BOX(trend_box), op->trend_span_combo, FALSE, FALSE, 0);

	if(comp) {
		gtk_box_pack_start(GTK_BOX(trend_box), gtk_label_new(_("Settling time (s)")),
			FALSE, FALSE, 0);
		op->settling_spin = gtk_spin_button_new_with_range(0, TREND_MAX_SETTLING_S, 1);
		gtk_widget_set_tooltip_text(op->settling_spin,
			_("Seconds to wait after a position change; 0 disables settling."));
		gtk_box_pack_start(GTK_BOX(trend_box), op->settling_spin, FALSE, FALSE, 0);
		g_signal_connect(op->settling_spin, "value_changed", G_CALLBACK(handle_settling_setting), op);
		op->settling_status = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(trend_box), op->settling_status, FALSE, FALSE, 0);
	}
	GtkWidget *correlations_page = correlations_create_page(op);
	gtk_stack_add_titled(GTK_STACK(op->right_stack), correlations_page,
		"correlations", _("Correlations"));
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
