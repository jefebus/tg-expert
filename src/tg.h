/*
    tg
    Copyright (C) 2015 Marcello Mamino
    Copyright (C) 2026 jefebus (tg-expert contributions)

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

#ifndef TG_EXPERT_TG_H_INCLUDED
#define TG_EXPERT_TG_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <complex.h>
#include <fftw3.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>	// _() translates a string, N_() marks one for later translation
#include <locale.h>
#include <pthread.h>

#ifdef __CYGWIN__
#define _WIN32
#endif

#define CONFIG_FILE_NAME "tg-expert.ini"
/* The icon files keep the name of the original program (icons/.../tg-timer.png) */
#define ICON_NAME "tg-timer"
/* The user manual (PDF), in share/doc/tg-expert on every platform (DOCDIR is
 * set by the build to $(docdir)). If it is not installed, the releases page
 * is opened instead. */
#ifndef DOCDIR
#define DOCDIR ""
#endif
/* Translations: PACKAGE.mo in LOCALEDIR/<language>/LC_MESSAGES (set by the
 * build to $(localedir)), or in share/locale of the installation */
#ifndef LOCALEDIR
#define LOCALEDIR ""
#endif
#define MANUAL_FILE_NAME "tg-expert-manual.pdf"
#define MANUAL_URL "https://github.com/jefebus/tg-expert/releases/latest"

#define FIRST_STEP 1
#define FIRST_STEP_LIGHT 0

#define NSTEPS 4
#define PA_SAMPLE_RATE 44100u
#define PA_BUFF_SIZE (PA_SAMPLE_RATE << (NSTEPS + FIRST_STEP))
/* The longest analysis step holds 2^(NSTEPS-1+FIRST_STEP) seconds of audio,
 * which must fit in the PA_BUFF_SIZE ring buffer: 88200 Hz at most. */
#define MAX_SAMPLE_RATE (PA_BUFF_SIZE >> (NSTEPS - 1 + FIRST_STEP))
#define SAMPLE_RATES { 22050, 32000, 44100, 48000, 0 }

#define OUTPUT_FONT 40
#define OUTPUT_WINDOW_HEIGHT 70

#define POSITIVE_SPAN 10
#define NEGATIVE_SPAN 25

#define EVENTS_COUNT 10000
#define EVENTS_MAX 100
#define PAPERSTRIP_WINDOW_MS 30.0	// paperstrip width at 1x zoom (nearest whole division of the beat)
#define PAPERSTRIP_WINDOW_MS_CAL 10.0	// the same in calibration (of a second)
#define PAPERSTRIP_MARGIN .2
#define PAPERSTRIP_DOT_SIZE 2 // width and height of a beat dot in pixels
#define PAPERSTRIP_DIVISIONS 10 // vertical grid, shared by the ms and amplitude scales (25 deg each)

#define MIN_BPH 8100
#define TYP_BPH 12000
#define MAX_BPH 72000
#define DEFAULT_BPH 21600
#define MIN_LA 10 // deg
#define MAX_LA 90 // deg
#define DEFAULT_LA 52 // deg
#define MIN_CAL -1000 // 0.1 s/d
#define MAX_CAL 1000 // 0.1 s/d
#define AUDIO_DEVICE_DEFAULT -1
#define AUDIO_API_DEFAULT -1	// use PortAudio's default host API

#define PRESET_BPH { 12000, 14400, 17280, 18000, 19800, 21600, 25200, 28800, 36000, 43200, 72000, 0 };

#ifdef DEBUG
#define debug(...) print_debug(__VA_ARGS__)
#else
#define debug(...) {}
#endif

#define UNUSED(X) (void)(X)

/* algo.c */
enum algorithm_parameter {
	ALG_FILTER, ALG_NOISE_WINDOW, ALG_NOISE_SEGMENT, ALG_NOISE_THRESHOLD,
	ALG_TAPER, ALG_LOBES, ALG_GUESS_SEARCH, ALG_HARMONIC, ALG_DOUBLE_TRIGGER,
	ALG_PERIOD_REFINE, ALG_BE_SEARCH, ALG_TIC_SMOOTH, ALG_EVENT_SEARCH,
	ALG_TRIM, ALG_AMP_SMOOTH, ALG_AMP_INITIAL, ALG_AMP_NOISE, ALG_AMP_LIMIT,
	ALG_AMP_STEP, ALG_AMP_MIN, ALG_AMP_DIFFERENCE, ALG_PERIOD_LIMIT,
	ALG_PERIOD_SIGMA, ALG_CAL_COUNT, ALG_CAL_EDGE, ALG_CAL_INTERVAL,
	ALG_CAL_PRECISION, ALG_CAL_LOCK_LOW, ALG_CAL_LOCK_HIGH, ALG_COUNT
};
extern double algorithm_parameters[ALG_COUNT];

