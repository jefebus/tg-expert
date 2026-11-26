/* TG-31: preview of the shared renderers in the three themes.
 *
 * Writes render-preview.pdf (or the file given as argument) with one page per
 * theme: screen, document colour and document monochrome. Each page holds
 * the three correlation graphs of a synthetic recording, drawn as vectors
 * directly on the PDF surface, at the sizes a report would use (points).
 * Check by eye that the document themes are legible on white, that the
 * monochrome page keeps the positions apart by symbol, and that the readings
 * beyond the fixed rate range are counted ("Off scale") and not drawn on
 * the border.
 *
 * Then it writes report-preview.pdf: the PDF report (TG-16) of the same
 * synthetic data, with Markdown notes, as File > Export report does: the
 * table of the runs, run 1 (closed, two snapshots, final state left out),
 * run 2 (in progress, its state shown) and a snapshot of an older file. Give
 * "mono" as second argument for the monochrome report:
 *   ./render_preview render-preview.pdf mono
 *
 * Build (MSYS2 or Linux), from the source tree:
 *   gcc -o render_preview tests/render_preview.c src/render.c src/correlations.c \
 *       src/trend.c src/trend_render.c src/waveform_render.c src/paperstrip_render.c \
 *       src/report.c src/session.c src/serializer.c \
 *       -DPROGRAM_NAME='"tg-expert"' -DVERSION='"preview"' \
 *       $(pkg-config --cflags --libs gtk+-3.0 cairo-pdf pangocairo) -lm
 * The few functions of the rest of the program that these need are stubbed
 * below. */
#include <assert.h>
#include <cairo-pdf.h>
#include "../src/tg.h"

#define CHECK(X) do { if(!(X)) { fprintf(stderr, "line %d: %s\n", __LINE__, #X); abort(); } } while(0)

/* Stubs: only used by the GTK page and the tooltips, not by the renderers */
const char *watch_position_code(int pos)
{
	static const char *codes[POS_COUNT] = {NULL, "DU", "DD", "CU", "CD", "CL", "CR"};
	return pos > POS_NONE && pos < POS_COUNT ? codes[pos] : NULL;
}
const char *watch_position_label(int pos)
{
	static const char *labels[POS_COUNT] = {"(not set)", "Dial up (DU)", "Dial down (DD)",
		"Crown up (CU)", "Crown down (CD)", "Crown left (CL)", "Crown right (CR)"};
	return pos >= POS_NONE && pos < POS_COUNT ? labels[pos] : labels[0];
}
/* The report frees its copies with snapshot_destroy(); here they are
 * shallow copies of the static synthetic snapshot (preview_snapshot()):
 * only the struct and its notes are their own */
void snapshot_destroy(struct snapshot *s) { free(s->notes); free(s); }
void format_duration(char *s, size_t n, double seconds) { snprintf(s, n, "%.0f s", seconds); }
GtkWidget *op_create_trend_span_combo(struct output_panel *op) { UNUSED(op); return NULL; }
GtkWidget *op_create_trend_be_combo(struct output_panel *op) { UNUSED(op); return NULL; }
int algorithm_settings_custom(int light) { UNUSED(light); return 0; }	// used by trend.c

static struct trend *synthetic_trend(void)
{
	static struct trend_sample samples[600];
	struct trend *t = calloc(1, sizeof(*t));
	CHECK(t);
	int n = 0;
	unsigned seed = 12345;
	// Repeatable scatter around a trend for each position, like real readings
#define NOISE() ((seed = seed * 1103515245u + 12345u), ((seed >> 8) & 0xffff) / 65535.0 - .5)
	for(int pos = POS_NONE; pos < POS_COUNT; pos++) {
		for(int i = 0; i < 40 && n < 600; i++, n++) {
			double phase = i / 40.0;
			struct trend_sample *s = &samples[n];
			s->t = n * TREND_INTERVAL_S;
			s->amp = 230 + 60 * phase + 5 * pos + 8 * NOISE();
			s->rate = -6 + 3 * pos + 4 * sin(phase * 6.28) + (pos == POS_CR ? 12 : 0) + 3 * NOISE();
			s->be = 0.2 + 0.1 * pos + 0.3 * phase + 0.08 * NOISE();
			s->position = pos;
			s->settling = 0;
		}
	}
	// trend_set_samples() fills the statistics as when a file is loaded
	static float tm[600], rate[600], amp[600], be[600];
	static int position[600], settling[600];
	for(int i = 0; i < n; i++) {
		tm[i] = samples[i].t;
		rate[i] = samples[i].rate;
		amp[i] = samples[i].amp;
		be[i] = samples[i].be;
		position[i] = samples[i].position;
		settling[i] = i % 40 < 3;	// a few settling readings after each change
	}
	t->samples = NULL;	// trend_set_samples() allocates its own copy
	t->count = 0;
	t->bph = 28800;
	t->la = 52;
	t->cal = 0;
	t->nominal_sr = 44100;
	CHECK(trend_set_samples(t, tm, rate, amp, be, position, settling, n,
		n * TREND_INTERVAL_S) == 0);
	return t;
}

