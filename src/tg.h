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
	double fixed_rate, struct trend_plot_ranges *ranges);

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
	GtkWidget *correlation_areas[3];
	int correlation_position;   // -1 = all positions
	GtkWidget *settling_spin;
	GtkWidget *settling_status;
	GtkWidget *detection_status_label;
	enum detection_status shown_detection_status;
	gint64 detection_status_until;
	void (*settling_setting_changed)(void *, int);
	void *settling_setting_data;
	struct trend *trend;         // owned; NULL for snapshots loaded from a file
	int trend_rate_range;        // s/d shown either side of 0, 0 = automatic
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
	GtkWidget *open_item;
	GtkWidget *algorithm_item;
	GtkWidget *save_item;
	GtkWidget *save_all_item;
	GtkWidget *export_item;
	GtkWidget *close_all_item;
	struct output_panel *active_panel;

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

/* serializer.c */
/* trends[i] is the long-term trend of snapshot i, NULL if it has none */
int write_file(FILE *f, struct snapshot **s, char **names, struct trend **trends, uint64_t cnt);
int write_trend_csv(FILE *f, const struct trend *t, char separator, char decimal);
int read_file(FILE *f, struct snapshot ***s, char ***names, struct trend ***trends, uint64_t *cnt);