enum detection_status {
	DETECTION_STATUS_OK,
	DETECTION_STATUS_NO_WAVEFORM,
	DETECTION_STATUS_PERIOD_TOO_LONG,
	DETECTION_STATUS_NO_BEAT,
	DETECTION_STATUS_UNSTABLE,
	DETECTION_STATUS_RATE_ONLY,
	DETECTION_STATUS_AMPLITUDE_NOT_FOUND,
	DETECTION_STATUS_AMPLITUDE_TOO_LOW,
	DETECTION_STATUS_AMPLITUDE_TOO_HIGH,
	DETECTION_STATUS_AMPLITUDE_INCONSISTENT
};

struct processing_buffers {
	int sample_rate;
	int sample_count;
	float *samples, *samples_sc, *waveform, *waveform_sc, *tic_wf, *slice_wf, *tic_c;
	fftwf_complex *fft, *sc_fft, *tic_fft, *slice_fft;
	fftwf_plan plan_a, plan_b, plan_c, plan_d, plan_e, plan_f, plan_g;
	struct filter *hpf, *lpf;
	double period,sigma,be,waveform_max,phase,tic_pulse,toc_pulse,amp;
	double candidate_period;
	double cal_phase;
	int waveform_max_i;
	int tic,toc;
	int ready;
	int period_ready;
	enum detection_status detection_status;
	uint64_t timestamp, last_tic, last_toc, events_from;
	uint64_t *events;
	unsigned char *events_tictoc;
	float amp_history;
#ifdef DEBUG
	int debug_size;
	float *debug;
#endif
};

struct calibration_data {
	int wp;
	int size;
	int state;
	double calibration;
	uint64_t start_time;
	double *times;
	double *phases;
	uint64_t *events;
};

struct calibration_estimate {
	double calibration;
	double uncertainty;
	double duration;
	int observations;
};

void setup_buffers(struct processing_buffers *b);
void pb_destroy(struct processing_buffers *b);
struct processing_buffers *pb_clone(struct processing_buffers *p);
void pb_destroy_clone(struct processing_buffers *p);
void process(struct processing_buffers *p, int bph, double la, int light);
void setup_cal_data(struct calibration_data *cd);
void cal_data_destroy(struct calibration_data *cd);
int estimate_calibration(const struct calibration_data *cd, struct calibration_estimate *estimate);
int test_cal(struct processing_buffers *p);
int process_cal(struct processing_buffers *p, struct calibration_data *cd);

/* audio.c */
struct processing_data {
	struct processing_buffers *buffers;
	uint64_t last_tic;
	int is_light;
};

int start_portaudio(int *nominal_sample_rate, double *real_sample_rate);
int terminate_portaudio();
void set_audio_errors_quiet(int quiet);
int list_audio_input_devices(int **devices, char ***names, int *count, int *default_device);
void free_audio_input_devices(int *devices, char **names, int count);
int list_audio_apis(int **types, char ***names, int *count, int *current_type);
void free_audio_apis(int *types, char **names, int count);
void set_audio_api(int type);
int get_audio_api(void);
const char *get_audio_api_name(void);
int get_default_audio_input_device(void);
int list_audio_output_devices(int **devices, char ***names, int *count, int *default_device);
void free_audio_output_devices(int *devices, char **names, int count);
void set_audio_output_device_name(const char *name);
const char *get_audio_output_device_name(void);
int get_audio_output_device(void);
void set_audio_output_device(int device);
int start_audio_monitor(int device, double volume, char **error_message);
void pause_audio_monitor(void);
void stop_audio_monitor(void);
int audio_monitor_status(void);
void set_audio_monitor_volume(double volume);
void set_audio_input_device_name(const char *name);
const char *get_audio_input_device_name(void);
int list_supported_sample_rates(int device, int *rates, int max);
int sample_rate_valid(int rate);
void set_audio_input_device(int device);
int get_audio_input_device(void);
void set_audio_sample_rate(int sample_rate);
int get_audio_sample_rate(void);
float get_audio_level(void);
uint64_t get_timestamp(int light);
int analyze_pa_data(struct processing_data *pd, int bph, double la, uint64_t events_from);
int analyze_pa_data_cal(struct processing_data *pd, struct calibration_data *cd);
void set_audio_light(bool light);

enum audio_input_status {
	AUDIO_STATUS_UNAVAILABLE = -1,
	AUDIO_STATUS_NO_INPUT,
	AUDIO_STATUS_BPH_UNKNOWN,
	AUDIO_STATUS_BPH_UNCONFIRMED,
	AUDIO_STATUS_OK
};

/* computer.c */
struct snapshot {
	struct processing_buffers *pb;
	int is_old;
	int rate_only;
	uint64_t timestamp;
	int is_light;

