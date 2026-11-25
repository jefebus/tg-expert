/* Functions of interface.c that the analysis and file code calls, for test
 * programs linked without the interface (which holds main()). */
#include "../src/tg.h"

int preset_bph[] = PRESET_BPH;	// defined in interface.c, used by computer.c

void error(char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputc('\n', stderr);
}

void request_algorithm_restart(struct main_window *w, int light)
{
	UNUSED(w);
	UNUSED(light);
}

/* Used by correlations.c, trend_render.c and report.c */
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
