/* Functions of output_panel.c that correlations.c calls, for test programs
 * linked without the output panel. Not in stubs.c: tests that include
 * output_panel.c (detection_status) link stubs.c and would define them twice. */
#include "../src/tg.h"

void format_duration(char *s, size_t n, double seconds)
{
	snprintf(s, n, "%.0f s", seconds);
}

GtkWidget *op_create_trend_span_combo(struct output_panel *op)
{
	UNUSED(op);
	return NULL;
}

GtkWidget *op_create_trend_be_combo(struct output_panel *op)
{
	UNUSED(op);
	return NULL;
}