	int nominal_sr;
	int calibrate;
	int bph;
	double la; // deg
	int cal; // 0.1 s/d

	int events_count;
	uint64_t *events; // used in cal+timegrapher mode
	unsigned char *events_tictoc;	//< Tic or Toc for each event
	int events_wp; // used in cal+timegrapher mode
	uint64_t events_from; // used only in timegrapher mode

	/** Allocated arrays of amplitude measurements and timestamps */
	float *amps;		//< Circ buffer with history of amplitude measurements
	uint64_t *amps_time;	//< Timestamps for amps
	int amps_wp;		//< Index of recent amp in amps
	int amps_count;		//< Number of amplitude samples

	int signal;
	int audio_status;
	enum detection_status detection_status;

	int cal_state;
	int cal_percent;
	int cal_result; // 0.1 s/d
	int cal_estimate_available;
	int cal_estimate_result; // 0.1 s/d
	int cal_estimate_observations;
	double cal_estimate_duration;

	// data dependent on bph, la, cal
	double sample_rate;
	int guessed_bph;
	int candidate_bph;
	double rate;
	double be;
	double amp;

	double trace_centering;
	double trace_zoom;

	int position; // enum watch_position, set by the user on snapshot tabs

	/** Free text notes of a saved snapshot (UTF-8), NULL if none. Owned: deep-copied
	 *  by snapshot_clone() and freed by snapshot_destroy(). Never set on live data. */
	char *notes;

	/** 1 for a run (tramo): a period of continuous recording, from Record to
	 *  Pause, with its Trend and the real-time tab as it was at its end (or
	 *  when the session was saved). 0 for snapshots taken by the user, which
	 *  do not stop the recording. Stored as the optional "role" field ("run";
	 *  files of 0.11.x before runs wrote "saved-state", read as a run). Older
	 *  readers treat everything as a snapshot. */
	int saved_state;
	/** The run: a run's own number, or for a snapshot the run it was taken
	 *  in; 0 = unknown (older files). Optional field "run". */
	int run;
	/** For a run: when it started, seconds since 1970 UTC, 0 = unknown.
	 *  Optional field "run_start". It ended at wall_time. */
	int64_t run_start;
	/** For a snapshot: its number within its run (the M of "N.M"), given
	 *  when it is taken and kept, 0 = none. Optional field "snap". */
	int snap;

	/** When the snapshot was taken (for a run, when it ended or was saved): seconds since
	 *  1970-01-01 UTC, 0 = unknown (older files, live data). timestamp above
	 *  is the audio sample counter, which does not give a date. Stored as the
	 *  optional "wall_time" field. */
	int64_t wall_time;
};

/* Watch position during a measurement (stored in snapshots and .tgj files) */
enum watch_position {
	POS_NONE = 0,
	POS_DU,	// dial up
	POS_DD,	// dial down
	POS_CU,	// crown up
	POS_CD,	// crown down
	POS_CL,	// crown left
	POS_CR,	// crown right
	POS_COUNT
};

struct computer {
	pthread_t thread;
	int thread_joined;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

// controlled by interface
	int recompute;
	int calibrate;
	int bph;
	double la; // deg
	int clear_trace;
	void (*callback)(void *);
	void *callback_data;

	struct processing_data *pdata;
	struct calibration_data *cdata;

	struct snapshot *actv;
	struct snapshot *curr;
};

struct snapshot *snapshot_clone(struct snapshot *s);
void snapshot_destroy(struct snapshot *s);
void computer_join(struct computer *c);
void computer_destroy(struct computer *c);
struct computer *start_computer(int nominal_sr, int bph, double la, int cal, int light);
void lock_computer(struct computer *c);
void unlock_computer(struct computer *c);
void compute_results(struct snapshot *s);

/* trend.c: long-term statistics of the live measurement */
#define TREND_INTERVAL_S 2	// one reading every 2 s
#define TREND_MAX_HOURS 100
#define TREND_MAX_SETTLING_S 3600
#define TREND_GUESS_CHANGE_S 60	// a guessed bph must differ this long to restart the trend
#define TREND_MAX_SAMPLES (TREND_MAX_HOURS * 3600 / TREND_INTERVAL_S)

struct trend_sample {
	float t;	// s since the start of the recording
	float rate;	// s/d
	float amp;	// deg, 0 = not available
	float be;	// ms
	signed char position;	// enum watch_position
	unsigned char settling;
};

struct trend_stats {
	int n;
	double sum, sumsq, min, max;
};

struct trend_plot_ranges {
	double rate_min, rate_max;
	double amp_min, amp_max;
	double be_min, be_max;
};

