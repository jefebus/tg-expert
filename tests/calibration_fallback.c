#include <assert.h>
#include <math.h>

#include "../src/tg.h"

extern float pa_buffers[PA_BUFF_SIZE];
extern int write_pointer;
extern uint64_t timestamp;
extern pthread_mutex_t audio_mutex;

static void fill_ticks(uint64_t end, double drift, int silent_tail)
{
	const int rate = 12000;
	double period = rate * (1 + drift / 86400);
	uint64_t start = end - write_pointer;
	memset(pa_buffers, 0, sizeof(pa_buffers));
	for(int64_t n = (int64_t)floor(start / period); ; n++) {
		int64_t position = llround((n + 0.5) * period) - (int64_t)start;
		if(position >= write_pointer - silent_tail * rate)
			break;
		if(position >= 0)
			pa_buffers[position] = 1;
	}
	timestamp = end;
}

int main(void)
{
	struct calibration_data synthetic = {0};
	double synthetic_times[5], synthetic_phases[5], original_phases[5];
	synthetic.size = 5;
	synthetic.times = synthetic_times;
	synthetic.phases = synthetic_phases;
	synthetic.state = -1;
	synthetic.calibration = 13;
	for(int i = 0; i < synthetic.size; i++) {
		synthetic.times[i] = 20 * i;
		synthetic.phases[i] = fmod(0.37 + 2.5 * synthetic.times[i] / 86400, 1);
	}
	memcpy(original_phases, synthetic.phases, sizeof(original_phases));
	struct calibration_estimate estimate;
	synthetic.wp = 2;
	assert(!estimate_calibration(&synthetic, &estimate));
	synthetic.wp = synthetic.size;
	assert(estimate_calibration(&synthetic, &estimate));
	assert(fabs(estimate.calibration - 2.5) < 1e-6);
	assert(estimate.observations == synthetic.size);
	assert(fabs(estimate.duration - 80) < 1e-9);
	assert(synthetic.state == -1 && synthetic.calibration == 13);
	assert(memcmp(original_phases, synthetic.phases, sizeof(original_phases)) == 0);

	synthetic.phases[2] = NAN;
	assert(!estimate_calibration(&synthetic, &estimate));
	for(int i = 0; i < synthetic.size; i++)
		synthetic.phases[i] = fmod(0.37 + 200.0 * synthetic.times[i] / 86400, 1);
	assert(!estimate_calibration(&synthetic, &estimate));

	struct main_window window = {0};
	window.config_file = g_key_file_new();
	algorithm_settings_load(&window);
	g_key_file_free(window.config_file);

	struct processing_buffers buffers[NSTEPS] = {0};
	for(int i = 0; i < NSTEPS; i++) {
		buffers[i].sample_rate = 12000;
		buffers[i].sample_count = 24000 << i;
		setup_buffers(&buffers[i]);
	}
	assert(pthread_mutex_init(&audio_mutex, NULL) == 0);
	write_pointer = buffers[NSTEPS - 1].sample_count;
	struct processing_data pd = {0};
	pd.buffers = buffers;
	algorithm_parameters[ALG_CAL_COUNT] = 20;
	struct calibration_data cd = {0};
	setup_cal_data(&cd);

	uint64_t end = write_pointer + 10 * 12000;
	fill_ticks(end, 0, 0);
	assert(analyze_pa_data_cal(&pd, &cd) == NSTEPS);
	assert(cd.wp == 1);

	/* Silence in the latest two seconds makes the shortest check fail. */
	cd.wp = 0;
	cd.state = 0;
	for(int i = 0; i < cd.size; i++) {
		end = write_pointer + (uint64_t)(100 + 30 * i) * 12000;
		fill_ticks(end, 2, 2);
		memcpy(buffers[0].samples, pa_buffers + write_pointer - buffers[0].sample_count,
			buffers[0].sample_count * sizeof(*pa_buffers));
		assert(test_cal(&buffers[0]) != 0);
		assert(analyze_pa_data_cal(&pd, &cd) == NSTEPS);
	}
	assert(cd.wp == cd.size && cd.state == 1);
	assert(fabs(cd.calibration - 2) < 0.2);

	memset(pa_buffers, 0, sizeof(pa_buffers));
	cd.wp = 0;
	cd.state = 0;
	assert(analyze_pa_data_cal(&pd, &cd) == 0);
	assert(cd.wp == 0 && cd.state == 0);

	uint32_t noise = 1;
	for(int i = 0; i < write_pointer - 2 * 12000; i++) {
		noise = 1664525u * noise + 1013904223u;
		pa_buffers[i] = ((int32_t)(noise >> 16) - 32768) / 32768.0f;
	}
	assert(analyze_pa_data_cal(&pd, &cd) == 0);
	assert(cd.wp == 0 && cd.state == 0);

	fill_ticks(end, 0, 0);
	memset(pa_buffers, 0, (write_pointer - 2 * 12000) * sizeof(*pa_buffers));
	assert(analyze_pa_data_cal(&pd, &cd) == 1);
	assert(cd.wp == 0 && cd.state == 0);

	for(int i = 0; i < NSTEPS; i++) {
		pb_destroy(&buffers[i]);
		buffers[i].sample_count = 12000 << i;
		setup_buffers(&buffers[i]);
	}
	write_pointer = buffers[NSTEPS - 1].sample_count;
	pd.is_light = 1;
	end = write_pointer + 100 * 12000;
	fill_ticks(end, 0, 2);
	timestamp *= 2;
	memcpy(buffers[1].samples, pa_buffers + write_pointer - buffers[1].sample_count,
		buffers[1].sample_count * sizeof(*pa_buffers));
	assert(test_cal(&buffers[1]) != 0);
	assert(analyze_pa_data_cal(&pd, &cd) == NSTEPS);
	assert(cd.wp == 1);

	cal_data_destroy(&cd);
	for(int i = 0; i < NSTEPS; i++)
		pb_destroy(&buffers[i]);
	pthread_mutex_destroy(&audio_mutex);
	return 0;
}
