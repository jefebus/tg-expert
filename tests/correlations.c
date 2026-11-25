#include <assert.h>

#include "../src/correlations.c"

int main(void)
{
	static const double rate_ranges[] = {2, 5, 10, 20, 50, 100, 200, 500, 1000};
	static const double be_ranges[] = {1, 2, 5, 10, 20, 50, 100};
	for(size_t i = 0; i < sizeof(rate_ranges) / sizeof(rate_ranges[0]); i++) {
		double step = rate_ranges[i] / 2;
		int divisions = axis_subdivisions(step, 1);
		assert(divisions >= 5 && divisions <= 10);
		assert(fabs(step * 10 / divisions - round(step * 10 / divisions)) < 1e-9);
	}
	for(size_t i = 0; i < sizeof(be_ranges) / sizeof(be_ranges[0]); i++) {
		double step = be_ranges[i] / 2;
		int divisions = axis_subdivisions(step, 2);
		assert(divisions >= 5 && divisions <= 10);
		assert(fabs(step * 100 / divisions - round(step * 100 / divisions)) < 1e-9);
	}
	assert(axis_subdivisions(120, 1) == 6);
	assert(axis_subdivisions(60, 1) == 6);
	assert(axis_subdivisions(75, 1) == 5);
	assert(axis_subdivisions(150, 1) == 5);

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 121, 121);
	cairo_t *cr = cairo_create(surface);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	struct correlation_plot grid = {0};
	grid.xmin = 120; grid.xmax = 360;
	grid.ymin = -20; grid.ymax = 20;
	grid.width = grid.height = 120;
	draw_axis_grid(cr, &grid, CORRELATION_AMP_RATE, render_theme_get(RENDER_SCREEN));
	cairo_surface_flush(surface);
	unsigned char *pixels = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	assert(pixels[stride * 1 + 9 * 4 + 3] != 0);
	assert(pixels[stride * 4 + 9 * 4 + 3] == 0);
	assert(pixels[stride * 4 + 59 * 4 + 3] != 0);
	assert(pixels[stride * 5 + 1 * 4 + 3] != 0);
	assert(pixels[stride * 5 + 4 * 4 + 3] == 0);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	struct trend_sample samples[] = {
		{0, 10, 180, 0, POS_DU, 0},
		{2, 10, 220, 0, POS_DU, 0},
		{4, -20, 260, 1, POS_DD, 0},
		{6, -20, 300, 1, POS_DD, 0},
		{8, 5, 200, 2, POS_NONE, 0},
		{10, 0, 0, 0, POS_NONE, 0}
	};
	struct trend trend = {0};
	trend.samples = samples;
	trend.count = sizeof(samples) / sizeof(samples[0]);

	double x, y;
	assert(correlation_values(&samples[5], CORRELATION_BE_RATE, &x, &y));
	assert(x == 0 && y == 0);
	assert(!correlation_values(&samples[5], CORRELATION_AMP_RATE, &x, &y));
	assert(!correlation_values(&samples[5], CORRELATION_BE_AMP, &x, &y));
	assert(correlation_values(&samples[0], CORRELATION_AMP_RATE, &x, &y));
	assert(x == 180 && y == 10);
	assert(correlation_values(&samples[1], CORRELATION_AMP_RATE, &x, &y));
	assert(x == 220 && y == 10);
	assert(correlation_values(&samples[2], CORRELATION_AMP_RATE, &x, &y));
	assert(x == 260 && y == -20);
	assert(correlation_values(&samples[3], CORRELATION_AMP_RATE, &x, &y));
	assert(x == 300 && y == -20);

	struct correlation_plot plot;
	correlation_measure(&plot, &trend, CORRELATION_AMP_RATE, -1, 0, 10);
	assert(plot.valid == 5 && plot.selected == 5);
	correlation_measure(&plot, &trend, CORRELATION_AMP_RATE, POS_DU, 0, 10);
	assert(plot.valid == 5 && plot.selected == 2);
	correlation_measure(&plot, &trend, CORRELATION_BE_RATE, POS_NONE, 0, 10);
	assert(plot.valid == 6 && plot.selected == 2);
	correlation_measure(&plot, &trend, CORRELATION_AMP_RATE, POS_NONE, 0, 10);
	assert(plot.valid == 5 && plot.selected == 1);
	correlation_measure(&plot, &trend, CORRELATION_AMP_RATE, POS_CU, 0, 10);
	assert(plot.valid == 5 && plot.selected == 0);
	correlation_measure(&plot, &trend, CORRELATION_BE_RATE, -1, 8, 10);
	assert(plot.valid == 2 && plot.selected == 2);

	struct correlation_options o = {0};
	o.t0 = 0;
	o.t1 = 10;
	o.kind = CORRELATION_AMP_RATE;
	correlation_ranges(&plot, &trend, &o);
	assert(plot.xmin == 120 && plot.xmax == 360);
	assert(plot.ymin == -20 && plot.ymax == 20);
	o.kind = CORRELATION_BE_AMP;
	correlation_ranges(&plot, &trend, &o);
	assert(plot.xmin == 0 && plot.xmax == 2);
	assert(plot.ymin == 120 && plot.ymax == 360);

	o.rate_range = 10;
	o.kind = CORRELATION_AMP_RATE;
	correlation_ranges(&plot, &trend, &o);
	assert(plot.ymin == -10 && plot.ymax == 10);
	plot.left = 20; plot.top = 30; plot.width = 100; plot.height = 100;
	// A reading beyond the scales is not drawn (not put on the border)
	x = y = -1;
	assert(!correlation_point(&plot, 400, -20, &x, &y));
	assert(x == -1 && y == -1);
	assert(correlation_point(&plot, 360, -10, &x, &y));
	assert(x == 120 && y == 130);
	// Font 10 gives the margins of the screen
	correlation_geometry(&plot, 400, 300, 10);
	assert(plot.width == 228 && plot.height == 228);
	correlation_geometry(&plot, 300, 400, 10);
	assert(plot.width == 227 && plot.height == 227);

	samples[2].settling = 1;
	samples[2].rate = -200;
	samples[2].amp = 90;
	samples[2].be = 50;
	assert(!correlation_values(&samples[2], CORRELATION_AMP_RATE, &x, &y));
	assert(!correlation_values(&samples[2], CORRELATION_BE_RATE, &x, &y));
	assert(!correlation_values(&samples[2], CORRELATION_BE_AMP, &x, &y));
	correlation_measure(&plot, &trend, CORRELATION_BE_RATE, POS_DD, 0, 10);
	assert(plot.valid == 5 && plot.selected == 1);
	o.rate_range = 0;
	o.kind = CORRELATION_BE_AMP;
	correlation_ranges(&plot, &trend, &o);
	assert(plot.xmin == 0 && plot.xmax == 50 && plot.ymin == 60);
	o.kind = CORRELATION_AMP_RATE;
	o.t0 = 8;
	correlation_ranges(&plot, &trend, &o);
	assert(plot.xmin == 60 && plot.ymin == -5 && plot.ymax == 5);

	samples[4].position = -1;
	assert(sample_position(&samples[4]) == POS_NONE);
	samples[5].be = NAN;
	assert(!correlation_values(&samples[5], CORRELATION_BE_RATE, &x, &y));

	struct trend *legacy = trend_new();
	assert(legacy);
	legacy->bph = 21600;
	legacy->la = 52;
	legacy->nominal_sr = 44100;
	float times[] = {0, 2};
	float rates[] = {0, -5};
	float amplitudes[] = {0, 200};
	float beat_errors[] = {0, .5};
	assert(!trend_set_samples(legacy, times, rates, amplitudes, beat_errors,
		NULL, NULL, 2, 2));
	assert(legacy->samples[0].position == POS_NONE);
	assert(legacy->samples[1].position == POS_NONE);
	assert(!trend_set_samples(legacy, times, rates, NULL, beat_errors,
		NULL, NULL, 2, 2));
	assert(legacy->samples[0].amp == 0 && legacy->samples[1].amp == 0);
	correlation_measure(&plot, legacy, CORRELATION_BE_RATE, POS_NONE, 0, 2);
	assert(plot.selected == 2);
	correlation_measure(&plot, legacy, CORRELATION_AMP_RATE, POS_NONE, 0, 2);
	assert(plot.valid == 0);
	trend_free(legacy);
	return 0;
}