struct trend {
	struct trend_sample *samples;
	int count, capacity;
	int full;	// TREND_MAX_SAMPLES reached: recording stopped
	int frozen;	// copy kept by a snapshot tab
	gint64 start_us, last_us, end_us;	// monotonic time; end_us only if frozen
	gint64 settling_until_us;
	// settings the readings were taken with
	int bph, cal, nominal_sr, is_light, algorithm_custom;
	double la;
	gint64 guess_mismatch_us;	// since when the guessed bph differs from bph, 0 if it does not
	struct trend_stats rate, amp, be;
	struct trend_stats pos_rate[POS_COUNT], pos_amp[POS_COUNT], pos_be[POS_COUNT];
};

struct trend *trend_new(void);
void trend_free(struct trend *t);
struct trend *trend_clone(const struct trend *t);
void trend_reset(struct trend *t);
void trend_start_settling(struct trend *t, int seconds);
void trend_update(struct trend *t, const struct snapshot *s, int position);
int trend_set_samples(struct trend *t, const float *tm, const float *rate, const float *amp,
	const float *be, const int *position, const int *settling, int count, double elapsed);
double trend_elapsed(const struct trend *t);
double trend_stats_mean(const struct trend_stats *s);
double trend_stats_sd(const struct trend_stats *s);
void trend_get_plot_ranges(const struct trend *t, double t0, double t1,
	double fixed_rate, double fixed_be, struct trend_plot_ranges *ranges);

/* render.c: drawing themes shared by the screen and the PDF report (TG-31) */
enum render_theme_id {
	RENDER_SCREEN,		// colours on black, as on screen
	RENDER_DOC_COLOR,	// on white, for documents
	RENDER_DOC_MONO,	// black and greys; positions differ by symbol
	RENDER_THEME_COUNT
};

struct render_rgb { double r, g, b; };

struct render_theme {
	enum render_theme_id id;
	int monochrome;
	struct render_rgb background;
	struct render_rgb text;		// titles
	struct render_rgb label;	// scale numbers
	struct render_rgb message;	// "no data" messages
	struct render_rgb grid;
	double grid_alpha, grid_minor_alpha;
	double line_scale;		// multiplies line widths
	double symbol_scale;		// multiplies the size of the position symbols
	struct render_rgb position[POS_COUNT];	// symbol colours of the positions
	/* Series and scales of the Trend graphs, waveforms and paper strip */
	struct render_rgb axis;		// grids, scales and secondary text, with an alpha
	double axis_major_alpha, axis_minor_alpha;
	struct render_rgb rate, amp, be;
	struct render_rgb tic, toc, wave;
};

const struct render_theme *render_theme_get(enum render_theme_id id);
void render_set_rgb(cairo_t *cr, struct render_rgb c, double alpha);
void render_position_symbol(cairo_t *cr, const struct render_theme *th, int position,
	double x, double y, double radius);
void render_message(cairo_t *cr, const struct render_theme *th, double width, double height,
	const char *message);
void render_text(cairo_t *cr, double x, double y, double align, const char *text);
double render_position_legend(cairo_t *cr, const struct render_theme *th, double x, double y,
	double font, unsigned mask);

/* correlations.c: scatter plots of the Trend readings */
enum correlation_kind {
	CORRELATION_AMP_RATE,	// x amplitude, y rate
	CORRELATION_BE_RATE,	// x beat error, y rate
	CORRELATION_BE_AMP,	// x beat error, y amplitude
	CORRELATION_COUNT
};

struct correlation_options {
	enum correlation_kind kind;
	int position;		// enum watch_position, -1 = all positions
	double t0, t1;		// time range, s since the start of the recording
	int rate_range;		// s/d either side of 0, 0 = automatic
	int be_range;		// maximum beat error in ms, 0 = automatic
	double font;		// size of the scale numbers (10 on screen)
};

/* Time range shown for a Trend span (0 = everything) */
void correlation_time_range(const struct trend *t, int span_s, double *t0, double *t1);
/* Draw one scatter plot in a width x height area at the origin of cr */
void render_correlation(cairo_t *cr, double width, double height, const struct trend *t,
	const struct correlation_options *o, const struct render_theme *th);

/* trend_render.c: the Trend page (statistics and the rate, amplitude and
 * beat error graphs), drawn from the data alone */
#define TREND_RENDER_STATS	1	// tables: since the start, per position
#define TREND_RENDER_GRAPHS	2	// the three graphs on a shared time axis

struct trend_render_options {
	int sections;		// TREND_RENDER_* flags
	int span_s;		// time shown, 0 = everything since the start
	int rate_range;		// s/d either side of 0, 0 = automatic
	int be_range;		// maximum beat error in ms, 0 = automatic
	double font;		// text size (on screen from the window width, at least 12)
	int calibrating;	// for the message when there are no readings
	int paused;		// the live recording is paused (message when empty)
};

