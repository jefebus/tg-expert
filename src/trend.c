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
 * rate, light algorithm), and it stops after TREND_MAX_HOURS hours.
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
	memset(t, 0, sizeof(*t));
	t->samples = samples;	// keep the buffer for the next recording
	t->capacity = capacity;
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

/* Seconds since the recording started (until the snapshot, for a frozen copy) */
double trend_elapsed(const struct trend *t)
{
	if(!t || !t->count) return 0;
	gint64 end = t->frozen ? t->end_us : g_get_monotonic_time();
	return (end - t->start_us) / 1e6;
}

/* Fill t, a new trend, with readings from a file and freeze it. The settings
 * (bph, la, cal, nominal_sr, is_light, full) must already be set. Returns 1,
 * leaving t empty, if the data is not usable. */
int trend_set_samples(struct trend *t, const float *tm, const float *rate, const float *amp,
	const float *be, const int *position, int count, double elapsed)
{
	if(count < 0 || count > TREND_MAX_SAMPLES || !isfinite(elapsed) || elapsed < 0)
		return 1;
	if(count && (t->bph < MIN_BPH || t->bph > MAX_BPH || t->la < MIN_LA || t->la > MAX_LA ||
			t->cal < MIN_CAL || t->cal > MAX_CAL || t->nominal_sr <= 0))
		return 1;
	for(int i = 0; i < count; i++) {
		if(!isfinite(tm[i]) || tm[i] < 0 || (i && tm[i] < tm[i-1]) ||
				!isfinite(rate[i]) || fabs(rate[i]) > 9999 ||
				!isfinite(amp[i]) || amp[i] < 0 || amp[i] > 360 ||
				!isfinite(be[i]) || be[i] < 0 || be[i] > 99.9 ||
				position[i] < POS_NONE || position[i] >= POS_COUNT)
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
		x->amp = amp[i];
		x->be = be[i];
		x->position = position[i];
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

	if(!s->pb || s->is_old || s->guessed_bph <= 0 || !isfinite(s->rate) || !isfinite(s->be))
		return;	// no good signal: a gap

	/* Readings taken with other settings are not comparable. Checked only
	 * with a good signal: without one, guessed_bph falls back to DEFAULT_BPH
	 * (e.g. while the watch is turned to another position), which is not a
	 * change of setting. */
	if(t->count && (s->guessed_bph != t->bph || s->la != t->la || s->cal != t->cal ||
			s->nominal_sr != t->nominal_sr || s->is_light != t->is_light))
		trend_reset(t);

	gint64 now = g_get_monotonic_time();
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
	}

	struct trend_sample *x = &t->samples[t->count++];
	x->t = (now - t->start_us) / 1e6;
	x->rate = s->rate;
	x->amp = s->amp;	// 0 = not available
	x->be = s->be;
	x->position = position;
	t->last_us = now;

	stats_add(&t->rate, s->rate);
	stats_add(&t->be, s->be);
	stats_add(&t->pos_rate[position], s->rate);
	stats_add(&t->pos_be[position], s->be);
	if(s->amp > 0) {
		stats_add(&t->amp, s->amp);
		stats_add(&t->pos_amp[position], s->amp);
	}
}
