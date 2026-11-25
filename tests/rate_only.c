#include <assert.h>
#include <math.h>

#include "../src/computer.c"

extern float pa_buffers[PA_BUFF_SIZE];
extern int write_pointer;
extern uint64_t timestamp;
extern pthread_mutex_t audio_mutex;

int main(void)
{
	struct main_window window = {0};
	window.config_file = g_key_file_new();
	algorithm_settings_load(&window);
	g_key_file_free(window.config_file);

	struct processing_buffers buffers[NSTEPS] = {0};
	for(int i = 0; i < NSTEPS; i++) {
		buffers[i].sample_rate = 12000;
		buffers[i].sample_count = 24000 << i;
		setup_buffers(&buffers[i]);
		buffers[i].last_tic = buffers[i].last_toc = 0;
	}
	assert(pthread_mutex_init(&audio_mutex, NULL) == 0);
	write_pointer = buffers[NSTEPS - 1].sample_count;
	timestamp = 2 * (uint64_t)write_pointer;
	memset(pa_buffers, 0, sizeof(pa_buffers));
	for(int i = 1000; i < write_pointer; i += 4000)
		pa_buffers[i] = 1;
	/* A one-bin search forces tic/toc separation to fail deterministically. */
	algorithm_parameters[ALG_BE_SEARCH] = 0;

	struct processing_data pd = {0};
	pd.buffers = buffers;
	pd.is_light = 1;
	struct snapshot *s = calloc(1, sizeof(*s));
	s->nominal_sr = buffers[0].sample_rate;
	s->bph = 21600;
	s->la = 52;
	s->events_count = 8;
	s->events = calloc(s->events_count, sizeof(*s->events));
	s->events_tictoc = calloc(s->events_count, sizeof(*s->events_tictoc));
	s->amps_count = 8;
	s->amps = calloc(s->amps_count, sizeof(*s->amps));
	s->amps_time = calloc(s->amps_count, sizeof(*s->amps_time));
	s->trace_zoom = 1;
	struct computer computer = {0};
	computer.pdata = &pd;
	computer.actv = s;

	/* A valid period survives failed tic/toc detection, but must not add events. */
	compute_update(&computer);
	assert(buffers[0].period_ready && !buffers[0].ready);
	assert(buffers[NSTEPS - 1].period_ready && !buffers[NSTEPS - 1].ready);
	assert(buffers[NSTEPS - 1].detection_status == DETECTION_STATUS_NO_BEAT);
	assert(fabs(buffers[NSTEPS - 1].period - 4000) < 10);
	assert(isfinite(buffers[NSTEPS - 1].sigma));
	assert(buffers[NSTEPS - 1].sigma <=
		buffers[NSTEPS - 1].period * algorithm_parameters[ALG_PERIOD_SIGMA]);
	assert(buffers[NSTEPS - 1].events[0] == 0);
	compute_results(s);
	assert(!s->is_old && s->rate_only);
	assert(s->detection_status == DETECTION_STATUS_RATE_ONLY);
	assert(s->signal == NSTEPS && fabs(s->rate) < 500);
	assert(s->be == 0 && s->amp == 0);
	compute_events(&computer);
	assert(s->events_wp == 0 && s->events[0] == 0);
	assert(s->amps_wp == 0 && s->amps_time[0] == 0);

	struct trend *trend = trend_new();
	trend_update(trend, s, POS_NONE);
	assert(trend->count == 0);
	trend_free(trend);

	struct snapshot *copy = snapshot_clone(s);
	assert(copy->rate_only && !copy->is_old);
	snapshot_destroy(copy);

	s->timestamp = 1;
	FILE *f = tmpfile();
	assert(f);
	struct snapshot *snapshots[] = {s};
	char *names[] = {NULL};
	assert(write_file(f, snapshots, names, NULL, 1, NULL) == 0);
	rewind(f);
	struct snapshot **loaded = NULL;
	char **loaded_names = NULL;
	struct trend **loaded_trends = NULL;
	uint64_t count = 0;
	assert(read_file(f, &loaded, &loaded_names, &loaded_trends, &count, NULL) == 0);
	assert(count == 1 && loaded[0]->rate_only);
	assert(loaded[0]->detection_status == DETECTION_STATUS_RATE_ONLY);
	assert(loaded[0]->amp == 0 && loaded[0]->be == 0);
	snapshot_destroy(loaded[0]);
	free(loaded_names[0]);
	trend_free(loaded_trends[0]);
	free(loaded);
	free(loaded_names);
	free(loaded_trends);
	fclose(f);

	/* A legacy file without the new field defaults to a full measurement. */
	f = tmpfile();
	assert(f);
	s->rate_only = 0;
	assert(write_file(f, snapshots, names, NULL, 1, NULL) == 0);
	s->rate_only = 1;
	rewind(f);
	FILE *legacy = tmpfile();
	assert(legacy);
	char line[4096];
	int removed = 0;
	while(fgets(line, sizeof(line), f)) {
		if(!strcmp(line, "Lrate_only;\n")) {
			assert(fgets(line, sizeof(line), f));
			removed++;
		} else
			assert(fputs(line, legacy) >= 0);
	}
	assert(!ferror(f) && removed == 1);
	rewind(legacy);
	loaded = NULL;
	loaded_names = NULL;
	loaded_trends = NULL;
	count = 0;
	assert(read_file(legacy, &loaded, &loaded_names, &loaded_trends, &count, NULL) == 0);
	assert(count == 1 && !loaded[0]->rate_only);
	snapshot_destroy(loaded[0]);
	free(loaded_names[0]);
	trend_free(loaded_trends[0]);
	free(loaded);
	free(loaded_names);
	free(loaded_trends);
	fclose(legacy);
	fclose(f);

	/* An unstable period must not replace the previous good result. */
	double period_sigma = algorithm_parameters[ALG_PERIOD_SIGMA];
	algorithm_parameters[ALG_PERIOD_SIGMA] = -1;
	compute_update(&computer);
	assert(s->is_old && s->detection_status == DETECTION_STATUS_UNSTABLE);
	algorithm_parameters[ALG_PERIOD_SIGMA] = period_sigma;

	/* No period makes the last rate-only result stale. */
	memset(pa_buffers, 0, sizeof(pa_buffers));
	compute_update(&computer);
	assert(s->is_old && s->rate_only);

	/* A beat measurement restores the full path when separation succeeds. */
	for(int i = 1000; i < write_pointer; i += 4000)
		pa_buffers[i] = 1;
	algorithm_parameters[ALG_BE_SEARCH] = 10;
	compute_update(&computer);
	compute_results(s);
	assert(!s->is_old && !s->rate_only);

	snapshot_destroy(s);
	for(int i = 0; i < NSTEPS; i++)
		pb_destroy(&buffers[i]);
	pthread_mutex_destroy(&audio_mutex);
	return 0;
}