void render_trend(cairo_t *c, double width, double height, const struct trend *t,
	const struct trend_render_options *o, const struct render_theme *th);
double render_trend_stats_height(const struct trend *t, double font);

/* waveform_render.c: the averaged waveforms of a snapshot */
enum waveform_kind {
	WAVEFORM_TIC,		// first beat, with the amplitude scale and lift pulse
	WAVEFORM_TOC,		// second beat
	WAVEFORM_PERIOD		// the full period, tic and toc windows tinted
};

struct waveform_options {
	enum waveform_kind kind;
	int negative_span;	// ms before the beat, from waveform_negative_span()
	double font;		// text size (on screen from the window width, at least 12)
};

void render_waveform(cairo_t *c, double width, double height, const struct snapshot *s,
	const struct waveform_options *o, const struct render_theme *th);
double render_waveform_legend(cairo_t *c, const struct render_theme *th, double x, double y,
	double font);

/* paperstrip_render.c: the paper strip of a snapshot */
struct paperstrip_options {
	double tick_seconds;	// time between the marks on the vertical axis
	double window_seconds;	// time span shown vertically
	double guide_rate;	// s/d of the rate guide lines, NAN = none (the report draws none;
				// unused when calibrating, where the guide follows cal)
	uint64_t now;		// timestamp (samples) of the newest row: the snapshot's own when saved
	double font;		// text size (on screen from the window width, at least 12)
	int legend;		// draw the legend under the strip
};

extern const double paperstrip_zoom_levels[];	// ends with 0
int paperstrip_zoom_index(double trace_zoom);
double paperstrip_zoom_factor(const struct snapshot *snst);
void render_paperstrip(cairo_t *c, double width, double height, const struct snapshot *snst,
	const struct paperstrip_options *o, const struct render_theme *th);
double render_paperstrip_legend(cairo_t *c, const struct render_theme *th, double x, double y,
	double font, int calibrate, int guide);
/* output_panel.c: ms shown before a beat, from the current algorithm settings */
int waveform_negative_span(const struct snapshot *s);

/* output_panel.c */
struct output_panel {
	GtkWidget *panel;

	GtkWidget *output_drawing_area;
	GtkWidget *tic_drawing_area;
	GtkWidget *toc_drawing_area;
	GtkWidget *period_drawing_area;
	GtkWidget *paperstrip_drawing_area;
	GtkWidget *clear_button;
	GtkWidget *paperstrip_tick_spin;
	GtkWidget *paperstrip_window_spin;
	GtkWidget *right_stack; // "panels" (tic/toc/period) or "calibration" (instructions)
	GtkWidget *calibration_estimate_label;
	GtkWidget *output_frame;     // titled frames whose title depends on calibration
	GtkWidget *paperstrip_frame;
	double paperstrip_tick_seconds;
	double paperstrip_window_seconds;
	int updating_paperstrip_settings;
	void (*paperstrip_settings_changed)(void *, double, double);
	void *paperstrip_settings_data;
	double guide_rate;           // filtered rate (s/d) for the paper strip rate guide
	gint64 guide_time;           // monotonic time of the last filter update, 0 = restart
	int trace_centered;          // the live trace was centred since it appeared
	GtkWidget *right_switcher;   // chooses "panels" (waveforms) or "trend" in right_stack
	GtkWidget *trend_drawing_area;
	GtkWidget *trend_span_combo;
	GtkWidget *correlation_span_combo;
	GtkWidget *trend_be_combo;
	GtkWidget *correlation_be_combo;
	GtkWidget *correlation_areas[3];
	int correlation_position;   // -1 = all positions
	GtkWidget *settling_spin;
	GtkWidget *settling_status;
	GtkWidget *detection_status_label;
	enum detection_status shown_detection_status;
	gint64 detection_status_until;
	void (*settling_setting_changed)(void *, int);
	void *settling_setting_data;
	struct trend *trend;         // owned; NULL for snapshots (only runs have one)
	GtkWidget *trend_page;       // the Trend and Correlations pages of right_stack,
	GtkWidget *correlations_page;	// hidden on tabs without a trend
	int paused;                  // live panel: not recording (the Trend page says so)
	int trend_rate_range;        // s/d shown either side of 0, 0 = automatic
	int trend_be_range;          // maximum beat error in ms, 0 = automatic
	int trend_span_s;            // time shown, 0 = everything since the start
	float audio_meter_level;
	gint64 audio_meter_time;
	int audio_meter_active;
	int audio_meter_transition_count;
	int audio_status;
#ifdef DEBUG
	GtkWidget *debug_drawing_area;
#endif
	struct computer *computer;
	struct snapshot *snst;
};

