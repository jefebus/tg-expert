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

/* Long-term statistics of the live measurement (the "Trend" page).
 *
 * Every TREND_INTERVAL_S seconds the current rate, amplitude and beat error
 * are recorded, if the signal is good, together with the watch position.
 * Periods without signal are left out. The recording starts over when a
 * setting that changes the results changes (bph, lift angle, cal, sample
 * rate, light algorithm), and it stops after TREND_MAX_HOURS hours. With
 * the bph guessed, a short-lived different guess is left out instead.
 * Runs in the GUI thread only. */

#include "tg.h"

struct trend *trend_new(void)
{
	return calloc(1, sizeof(struct trend));
}

void trend_free(struct trend *t)
{
	if(!t) return;
	free(t->samples);
	free(t);
}

/* A frozen copy, for a snapshot tab */
struct trend *trend_clone(const struct trend *t)
{
	if(!t) return NULL;
	struct trend *c = malloc(sizeof(*c));
	if(!c) return NULL;
	*c = *t;
	c->samples = NULL;
	c->capacity = 0;
	if(t->count) {
		c->samples = malloc(t->count * sizeof(*c->samples));
		if(!c->samples) {
			free(c);
			return NULL;
		}
		memcpy(c->samples, t->samples, t->count * sizeof(*c->samples));
		c->capacity = t->count;
	}
	c->frozen = 1;
	c->end_us = t->frozen ? t->end_us : g_get_monotonic_time();
	return c;
}

void trend_reset(struct trend *t)
{
	if(!t) return;
	struct trend_sample *samples = t->samples;
	int capacity = t->capacity;
	gint64 settling_until_us = t->settling_until_us;
	memset(t, 0, sizeof(*t));
	t->samples = samples;	// keep the buffer for the next recording
	t->capacity = capacity;
	t->settling_until_us = settling_until_us;
}

void trend_start_settling(struct trend *t, int seconds)
{
	if(t && !t->frozen)
		t->settling_until_us = seconds > 0 ?
			g_get_monotonic_time() + (gint64)seconds * 1000000 : 0;
}

static void stats_add(struct trend_stats *s, double v)
{
	if(!s->n || v < s->min) s->min = v;
	if(!s->n || v > s->max) s->max = v;
	s->sum += v;
	s->sumsq += v * v;
	s->n++;
}

double trend_stats_mean(const struct trend_stats *s)
{
	return s->n ? s->sum / s->n : 0;
}

double trend_stats_sd(const struct trend_stats *s)
{
	if(s->n < 2) return 0;
	double mean = s->sum / s->n;
	double var = s->sumsq / s->n - mean * mean;
	return var > 0 ? sqrt(var) : 0;
}

void trend_get_plot_ranges(const struct trend *t, double t0, double t1,
	double fixed_rate, double fixed_be, struct trend_plot_ranges *ranges)
{
	double max_rate = 0, max_be = 0;
	ranges->amp_min = 120;
	ranges->amp_max = 360;
	ranges->be_min = 0;
	for(int i = 0; t && i < t->count; i++) {
		const struct trend_sample *sample = &t->samples[i];
		if(sample->amp > 0 && sample->amp < 120)
			ranges->amp_min = 60;
		if(sample->t < t0 || sample->t > t1)
			continue;
		max_rate = fmax(max_rate, fabs(sample->rate));
		max_be = fmax(max_be, sample->be);
	}

	double rate_range = fixed_rate;
	if(rate_range <= 0) {
		static const double rate_ranges[] = {2, 5, 10, 20, 50, 100, 200, 500, 1000, 0};
		rate_range = 1000;
		for(int i = 0; rate_ranges[i]; i++)
			if(max_rate <= rate_ranges[i]) {
				rate_range = rate_ranges[i];
				break;
			}
	}
	ranges->rate_min = -rate_range;
	ranges->rate_max = rate_range;

	ranges->be_max = fixed_be;
	if(ranges->be_max <= 0) {
		static const double be_ranges[] = {1, 2, 5, 10, 20, 50, 100, 0};
		ranges->be_max = 100;
		for(int i = 0; be_ranges[i]; i++)
			if(max_be <= be_ranges[i]) {
				ranges->be_max = be_ranges[i];
				break;
			}
	}
}

/* Seconds since the recording started (until the snapshot, for a frozen copy) */
double trend_elapsed(const struct trend *t)
{
	if(!t || !t->count) return 0;
	gint64 end = t->frozen ? t->end_us : g_get_monotonic_time();
	return (end - t->start_us) / 1e6;
}

/* Fill t, a new trend, with readings from a file and freeze it. The settings
 * (bph, la, cal, nominal_sr, is_light, algorithm_custom, full) must already be set. Returns 1,
 * leaving t empty, if the data is not usable. Missing positions and amplitudes
 * are recorded as unknown and unavailable respectively. */
