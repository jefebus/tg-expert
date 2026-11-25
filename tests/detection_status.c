#include <assert.h>
#include <string.h>

#include "../src/tg.h"
static char drawn_text[1024];
static void capture_text(cairo_t *cr, const char *text);
#define cairo_show_text capture_text
#include "../src/output_panel.c"
#undef cairo_show_text

static void capture_text(cairo_t *cr, const char *text)
{
	assert(strlen(drawn_text) + strlen(text) < sizeof(drawn_text));
	strcat(drawn_text, text);
	cairo_show_text(cr, text);
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	assert(detection_status_message(DETECTION_STATUS_OK) == NULL);
#ifdef DEBUG
	const char *messages[] = {
		"No watch detected: no valid beat period found.",
		"No watch detected: the detected period is too long.",
		"No watch detected: tic and toc could not be separated.",
		"No watch detected: the beat period is too unstable.",
		"Rate only: tic and toc could not be separated; beat error and amplitude are unavailable.",
		"Amplitude unavailable: lift pulses could not be measured.",
		"Amplitude unavailable: the measured amplitude is too low.",
		"Amplitude unavailable: the measured amplitude is too high.",
		"Amplitude unavailable: tic and toc amplitudes differ too much."
	};
#endif
	for(int i = DETECTION_STATUS_NO_WAVEFORM;
			i <= DETECTION_STATUS_AMPLITUDE_INCONSISTENT; i++) {
#ifdef DEBUG
		assert(!strcmp(detection_status_message(i),
			messages[i - DETECTION_STATUS_NO_WAVEFORM]));
#else
		assert(!strcmp(detection_status_message(i),
			i >= DETECTION_STATUS_AMPLITUDE_NOT_FOUND ?
			"Amplitude unavailable." : i == DETECTION_STATUS_RATE_ONLY ?
			"Rate only: beat error and amplitude unavailable." : "No watch detected."));
#endif
	}
	struct snapshot snapshot = {0};
	struct output_panel panel = {0};
	panel.snst = &snapshot;
	panel.detection_status_label = gtk_label_new("");
	gtk_widget_show(panel.detection_status_label);

	snapshot.detection_status = DETECTION_STATUS_NO_WAVEFORM;
	gint64 before = g_get_monotonic_time();
	update_detection_status(&panel);
	assert(gtk_widget_get_visible(panel.detection_status_label));
	gint64 first_deadline = panel.detection_status_until;
	assert(first_deadline >= before + 2000000);

	snapshot.detection_status = DETECTION_STATUS_AMPLITUDE_TOO_HIGH;
	update_detection_status(&panel);
	assert(panel.shown_detection_status == DETECTION_STATUS_NO_WAVEFORM);
	assert(panel.detection_status_until == first_deadline);

	snapshot.detection_status = DETECTION_STATUS_AMPLITUDE_TOO_LOW;
	panel.detection_status_until = g_get_monotonic_time() - 1;
	update_detection_status(&panel);
#ifdef DEBUG
	assert(panel.shown_detection_status == DETECTION_STATUS_AMPLITUDE_TOO_LOW);
	assert(!strcmp(gtk_label_get_text(GTK_LABEL(panel.detection_status_label)),
		"Amplitude unavailable: the measured amplitude is too low."));
#else
	assert(panel.shown_detection_status == DETECTION_STATUS_AMPLITUDE_NOT_FOUND);
	assert(!strcmp(gtk_label_get_text(GTK_LABEL(panel.detection_status_label)),
		"Amplitude unavailable."));
	gint64 amplitude_deadline = panel.detection_status_until;
	snapshot.detection_status = DETECTION_STATUS_AMPLITUDE_INCONSISTENT;
	update_detection_status(&panel);
	assert(panel.detection_status_until == amplitude_deadline);
#endif

	snapshot.detection_status = DETECTION_STATUS_OK;
	panel.detection_status_until = g_get_monotonic_time() + 2000000;
	update_detection_status(&panel);
	assert(gtk_widget_get_visible(panel.detection_status_label));
	panel.detection_status_until = g_get_monotonic_time() - 1;
	update_detection_status(&panel);
	assert(gtk_widget_get_visible(panel.detection_status_label));
	assert(!strcmp(gtk_label_get_text(GTK_LABEL(panel.detection_status_label)), " "));
	assert(!gtk_widget_get_has_tooltip(panel.detection_status_label));
	gtk_widget_destroy(panel.detection_status_label);

	struct processing_buffers buffers = {0};
	snapshot.pb = &buffers;
	snapshot.rate_only = 1;
	snapshot.rate = 23;
	snapshot.amp = 230;
	snapshot.be = 1.5;
	snapshot.la = 52;
	snapshot.guessed_bph = 21600;
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 800, 100);
	cairo_t *cr = cairo_create(surface);
	draw_results_table(cr, 0, 800, 100, &snapshot, &buffers, 0);
	assert(strstr(drawn_text, "+23s/d------52"));
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	struct snapshot *saved = calloc(1, sizeof(*saved));
	struct output_panel *saved_panel = init_output_panel(NULL, saved, 0);
	assert(gtk_widget_get_visible(saved_panel->detection_status_label));
	assert(!strcmp(gtk_label_get_text(GTK_LABEL(saved_panel->detection_status_label)), " "));
	int empty_min, empty_natural, message_min, message_natural;
	gtk_widget_get_preferred_height_for_width(saved_panel->detection_status_label, 120,
		&empty_min, &empty_natural);
	saved->rate_only = 1;
	saved->detection_status = DETECTION_STATUS_RATE_ONLY;
	update_detection_status(saved_panel);
	assert(!strcmp(gtk_label_get_text(GTK_LABEL(saved_panel->detection_status_label)),
		detection_status_message(DETECTION_STATUS_RATE_ONLY)));
	gtk_widget_get_preferred_height_for_width(saved_panel->detection_status_label, 120,
		&message_min, &message_natural);
	assert(empty_min > 0 && empty_min == message_min && empty_natural == message_natural);
	assert(gtk_widget_get_has_tooltip(saved_panel->detection_status_label));
	char *tooltip = gtk_widget_get_tooltip_text(saved_panel->detection_status_label);
	assert(!strcmp(tooltip, detection_status_message(DETECTION_STATUS_RATE_ONLY)));
	g_free(tooltip);
	gtk_widget_show_all(saved_panel->panel);
	assert(gtk_widget_get_visible(saved_panel->detection_status_label));
	saved->detection_status = DETECTION_STATUS_OK;
	saved_panel->detection_status_until = g_get_monotonic_time() - 1;
	update_detection_status(saved_panel);
	gtk_widget_get_preferred_height_for_width(saved_panel->detection_status_label, 120,
		&message_min, &message_natural);
	assert(empty_min == message_min && empty_natural == message_natural);
	gtk_widget_destroy(saved_panel->panel);
	op_destroy(saved_panel);

	saved = calloc(1, sizeof(*saved));
	saved->rate_only = 1;
	saved->detection_status = DETECTION_STATUS_RATE_ONLY;
	saved_panel = init_output_panel(NULL, saved, 0);
	assert(!strcmp(gtk_label_get_text(GTK_LABEL(saved_panel->detection_status_label)),
		detection_status_message(DETECTION_STATUS_RATE_ONLY)));
	gtk_widget_destroy(saved_panel->panel);
	op_destroy(saved_panel);
	return 0;
}