void initialize_palette();
struct output_panel *init_output_panel(struct computer *comp, struct snapshot *snst, int border);
void op_set_paperstrip_settings(struct output_panel *op, double tick_seconds, double window_seconds);
void op_set_paperstrip_settings_callback(struct output_panel *op,
	void (*callback)(void *, double, double), void *data);
void op_set_settling_setting(struct output_panel *op, int seconds,
	void (*callback)(void *, int), void *data);
void op_update_settling_status(struct output_panel *op);
void redraw_op(struct output_panel *op);
void op_set_snapshot(struct output_panel *op, struct snapshot *snst);
void op_set_border(struct output_panel *op, int i);
void op_destroy(struct output_panel *op);
/* Show the Trend and Correlations pages only where there is a trend: the
 * live panel and runs; snapshots have none. Call after setting op->trend. */
void op_update_trend_pages(struct output_panel *op);

/* Project metadata stored in .tgj files (the "project-metadata" block inside "data").
 * All strings are UTF-8, owned by the structure, and NULL or "" mean "not set".
 * The watch label is a copy: it never refers to an entry of the INI file.
 * Functions in serializer.c. */
#define PROJECT_FIELD_MAX 256		// bytes, title, reference, label, owner and date
#define PROJECT_NOTES_MAX 65536		// bytes, general notes and snapshot notes
#define PROJECT_SCHEMA_VERSION 1

struct project_metadata {
	char *title;
	char *project_reference;
	char *watch_label;
	char *owner;
	char *last_service_date;	// "", "YYYY", "YYYY-MM" or "YYYY-MM-DD"
	char *notes;
};

/* interface.c */
enum csv_format {
	CSV_FORMAT_PORTABLE,
	CSV_FORMAT_LOCALIZED
};

struct main_window {
	GtkApplication *app;

	GtkWidget *window;
	GtkWidget *bar_flow;	// the bars, side by side or in rows when narrow
	GtkWidget *watch_combo_box;
	GtkWidget *watch_save_button;
	GtkWidget *watch_delete_button;
	int watch_updating; // set while the UI is being changed programmatically
	GtkWidget *bph_combo_box;
	GtkWidget *audio_button;	// shows the input in use, opens the audio settings
	GtkWidget *audio_button_label;
	GtkWidget *listen_button;
	int listen_active;
	int listen_updating;
	int listen_warning_shown;
	struct audio_settings *audio_settings;	// the audio settings dialog, NULL when closed
	GtkWidget *la_spin_button;
	GtkWidget *cal_spin_button;
	GtkWidget *snapshot_button;
	GtkWidget *snapshot_name;
	GtkWidget *snapshot_name_entry;
	GtkWidget *snapshot_position_combo;
	int position_updating; // set while the position combo is changed programmatically
	int live_position;     // position for the next snapshot (real-time tab)
	GtkWidget *cal_button; // "Start calibration" / "Stop calibration" menu item
	GtkWidget *notebook;
	GtkWidget *new_session_item;
	GtkWidget *open_item;
	GtkWidget *algorithm_item;
	GtkWidget *save_item;
	GtkWidget *save_as_item;
	GtkWidget *export_item;
	GtkWidget *report_item;	// File > Export report (PDF)
	GtkWidget *session_bar;	// notice about the watch of an opened session
	GtkWidget *session_bar_label;
	GtkWidget *session_bar_use;	// "Use <watch> to continue measuring"
	struct output_panel *active_panel;

	/* The session: one document with the project metadata, the run and
	 * snapshot tabs and, when saved, the run in progress (TG-34).
	 * Loaded measurements and closed runs are kept as they are (tab data
	 * "from-file"). */
	struct project_metadata project;
	gchar *session_path;	// UTF-8, NULL until saved or opened
	int session_dirty;	// changed since the last save, open or new session
	int session_updating;	// tabs are being replaced: no change tracking
	int saved_trend_count;	// live trend when last saved, to see new readings
	gint64 saved_trend_start_us;
	int title_dirty_shown;	// -1 = title not set yet
	struct notes_panel *notes;	// project and notes panel (TG-29), below the tabs

	/* Runs (TG-34): the session is recorded in runs, from Record to Pause.
	 * While paused the live display still analyses the audio, but nothing
	 * is recorded and no snapshot can be taken. A closed run becomes a tab;
	 * the current one is written as a run entry when the session is saved. */
	GtkWidget *record_button;	// "Record" / "Pause"
	GtkWidget *live_tab_label;	// title of the real-time tab, shows the state
	int recording;
	int run_number;		// the current run, while recording
	int64_t run_start;	// when it started, seconds since 1970 UTC
	struct snapshot *last_live;	// last usable live state of the current run, or NULL
	gint64 last_live_us;	// monotonic time it was copied
	char *run_notes;	// notes of the run being recorded (UTF-8, owned), or NULL
	int run_last_snap;	// the highest snapshot number given in the run being recorded