/* A snapshot with a synthetic averaged waveform: 21600 bph at 44.1 kHz, a
 * tic and a toc made of a lift pulse and a louder drop */
static struct snapshot *synthetic_snapshot(void)
{
	static float wave[14700];
	static struct processing_buffers pb;
	static struct snapshot s;
	const double sr = 44100, period = 7200.0 / 21600 * sr;
	const int tic = 2000, toc = 9350;
	for(int i = 0; i < (int)period; i++) {
		double v = 0.02;
		for(int b = 0; b < 2; b++) {
			double t = (i - (b ? toc : tic)) / sr * 1000;	// ms from the beat
			v += 0.35 * exp(-(t + 9) * (t + 9) / 0.8);	// lift pulse, 9 ms before
			v += (b ? 0.8 : 1.0) * exp(-t * t / 1.5);	// drop
			v += 0.25 * exp(-(t - 4) * (t - 4) / 3);
		}
		wave[i] = v;
	}
	pb.waveform = wave;
	pb.sample_count = (int)period;
	pb.period = period;
	pb.waveform_max = 1.1;
	pb.sample_rate = (int)sr;
	pb.tic = tic;
	pb.toc = toc;
	pb.tic_pulse = 9 * sr / 1000;
	pb.toc_pulse = 9 * sr / 1000;
	s.pb = &pb;
	s.sample_rate = sr;
	s.nominal_sr = (int)sr;
	s.bph = s.guessed_bph = 21600;
	s.la = 52;
	s.amp = 280;

	// A minute of beats for the paper strip: +8 s/d, so that the trace leans
	// right; tic and toc alternate, with a small beat error. The amplitude
	// drops slowly and, for a few readings, below the scale (not drawn).
	static uint64_t events[360];
	static unsigned char tictoc[360];
	static float amps[180];
	static uint64_t amps_time[180];
	const double beat = sr * 3600 / 21600 * (1 - 8.0 / 86400);
	for(int i = 0; i < 360; i++) {
		events[i] = (uint64_t)(sr + i * beat + (i % 2 ? 0.4e-3 * sr : 0));
		tictoc[i] = !(i % 2);
	}
	for(int i = 0; i < 180; i++) {
		amps_time[i] = events[2 * i];
		double deg = 290 - i * 0.4 - (i > 150 && i < 156 ? 220 : 0);
		amps[i] = deg / s.la;
	}
	s.events = events;
	s.events_tictoc = tictoc;
	s.events_count = 360;
	s.events_wp = 359;
	s.amps = amps;
	s.amps_time = amps_time;
	s.amps_count = 180;
	s.amps_wp = 179;
	s.rate = 8;
	s.trace_zoom = 1;
	s.trace_centering = 0;
	s.timestamp = events[359] + 100;
	return &s;
}

/* A copy of the synthetic snapshot for the report, as a run's state
 * (saved_state 1) or a snapshot of a run (its number snap) */