int trend_set_samples(struct trend *t, const float *tm, const float *rate, const float *amp,
	const float *be, const int *position, const int *settling, int count, double elapsed)
{
	if(count < 0 || count > TREND_MAX_SAMPLES || !isfinite(elapsed) || elapsed < 0)
		return 1;
	if(count && (t->bph < MIN_BPH || t->bph > MAX_BPH || t->la < MIN_LA || t->la > MAX_LA ||
			t->cal < MIN_CAL || t->cal > MAX_CAL || t->nominal_sr <= 0 ||
			(t->algorithm_custom != 0 && t->algorithm_custom != 1)))
		return 1;
	for(int i = 0; i < count; i++) {
		if(!isfinite(tm[i]) || tm[i] < 0 || (i && tm[i] < tm[i-1]) ||
				!isfinite(rate[i]) || fabs(rate[i]) > 9999 ||
				(amp && (!isfinite(amp[i]) || amp[i] < 0 || amp[i] > 360)) ||
				!isfinite(be[i]) || be[i] < 0 || be[i] > 99.9 ||
				(position && (position[i] < POS_NONE || position[i] >= POS_COUNT)) ||
				(settling && settling[i] != 0 && settling[i] != 1))
			return 1;
	}

	struct trend_sample *samples = NULL;
	if(count) {
		samples = malloc(count * sizeof(*samples));
		if(!samples)
			return 1;
	}
	free(t->samples);
	t->samples = samples;
	t->capacity = t->count = count;
	memset(&t->rate, 0, sizeof(t->rate));
	memset(&t->amp, 0, sizeof(t->amp));
	memset(&t->be, 0, sizeof(t->be));
	memset(t->pos_rate, 0, sizeof(t->pos_rate));
	memset(t->pos_amp, 0, sizeof(t->pos_amp));
	memset(t->pos_be, 0, sizeof(t->pos_be));
	for(int i = 0; i < count; i++) {
		struct trend_sample *x = &samples[i];
		x->t = tm[i];
		x->rate = rate[i];
		x->amp = amp ? amp[i] : 0;
		x->be = be[i];
		x->position = position ? position[i] : POS_NONE;
		x->settling = settling ? settling[i] : 0;
		if(x->settling) continue;
		stats_add(&t->rate, x->rate);
		stats_add(&t->be, x->be);
		stats_add(&t->pos_rate[x->position], x->rate);
		stats_add(&t->pos_be[x->position], x->be);
		if(x->amp > 0) {
			stats_add(&t->amp, x->amp);
			stats_add(&t->pos_amp[x->position], x->amp);
		}
	}
	// The recording lasted at least until its last reading
	if(count && elapsed < samples[count-1].t)
		elapsed = samples[count-1].t;
	t->frozen = 1;
	t->start_us = 0;
	t->end_us = (gint64)(elapsed * 1e6);
	t->last_us = count ? (gint64)(samples[count-1].t * 1e6) : 0;
	return 0;
}

void trend_update(struct trend *t, const struct snapshot *s, int position)
{
	if(!t || !s || t->frozen || s->calibrate)
		return;
	if(position < POS_NONE || position >= POS_COUNT)
		position = POS_NONE;

	if(!s->pb || s->is_old || s->rate_only || s->guessed_bph <= 0 ||
			!isfinite(s->rate) || !isfinite(s->be))
		return;	// no good signal: a gap

	gint64 now = g_get_monotonic_time();

	/* Readings taken with other settings are not comparable: the recording
	 * starts over when the user changes lift angle, cal, sample rate or the
	 * light algorithm. Checked only with a good signal: without one,
	 * guessed_bph falls back to DEFAULT_BPH, which is not a change of setting. */
	if(t->count && (s->la != t->la || s->cal != t->cal ||
			s->nominal_sr != t->nominal_sr || s->is_light != t->is_light))
		trend_reset(t);

	if(t->count && s->guessed_bph != t->bph) {
		if(s->bph) {
			trend_reset(t);	// the user chose another bph
		} else {
			/* bph is guessed from each period. While the watch is turned
			 * or the signal is poor, the guess can jump to another rate for
			 * a while: those readings are wrong (the rate is computed for
			 * the wrong bph), so they are left out, like a gap. Only a guess
			 * that stays different for TREND_GUESS_CHANGE_S seconds, such as
			 * a wrong first guess, starts the recording over. */
			if(!t->guess_mismatch_us)
				t->guess_mismatch_us = now;
			if(now - t->guess_mismatch_us < (gint64)TREND_GUESS_CHANGE_S * 1000000)
				return;
			trend_reset(t);
		}
	} else {
		t->guess_mismatch_us = 0;
	}

	if(t->count && now - t->last_us < (gint64)TREND_INTERVAL_S * 1000000)
		return;
	if(t->count >= TREND_MAX_SAMPLES) {
		t->full = 1;
		return;
	}
	if(t->count == t->capacity) {
		int capacity = t->capacity ? 2 * t->capacity : 1024;
		if(capacity > TREND_MAX_SAMPLES) capacity = TREND_MAX_SAMPLES;
		struct trend_sample *samples = realloc(t->samples, capacity * sizeof(*samples));
		if(!samples) return;
		t->samples = samples;
		t->capacity = capacity;
	}

	if(!t->count) {
		t->start_us = now;
		t->bph = s->guessed_bph;
		t->la = s->la;
		t->cal = s->cal;
		t->nominal_sr = s->nominal_sr;
		t->is_light = s->is_light;
		t->algorithm_custom = algorithm_settings_custom(s->is_light);
	}

	struct trend_sample *x = &t->samples[t->count++];
	x->t = (now - t->start_us) / 1e6;
	x->rate = s->rate;
	x->amp = s->amp;	// 0 = not available
	x->be = s->be;
	x->position = position;
	x->settling = now < t->settling_until_us;
	t->last_us = now;

	if(x->settling) return;
	stats_add(&t->rate, s->rate);
	stats_add(&t->be, s->be);
	stats_add(&t->pos_rate[position], s->rate);
	stats_add(&t->pos_be[position], s->be);
	if(s->amp > 0) {
		stats_add(&t->amp, s->amp);
		stats_add(&t->pos_amp[position], s->amp);
	}
}