	struct computer *computer;
	struct snapshot *active_snapshot;
	int computer_timeout;

	int is_light;
	int zombie;
	int controls_active;
	int calibrate;
	int calibration_stop_pending;
	int bph;
	int audio_device;
	int audio_api;	// PaHostApiTypeId, or AUDIO_API_DEFAULT
	gchar *audio_output_device_name;
	double listen_volume;
	double la; // deg
	int cal; // 0.1 s/d
	double paperstrip_tick_seconds;
	double paperstrip_window_seconds;
	int settling_seconds;
	int csv_format;
	int nominal_sr;
	int restart_audio;
	int restart_algorithm;
	int pending_audio_device;
	int pending_audio_rate;
	int pending_audio_api;
	gchar *audio_device_name;	// input device to reopen at startup (saved by name)
	int audio_ok;	// an input stream is open
	const char *audio_message;	// result of the last audio change, or NULL

	GKeyFile *config_file;
	gchar *config_file_name;
	struct conf_data *conf_data;

	guint kick_timeout;
	guint save_timeout;
};

void algorithm_settings_load(struct main_window *w);
void algorithm_settings_open(struct main_window *w);
int algorithm_settings_custom(int light);
void algorithm_settings_prune_defaults(GKeyFile *config);
void request_algorithm_restart(struct main_window *w, int light);

extern int preset_bph[];

#ifdef DEBUG
extern int testing;
#endif

void print_debug(char *format,...);
void error(char *format,...);
void request_audio_restart(struct main_window *w, int api, int device, int rate);
const char *watch_position_code(int pos);
const char *watch_position_label(int pos);
GtkWidget *correlations_create_page(struct output_panel *op);
void correlations_queue_draw(struct output_panel *op);
GtkWidget *op_create_trend_span_combo(struct output_panel *op);
GtkWidget *op_create_trend_be_combo(struct output_panel *op);
void format_duration(char *s, size_t n, double seconds);

/* audio_settings.c */
void audio_settings_open(struct main_window *w);
void audio_settings_update(struct main_window *w);
int audio_settings_is_open(struct main_window *w);

/* config.c */
#define CONFIG_FIELDS(OP) \
	OP(bph, bph, int) \
	OP(lift_angle, la, double) \
	OP(calibration, cal, int) \
	OP(light_algorithm, is_light, int) \
	OP(audio_device, audio_device, int) \
	OP(audio_api, audio_api, int) \
	OP(listen_volume, listen_volume, double) \
	OP(csv_format, csv_format, int) \
	OP(settling_seconds, settling_seconds, int) \
	OP(audio_rate, nominal_sr, int) \
	OP(paperstrip_tick_seconds, paperstrip_tick_seconds, double) \
	OP(paperstrip_window_seconds, paperstrip_window_seconds, double)

#define AUDIO_DEVICE_NAME_KEY "audio_device_name"
#define AUDIO_OUTPUT_DEVICE_NAME_KEY "audio_output_device_name"

struct conf_data {
#define DEF(NAME,PLACE,TYPE) TYPE PLACE;
	CONFIG_FIELDS(DEF)
};

void load_config(struct main_window *w);
void save_config(struct main_window *w);
gboolean save_config_checked(struct main_window *w, GError **error);
void save_on_change(struct main_window *w);
void close_config(struct main_window *w);

#define WATCH_NAME_MAX 64 // characters
int watch_name_valid(const char *name);
int watch_exists(struct main_window *w, const char *name);
int get_watch(struct main_window *w, const char *name, int *bph, double *la);
gchar **list_watches(struct main_window *w); // free with g_strfreev
void set_watch(struct main_window *w, const char *name, int bph, double la);
void delete_watch(struct main_window *w, const char *name);

/* serializer.c (struct project_metadata is defined before struct main_window) */

void project_metadata_init(struct project_metadata *m);
void project_metadata_free(struct project_metadata *m);	// leaves it empty
int project_metadata_copy(struct project_metadata *dst, const struct project_metadata *src);
int project_metadata_is_empty(const struct project_metadata *m);
int project_date_valid(const char *date);	// NULL and "" are valid (not set)
int project_text_valid(const char *text, size_t max_bytes);	// UTF-8 and length

/* trends[i] is the long-term trend of snapshot i, NULL if it has none.
 * meta may be NULL: nothing is written (write_file) or read (read_file) of the project.
 * read_file fills *meta with the stored metadata, or leaves it empty if the file has none.
 * Fails if a stored value is invalid (too long, not UTF-8, duplicated, bad date). */
int write_file(FILE *f, struct snapshot **s, char **names, struct trend **trends, uint64_t cnt,
	const struct project_metadata *meta);
int write_trend_csv(FILE *f, const struct trend *t, char separator, char decimal);
int read_file(FILE *f, struct snapshot ***s, char ***names, struct trend ***trends, uint64_t *cnt,
	struct project_metadata *meta);

