#include <assert.h>

#include "../src/tg.h"

int main(void)
{
	struct trend_sample samples[] = {
		{0, 5, 200, 0.5, POS_DU, 0},
		{2, -8, 220, 5, POS_DU, 0}
	};
	struct trend trend = {0};
	trend.samples = samples;
	trend.count = 2;
	struct trend_plot_ranges ranges;

	trend_get_plot_ranges(&trend, 0, 2, 0, 0, &ranges);
	assert(ranges.be_min == 0 && ranges.be_max == 5);
	assert(ranges.rate_min == -10 && ranges.rate_max == 10);
	assert(ranges.amp_min == 120 && ranges.amp_max == 360);
	trend_get_plot_ranges(&trend, 0, 2, 0, 1, &ranges);
	assert(ranges.be_max == 1 && ranges.rate_max == 10);
	trend_get_plot_ranges(&trend, 0, 2, 50, 2, &ranges);
	assert(ranges.be_max == 2 && ranges.rate_max == 50);
	trend_get_plot_ranges(&trend, 0, 0, 0, 0, &ranges);
	assert(ranges.be_max == 1);
	assert(samples[1].be == 5);
	return 0;
}