static struct snapshot *preview_snapshot(int position, int64_t wall_time, int saved_state,
	int run, int snap)
{
	struct snapshot *c = malloc(sizeof(*c));
	CHECK(c);
	*c = *synthetic_snapshot();
	c->notes = NULL;
	c->position = position;
	c->wall_time = wall_time;
	c->saved_state = saved_state;
	c->run = run;
	c->snap = snap;
	c->run_start = 0;
	return c;
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "render-preview.pdf";
	const double page_w = 595, page_h = 842;	// A4 in points
	cairo_surface_t *surface = cairo_pdf_surface_create(path, page_w, page_h);
	CHECK(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
	cairo_t *cr = cairo_create(surface);
	struct trend *t = synthetic_trend();

	for(int theme = 0; theme < RENDER_THEME_COUNT; theme++) {
		const struct render_theme *th = render_theme_get(theme);
		double side = (page_w - 4 * 20) / 3;
		for(int row = 0; row < 2; row++) {
			for(int kind = 0; kind < CORRELATION_COUNT; kind++) {
				struct correlation_options o = {0};
				o.kind = kind;
				o.position = -1;
				correlation_time_range(t, 0, &o.t0, &o.t1);
				o.rate_range = row ? 10 : 0;	// second row: fixed ±10 s/d, some off scale
				o.be_range = 0;
				o.font = 7;
				cairo_save(cr);
				cairo_translate(cr, 20 + kind * (side + 20), 40 + row * (side + 40));
				render_correlation(cr, side, side, t, &o, th);
				cairo_restore(cr);
			}
		}
		// The legend, drawn with the same symbols as the points (shared with
		// the report). Its text uses the theme colour: on the screen theme it
		// is white, so it gets a black strip behind it.
		double ly = 40 + 2 * (side + 40) + 10;
		render_set_rgb(cr, th->background, 1);
		cairo_rectangle(cr, 20, ly - 10, page_w - 40, 14);
		cairo_fill(cr);
		render_position_legend(cr, th, 30, ly, 8, 0);
		// The Trend page below: statistics and the three graphs, rate fixed
		// at ±10 s/d so that some readings are off scale
		struct trend_render_options to = {
			.sections = TREND_RENDER_STATS | TREND_RENDER_GRAPHS,
			.span_s = 0,
			.rate_range = 10,
			.be_range = 0,
			.font = 8,
			.calibrating = 0
		};
		double top = 40 + 2 * (side + 40) + 30;
		cairo_save(cr);
		cairo_translate(cr, 20, top);
		render_trend(cr, page_w - 40, page_h - top - 30, t, &to, th);
		cairo_restore(cr);
		cairo_show_page(cr);

		// Second page: one snapshot laid out as on screen, the paper strip
		// on the left and the tic, toc and full period waveforms stacked on
		// the right, with the waveform legend below them
		struct snapshot *snap = synthetic_snapshot();
		const double top2 = 40, block_h = 480, gap = 16;
		const double strip_w = 200;
		const double wave_x = 20 + strip_w + gap, wave_w = page_w - 20 - wave_x;
		const double legend_h = 16;
		const double wave_h = (block_h - legend_h - 2 * 8) / 3;

		// The report draws no rate guide (NAN): it is for the live display
		struct paperstrip_options po = {
			.tick_seconds = 10,
			.window_seconds = 60,
			.guide_rate = NAN,
			.now = snap->timestamp,
			.font = 8,
			.legend = 1
		};
		cairo_save(cr);
		cairo_translate(cr, 20, top2);
		render_paperstrip(cr, strip_w, block_h, snap, &po, th);
		cairo_restore(cr);

		for(int k = 0; k < 3; k++) {
			struct waveform_options wo = {
				.kind = k == 0 ? WAVEFORM_TIC : k == 1 ? WAVEFORM_TOC : WAVEFORM_PERIOD,
				.negative_span = NEGATIVE_SPAN,
				.font = 8
			};
			cairo_save(cr);
			cairo_translate(cr, wave_x, top2 + k * (wave_h + 8));
			render_waveform(cr, wave_w, wave_h, snap, &wo, th);
			cairo_restore(cr);
		}
		// The waveform legend (the text in the theme colour: a strip of the
		// background behind it, for the screen theme)
		double wy = top2 + 3 * wave_h + 2 * 8 + legend_h - 4;
		render_set_rgb(cr, th->background, 1);
		cairo_rectangle(cr, wave_x, wy - 11, wave_w, 15);
		cairo_fill(cr);
		render_waveform_legend(cr, th, wave_x + 6, wy, 8);
		cairo_show_page(cr);
	}
	cairo_destroy(cr);
	cairo_surface_finish(surface);
	CHECK(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
	cairo_surface_destroy(surface);
	printf("%s written\n", path);

	// The whole report, as File > Export report writes it, with Markdown
	// notes (including syntax that is not supported, shown as written)
	struct report_data d;
	memset(&d, 0, sizeof(d));
	project_metadata_init(&d.project);
	d.project.title = strdup("Revisi\xC3\xB3n Omega \xE2\x80\x9C" "Seamaster\xE2\x80\x9D");
	d.project.project_reference = strdup("OT-2026/0142");
	d.project.watch_label = strdup("Omega 565");
	d.project.owner = strdup("Mar\xC3\xAD" "a Garc\xC3\xAD" "a");
	d.project.last_service_date = strdup("2019-05");
	d.project.notes = strdup(
		"# Estado inicial\n"
		"Reloj recibido **parado**. Cristal rayado, corona _floja_.\n"
		"Segunda l\xC3\xADnea del mismo p\xC3\xA1rrafo.\n"
		"\n"
		"## Trabajos\n"
		"- Desmontaje y limpieza\n"
		"- Cambio de *muelle real*\n"
		"- Lubricaci\xC3\xB3n con 9010 y 9415\n"
		"\n"
		"1. Primera prueba en 6 posiciones\n"
		"2. Ajuste de **error de batido**\n"
		"\n"
		"Literal: snake_case, 2 * 3 = 6, <etiqueta> & [enlace](https://example.com), `c\xC3\xB3" "digo`, ** sin cerrar\n"
		"#### Nivel 4 queda como texto\n");
	d.include_owner = 0;
	d.theme = argc > 2 && !strcmp(argv[2], "mono") ? RENDER_DOC_MONO : RENDER_DOC_COLOR;
	d.date = g_strdup("2026-11-25 18:40");
	d.paperstrip_tick_seconds = 10;
	d.paperstrip_window_seconds = 60;
	// The program icon, from the source tree (run from the source folder)
	d.logo = gdk_pixbuf_new_from_file("icons/256x256/apps/tg-timer.png", NULL);
	if(!d.logo)
		fprintf(stderr, "icons/256x256/apps/tg-timer.png not found: the report uses the drawn logo\n");
	else if(d.theme == RENDER_DOC_MONO)	// as the export dialog does
		gdk_pixbuf_saturate_and_pixelate(d.logo, d.logo, 0, FALSE);
	d.summary = 1;
	// Run 1, closed, with two snapshots: its final state is left out, as the
	// dialog does by default for a run with snapshots
	const int64_t t0 = 1795620000;	// 2026-11-25 16:20 in Madrid (15:20 UTC)
	d.runs = g_new0(struct report_run, 3);
	d.run_count = 2;
	struct report_run *r1 = &d.runs[0];
	r1->number = 1;
	r1->name = g_strdup("antes del ajuste");
	r1->start = t0;
	r1->end = t0 + 5400;
	r1->trend = trend_clone(t);
	r1->show_trends = 1;
	r1->state = (struct report_measurement){preview_snapshot(POS_DU, t0 + 5400, 1, 1, 0),
		g_strdup("Tramo 1: estado final"), NEGATIVE_SPAN};
	r1->state.snapshot->run_start = r1->start;
	r1->state.snapshot->notes = strdup("Notas del **tramo 1**: esfera arriba y luego abajo.");
	r1->show_state = 0;
	r1->snapshots = g_new0(struct report_measurement, 3);
	r1->snapshot_count = 2;
	r1->snapshots_taken = 2;
	r1->snapshots[0] = (struct report_measurement){preview_snapshot(POS_DU, t0 + 1800, 0, 1, 1),
		g_strdup("1.1 (DU)"), NEGATIVE_SPAN};
	r1->snapshots[0].snapshot->notes = strdup("Tras el ajuste.\n\n- Amplitud **estable**\n- Sin ruidos _an\xC3\xB3malos_");
	r1->snapshots[1] = (struct report_measurement){preview_snapshot(POS_DD, t0 + 4800, 0, 1, 2),
		g_strdup("1.2 esfera abajo (DD)"), NEGATIVE_SPAN};
	// Run 2, being recorded: no snapshots, so its state is shown
	struct report_run *r2 = &d.runs[1];
	r2->number = 2;
	r2->in_progress = 1;
	r2->start = t0 + 7200;
	r2->end = 1795628400;	// the export, 2026-11-25 18:40
	r2->trend = trend_clone(t);
	r2->show_trends = 1;
	r2->state = (struct report_measurement){preview_snapshot(POS_CU, r2->end, 1, 2, 0),
		g_strdup("Tramo 2 (en curso): estado al exportar"), NEGATIVE_SPAN};
	r2->state.snapshot->run_start = r2->start;
	r2->show_state = 1;
	// A snapshot of an older file, without a run
	d.others = g_new0(struct report_measurement, 2);
	d.other_count = 1;
	d.others[0] = (struct report_measurement){preview_snapshot(POS_NONE, 0, 0, 0, 0),
		g_strdup("Omega 565"), NEGATIVE_SPAN};
	const char *report_path = "report-preview.pdf";
	CHECK(report_write_pdf(report_path, &d) == 0);
	printf("%s written\n", report_path);
	report_data_free(&d);	// the copies: see snapshot_destroy() above
	trend_free(t);
	return 0;
}