/* interface.c: the session changed (marks it modified, unless it is being replaced) */
void session_mark_changed(struct main_window *w);

/* notes_panel.c */
struct notes_panel;
struct notes_panel *notes_panel_new(struct main_window *w);
GtkWidget *notes_panel_widget(struct notes_panel *np);
void notes_panel_free(struct notes_panel *np);
void notes_panel_show_project(struct notes_panel *np);	// all project fields, from w->project
void notes_panel_show_watch(struct notes_panel *np);
/* The Measurement page edits *target (the notes of a run or snapshot), shown
 * as "Notes of: <title>"; target NULL shows the message none instead (NULL:
 * the default hint) */
void notes_panel_show_measurement(struct notes_panel *np, char **target, const char *title,
	const char *none);
void notes_panel_forget(struct notes_panel *np, char **target);	// the notes are being freed
void notes_panel_attach_toggle(struct notes_panel *np, GtkWidget *check_item);
int notes_panel_remembered_visible(struct main_window *w);
void notes_panel_set_visible(struct notes_panel *np, int visible);

/* session.c */
char *session_sanitize_filename_part(const char *text);
char *session_suggest_filename(const char *date, const char *name);
char *session_suggest_filename_ext(const char *date, const char *name, const char *ext);

/* interface.c: the run being recorded as a run entry (a copy with its notes),
 * NULL if not recording or there is nothing to keep */
struct snapshot *session_current_run(struct main_window *w);
/* interface.c: the title of a run or snapshot as its tab shows it ("Run 2",
 * "2.3 name (DU)"); name is the one given by the user, or NULL. g_free(). */
gchar *measurement_title(const struct snapshot *s, const char *name);

/* interface.c: the program icon (ICON_NAME) at size x size, NULL if it cannot
 * be found. Free with g_object_unref(). */
GdkPixbuf *program_icon(int size);

/* interface.c: a save dialog returning the chosen path (UTF-8, with the
 * extension), after asking before replacing a file; NULL if cancelled */
char *choose_path_for_save(struct main_window *w, const char *title,
	const char *suggestion, const char *extension);

/* report.c: the PDF report (TG-16). The data is a copy taken when the export
 * starts, so that the report is coherent while the measurement goes on; the
 * report is built from it alone, without GTK widgets. */
struct report_measurement {
	struct snapshot *snapshot;	// owned copy, NULL = not in the report
	char *title;			// as the tab shows it (name and position)
	int negative_span;		// ms before a beat in the waveforms
};

/* One run in the report: its Trends, optionally its final state, and the
 * snapshots chosen among those taken in it */
struct report_run {
	int number;
	char *name;			// given by the user (owned), or NULL
	int in_progress;		// the run being recorded when exporting
	int64_t start, end;		// seconds since 1970 UTC, 0 = unknown
	struct trend *trend;		// owned copy, NULL if it has none
	int show_trends;		// statistics, graphs and correlations
	struct report_measurement state;	// its final state (owned); its notes are the run's
	int show_state;
	struct report_measurement *snapshots;	// in the order taken
	int snapshot_count;
	int snapshots_taken;		// in the session, chosen or not (summary table)
};

struct report_data {
	struct project_metadata project;	// copy
	int include_owner;		// the owner is left out unless asked for
	enum render_theme_id theme;	// RENDER_DOC_COLOR or RENDER_DOC_MONO
	char *date;			// when it was exported, "YYYY-MM-DD HH:MM"
	double paperstrip_tick_seconds, paperstrip_window_seconds;
	int summary;			// the table of the runs on the first page
	struct report_run *runs;	// in chronological order
	int run_count;
	struct report_measurement current;	// the real-time tab, frozen, or no snapshot
	struct report_measurement *others;	// snapshots without a run (older files)
	int other_count;
	GdkPixbuf *logo;		// the program icon for the page header (owned), or NULL
};

#define REPORT_ERROR_WRITE 1	// the file could not be written (the previous one is kept)
#define REPORT_ERROR_NO_PDF 2	// this Cairo has no PDF backend

void report_data_free(struct report_data *d);
/* Draw the report on cr, one A4 page after another (cairo_show_page). With
 * total_pages 0 the footers say "page N of N"; returns the number of pages. */
int report_render(cairo_t *cr, const struct report_data *d, int total_pages);
/* Write the report to path safely (temporary file, then replace). 0 = written. */
int report_write_pdf(const char *path, const struct report_data *d);

/* report_dialog.c: File > Export report */
void report_dialog_open(struct main_window *w);
int session_write_file_safely(const char *path, int (*writer)(FILE *f, void *data), void *data);

#endif // TG_EXPERT_TG_H_INCLUDED