/*
    tg
    Copyright (C) 2015 Marcello Mamino

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

#include "tg.h"
#include <float.h>

/**
 * @brief Coefficients of a second-order IIR biquad.
 *
 * The same representation is used by the high-pass stage that isolates the
 * short escapement shocks and by the low-pass stage that extracts the acoustic
 * envelope. The recursive filter state is local to run_filter().
 */
struct filter {
	double a0,a1,a2,b1,b2;
};

/**
 * @brief One located acoustic tic or toc event.
 *
 * The sample position locates the event inside the current audio buffer. The
 * tic/toc flag preserves the pallet direction so alternating beats can be
 * displayed, timed, and corrected independently.
 */
struct detected_event {
	int pos;
	unsigned char tictoc;
};

/**
 * @brief Compare two detected acoustic events by their sample position.
 *
 * Sorting the merged event list chronologically preserves the alternating tic/toc sequence
 * used by the trace.
 *
 * @param a Pointer to the first detected_event.
 * @param b Pointer to the second detected_event.
 * @return A negative value if a precedes b, zero if both positions are equal, or a positive value otherwise.
 */
static int detected_event_cmp(const void *a, const void *b)
{
	int x = ((const struct detected_event *)a)->pos;
	int y = ((const struct detected_event *)b)->pos;
	return x<y ? -1 : x>y ? 1 : 0;
}

/**
 * @brief Configure a second-order Butterworth high-pass filter.
 *
 * The high-pass stage suppresses slow case movement, handling noise, and baseline drift while
 * retaining the short unlocking, impulse, locking, and banking transients.
 *
 * @param f Filter coefficient structure to initialize.
 * @param freq Cutoff frequency normalized to the audio sample rate.
 */
static void make_hp(struct filter *f, double freq)
{
	double K = tan(M_PI * freq);
	double norm = 1 / (1 + K * sqrt(2) + K * K);
	f->a0 = 1 * norm;
	f->a1 = -2 * f->a0;
	f->a2 = f->a0;
	f->b1 = 2 * (K * K - 1) * norm;
	f->b2 = (1 - K * sqrt(2) + K * K) * norm;
}

/**
 * @brief Configure a second-order Butterworth low-pass filter.
 *
 * After full-wave rectification, the low-pass stage extracts the slowly varying acoustic
 * envelope of each tic and toc.
 *
 * @param f Filter coefficient structure to initialize.
 * @param freq Cutoff frequency normalized to the audio sample rate.
 */
static void make_lp(struct filter *f, double freq)
{
	double K = tan(M_PI * freq);
	double norm = 1 / (1 + K * sqrt(2) + K * K);
	f->a0 = K * K * norm;
	f->a1 = 2 * f->a0;
	f->a2 = f->a0;
	f->b1 = 2 * (K * K - 1) * norm;
	f->b2 = (1 - K * sqrt(2) + K * K) * norm;
}

/**
 * @brief Apply one recursive biquad to a sample buffer.
 *
 * The direct-form state variables implement the configured high-pass or low-pass response
 * without changing sample timing.
 *
 * @param f Filter coefficients.
 * @param buff Audio samples processed in place.
 * @param size Number of samples to process.
 */
static void run_filter(struct filter *f, float *buff, int size)
{
	int i;
	double z1 = 0, z2 = 0;
	for(i=0; i<size; i++) {
		double in = buff[i];
		double out = in * f->a0 + z1;
		z1 = in * f->a1 + z2 - f->b1 * out;
		z2 = in * f->a2 - f->b2 * out;
		buff[i] = out;
	}
}

/**
 * @brief Allocate the signal-processing buffers and FFTW plans.
 *
 * The context stores raw audio, the rectified envelope, autocorrelation, folded tic/toc
 * waveforms, template correlations, event timestamps, and optional diagnostic data.
 *
 * @param b Processing context with sample_count and sample_rate already set.
 */
void setup_buffers(struct processing_buffers *b)
{
	b->samples = fftwf_malloc(2 * b->sample_count * sizeof(float));
	b->samples_sc = malloc(2 * b->sample_count * sizeof(float));
	b->waveform = malloc(2 * b->sample_rate * sizeof(float));
	b->waveform_sc = malloc(2 * b->sample_rate * sizeof(float));
	b->fft = fftwf_malloc((b->sample_count + 1) * sizeof(fftwf_complex));
	b->sc_fft = fftwf_malloc((b->sample_count + 1) * sizeof(fftwf_complex));
	b->tic_wf = fftwf_malloc(b->sample_rate * sizeof(float));
	b->slice_wf = fftwf_malloc(b->sample_rate * sizeof(float));
	b->tic_fft = fftwf_malloc((b->sample_rate/2 + 1) * sizeof(fftwf_complex));
	b->slice_fft = fftwf_malloc((b->sample_rate/2 + 1) * sizeof(fftwf_complex));
	b->tic_c = malloc(2 * b->sample_count * sizeof(float));
	b->plan_a = fftwf_plan_dft_r2c_1d(2 * b->sample_count, b->samples, b->fft, FFTW_ESTIMATE);
	b->plan_b = fftwf_plan_dft_c2r_1d(2 * b->sample_count, b->sc_fft, b->samples_sc, FFTW_ESTIMATE);
	b->plan_c = fftwf_plan_dft_r2c_1d(2 * b->sample_rate, b->waveform, b->sc_fft, FFTW_ESTIMATE);
	b->plan_d = fftwf_plan_dft_c2r_1d(2 * b->sample_rate, b->sc_fft, b->waveform_sc, FFTW_ESTIMATE);
	b->plan_e = fftwf_plan_dft_r2c_1d(b->sample_rate, b->tic_wf, b->tic_fft, FFTW_ESTIMATE);
	b->plan_f = fftwf_plan_dft_r2c_1d(b->sample_rate, b->slice_wf, b->slice_fft, FFTW_ESTIMATE);
	b->plan_g = fftwf_plan_dft_c2r_1d(b->sample_rate, b->slice_fft, b->slice_wf, FFTW_ESTIMATE);
	b->hpf = malloc(sizeof(struct filter));
	make_hp(b->hpf,algorithm_parameters[ALG_FILTER]/b->sample_rate);
	b->lpf = malloc(sizeof(struct filter));
	make_lp(b->lpf,algorithm_parameters[ALG_FILTER]/b->sample_rate);
	b->events = malloc(EVENTS_MAX * sizeof(uint64_t));
	b->events_tictoc = malloc(EVENTS_MAX * sizeof(unsigned char));
	b->ready = 0;
#ifdef DEBUG
	b->debug_size = b->sample_count;
	b->debug = fftwf_malloc(b->debug_size * sizeof(float));
#endif
}

/**
 * @brief Release a complete processing context.
 *
 * All buffers, filters, event arrays, and FFTW plans allocated by setup_buffers() are released
 * here.
 *
 * @param b Processing context to destroy.
 */
void pb_destroy(struct processing_buffers *b)
{
	fftwf_free(b->samples);
	free(b->samples_sc);
	free(b->waveform);
	free(b->waveform_sc);
	fftwf_free(b->fft);
	fftwf_free(b->sc_fft);
	fftwf_free(b->tic_wf);
	fftwf_free(b->slice_wf);
	fftwf_free(b->tic_fft);
	fftwf_free(b->slice_fft);
	free(b->tic_c);
	fftwf_destroy_plan(b->plan_a);
	fftwf_destroy_plan(b->plan_b);
	fftwf_destroy_plan(b->plan_c);
	fftwf_destroy_plan(b->plan_d);
	fftwf_destroy_plan(b->plan_e);
	fftwf_destroy_plan(b->plan_f);
	fftwf_destroy_plan(b->plan_g);
	free(b->hpf);
	free(b->lpf);
	free(b->events);
	free(b->events_tictoc);
#ifdef DEBUG
	fftwf_free(b->debug);
#endif
}

struct processing_buffers *pb_clone(struct processing_buffers *p)
{
	struct processing_buffers *new = malloc(sizeof(struct processing_buffers));
	new->sample_count = ceil(p->period);
	new->waveform = malloc(new->sample_count * sizeof(float));
	memcpy(new->waveform, p->waveform, new->sample_count * sizeof(float));
	if(p->events) {
		new->events = malloc(EVENTS_MAX * sizeof(uint64_t));
		memcpy(new->events, p->events, EVENTS_MAX * sizeof(uint64_t));
	} else
		new->events = NULL;
	if(p->events_tictoc) {
		new->events_tictoc = malloc(EVENTS_MAX * sizeof(unsigned char));
		memcpy(new->events_tictoc, p->events_tictoc, EVENTS_MAX * sizeof(unsigned char));
	} else
		new->events_tictoc = NULL;

#ifdef DEBUG
	new->debug_size = p->debug_size;
	if(p->debug) {
		new->debug = malloc(new->debug_size * sizeof(float));
		memcpy(new->debug, p->debug, new->debug_size * sizeof(float));
	} else
		new->debug = NULL;
#endif

	new->sample_rate = p->sample_rate;
	new->period = p->period;
	new->sigma = p->sigma;
	new->be = p->be;
	new->waveform_max = p->waveform_max;
	new->tic_pulse = p->tic_pulse;
	new->toc_pulse = p->toc_pulse;
	new->amp = p->amp;
	new->tic = p->tic;
	new->toc = p->toc;
	new->ready = p->ready;
	new->timestamp = p->timestamp;
	return new;
}

/**
 * @brief Release a snapshot created by pb_clone().
 *
 * Only the fields allocated for the lightweight snapshot are released.
 *
 * @param p Snapshot to destroy.
 */
void pb_destroy_clone(struct processing_buffers *p)
{
	free(p->waveform);
	free(p->events);
	free(p->events_tictoc);
#ifdef DEBUG
	free(p->debug);
#endif
	free(p);
}

/**
 * @brief Find the maximum sample in a half-open interval.
 *
 * Peak level and position are used throughout period detection, waveform alignment, threshold
 * estimation, and calibration.
 *
 * @param v Input vector.
 * @param a Inclusive first index.
 * @param b Exclusive last index.
 * @param i_max Optional destination for the maximum index.
 * @return Maximum value in the interval.
 */
static float vmax(float *v, int a, int b, int *i_max)
{
	float max = v[a];
	if(i_max) *i_max = a;
	int i;
	for(i = a+1; i < b; i++) {
		if(v[i] > max) {
			max = v[i];
			if(i_max) *i_max = i;
		}
	}
	return max;
}

/* Choose pivot: use median of first, last, middle elements */
/**
 * @brief Select a median-of-three pivot for quickselect.
 *
 * The pivot strategy avoids poor partitions for ordered or nearly ordered signal-level
 * samples.
 *
 * @param x Input values.
 * @param l Left index.
 * @param r Right index.
 * @return Index of the selected pivot.
 */
static int pivot(const float *x, int l, int r)
{
	const int m = (l+r)/2;
	if (x[l] < x[r]) {
		if (x[m] <= x[l])
			return l;
		if (x[m] >= x[r])
			return r;
		return m;
	} else {
		if (x[m] <= x[r])
			return r;
		if (x[m] >= x[l])
			return l;
		return m;
	}
}

/** Partition list in ascending order at rank k.
 *
 * Re-orders the list so that the k'th largest values are in the first k
 * elements, the (k+1)'th largest value is x[k], and all the values smaller than
 * x[k] follow it.  Thus, x[0 .. k-1] >= x[k] >= x[k+1 .. n-1].  Note that
 * x[0 .. k-1] and x[k+1 .. n-1] are not sorted, so this is like, but not the
 * same as, a quicksort.  If we avoid worst case behavior in choice of pivots,
 * it's O(n) rather than O(n log n) like quicksort.
 *
 * This uses the "median of three" pivot strategy.  I also tried simpler ones,
 * like always use the last element, but this benchmarked faster.
 *
 * @param[in,out] x The values to partition.
 * @param[in] n The number of values.
 * @param[in] k The index to partition at.
 * @returns Nothing, but x will be partitioned.
 */
static void quickselect(float* x, int n, int k)
{
	int l = 0, r = n - 1;
	while (1) {
		if (l == r)
			return;
		if (r - l == 1) {
			/* Only two values left, put them in decending order */
			if (x[l] < x[r]) {
				float t = x[r];
				x[r] = x[l];
				x[l] = t;
			}
			return;
		}

		int p = pivot(x, l, r);
		const float pv = x[p];
		x[p] = x[r];
		p = l;
		/* Partition:  Everything greater than pv at start, then pv at
		 * position p, then everything smaller.  */
		int i;
		for (i = l; i < r; i++) {
			if (x[i] > pv) {
				float t = x[i];
				x[i] = x[p];
				x[p++] = t;
			}
		}
		/* Swap pivot value into position p */
		x[r] = x[p];
		x[p] = pv;
		if (k == p)
			return;
		if (k < p)
			r = p - 1;
		else
			l = p + 1;
	};
}

/**
 * @brief Suppress sustained high-energy interference in the recording.
 *
 * Energy is integrated over configurable windows and summarized in sections. Regions
 * greatly exceeding the median section level are cleared before escapement periodicity is
 * measured.
 *
 * @param p Processing context containing raw audio and scratch buffers.
 */
static void noise_suppressor(struct processing_buffers *p)
{
	float *a = p->samples_sc;
	float *b = p->samples_sc + p->sample_count;
	/* Integrate squared signal energy over the configured window. */
	int window = (int)(p->sample_rate * algorithm_parameters[ALG_NOISE_WINDOW] / 1000);
	int i;

	for(i = 0; i < p->sample_count; i++)
		a[i] = p->samples[i] * p->samples[i];

	/* Maintain the running window energy without recomputing every sum. */
	double r_av = 0;
	for(i = 0; i < window; i++)
		r_av += a[i];
	for(i = 0;; i++) {
		b[i] = r_av;
		if(i + window == p->sample_count) break;
		r_av += a[i + window] - a[i];
	}

	int m = p->sample_count - window + 1;
	/* Summarize each section by its strongest local energy. */
	int step = (int)(p->sample_rate * algorithm_parameters[ALG_NOISE_SEGMENT] / 1000);
	int j = 0;
	for(i = 0; i + step - 1 < m; i += step)
		a[j++] = vmax(b, i, i+step, NULL);
	/* Use the median section maximum as a robust background reference. */
	quickselect(a, j, j/2);
	float k = a[j/2];

	for(i = 0; i < p->sample_count; i++) {
		int j = i - window / 2;
		j = j < 0 ? 0 : j > p->sample_count - window ? p->sample_count - window : j;
		/* Remove samples exceeding the configured multiple of the reference level. */
		if(b[j] > algorithm_parameters[ALG_NOISE_THRESHOLD]*k) p->samples[i] = 0;
	}
}

/**
 * @brief Convert raw audio into an autocorrelated acoustic envelope.
 *
 * The chain applies high-pass filtering, optional interference suppression, full-wave
 * rectification, low-pass envelope extraction, DC removal, edge tapering, and FFT
 * autocorrelation.
 *
 * @param b Processing context.
 * @param run_noise_suppressor Nonzero to suppress sustained interference.
 */
static void prepare_data(struct processing_buffers *b, int run_noise_suppressor)
{
	int i;

	/* Zero-padding prevents circular wraparound from contaminating the autocorrelation. */
	memset(b->samples + b->sample_count, 0, b->sample_count * sizeof(float));
	/* Retain short escapement impacts while rejecting slow mechanical movement. */
	run_filter(b->hpf, b->samples, b->sample_count);
	if(run_noise_suppressor) noise_suppressor(b);

	for(i=0; i < b->sample_count; i++)
		/* Full-wave rectification converts bipolar microphone vibration into a positive envelope input. */
		b->samples[i] = fabs(b->samples[i]);

	/* Smooth the rectified audio into the composite tic/toc energy envelope. */
	run_filter(b->lpf, b->samples, b->sample_count);

	double average = 0;
	for(i=0; i < b->sample_count; i++)
		average += b->samples[i];
	/* Remove the envelope DC level before autocorrelation. */
	average /= b->sample_count;
	for(i=0; i < b->sample_count; i++)
		b->samples[i] -= average;

	int taper = (int)(b->sample_rate * algorithm_parameters[ALG_TAPER] / 1000);
	for(i=0; i < taper; i++) {
		/* Apply a raised-cosine taper to reduce FFT edge discontinuities. */
		double k = ( 1 - cos(i*M_PI/taper) ) / 2;
		b->samples[i] *= k;
		b->samples[b->sample_count - i - 1] *= k;
	}

	fftwf_execute(b->plan_a);
	for(i=0; i < b->sample_count+1; i++)
			/* The power spectrum transforms back into the linear autocorrelation sequence. */
			b->sc_fft[i] = b->fft[i] * conj(b->fft[i]);
	fftwf_execute(b->plan_b);

#ifdef DEBUG
	memcpy(b->debug, b->samples_sc, b->sample_count * sizeof(float));
#endif
}

/**
 * @brief Locate a reliable positive peak inside a bounded interval.
 *
 * The candidate must rise above the median on both sides and must not belong to an excessively
 * fragmented group of lobes.
 *
 * @param buff Correlation or envelope buffer.
 * @param a Inclusive first index.
 * @param b Inclusive last index.
 * @return Peak index, or -1 if no reliable peak is found.
 */
static int peak_detector(float *buff, int a, int b)
{
	int i_max;
	double max = vmax(buff, a, b+1, &i_max);
	if(max <= 0) return -1;

	int i;
	float v[b-a+1];
	memcpy(v, buff + a, sizeof(v));
	quickselect(v, b-a+1, (b-a+1)/2);
	float med = v[(b-a+1)/2];

	for(i=a+1; i<i_max; i++)
		if(buff[i] <= med) break;
	if(i==i_max) return -1;
	for(i=i_max+1; i<=b; i++)
		if(buff[i] <= med) break;
	if(i==b+1) return -1;

	int cnt = 0, down = 1;
	for(i=a+1; i<=b; i++) {
		if(buff[i] > (max + med) / 2) {
			cnt += down;
			down = 0;
		}
		if(buff[i] < med)
			down = 1;
	}
	debug("max = %f med = %f cnt = %d\n",max,med,cnt);
	if(cnt > algorithm_parameters[ALG_LOBES])
		return -1;
	return i_max;
}

/**
 * @brief Estimate the interval between equivalent escapement events.
 *
 * The search examines autocorrelation peaks over plausible mechanical-watch periods, checks
 * subharmonics, and rejects double triggering caused by alternating tic/toc structure.
 *
 * @param p Processing context containing the autocorrelation.
 * @return Estimated period in samples, or -1 on failure.
 */
static double estimate_period(struct processing_buffers *p)
{
	int first_estimate;
	int tolerance = (int)(p->sample_rate * algorithm_parameters[ALG_GUESS_SEARCH] / 1000);
	/* Restrict the initial search to plausible mechanical-watch beat periods. */
	vmax(p->samples_sc, p->sample_rate / 12, p->sample_rate, &first_estimate);
	first_estimate = peak_detector(p->samples_sc,
			/* Restrict the initial search to plausible mechanical-watch beat periods. */
			fmax(p->sample_rate / 12, first_estimate - p->sample_rate / 12),
			/* Restrict the initial search to plausible mechanical-watch beat periods. */
			first_estimate + p->sample_rate / 12);
	if(first_estimate == -1) {
		debug("no candidate period\n");
		return -1;
	}
	int estimate = first_estimate;
	int factor = 1;
	int fct;
	/* Restrict the initial search to plausible mechanical-watch beat periods. */
	for(fct = 2; first_estimate / fct > p->sample_rate / 12; fct++) {
		int new_estimate = peak_detector(p->samples_sc,
					first_estimate / fct - tolerance,
					first_estimate / fct + tolerance);
		if(new_estimate > -1 && p->samples_sc[new_estimate] > algorithm_parameters[ALG_HARMONIC] / 100 * p->samples_sc[first_estimate]) {
			estimate = new_estimate;
			factor = fct;
		}
	}
	/* Inspect the alternating tic/toc structure at one and a half candidate periods. */
	int a = estimate*3/2 - tolerance;
	/* Inspect the alternating tic/toc structure at one and a half candidate periods. */
	int b = estimate*3/2 + tolerance;
	double max = vmax(p->samples_sc, a, b+1, NULL);
	if(max < algorithm_parameters[ALG_DOUBLE_TRIGGER] / 100 * p->samples_sc[estimate]) {
		if(first_estimate * 2 / factor < p->sample_rate ) {
			/* Fall back to a doubled interval when each acoustic beat has been counted twice. */
			debug("double triggered\n");
			return peak_detector(p->samples_sc,
					first_estimate * 2 / factor - tolerance,
					first_estimate * 2 / factor + tolerance);
		} else {
			debug("period rejected (immense beat error?)\n");
			return -1;
		}
	} else return estimate;
}

/**
 * @brief Refine the period between consecutive equivalent events.
 *
 * For a known beat rate, the initial search is centered on 7200/BPH seconds, corresponding to
 * tic-to-tic or toc-to-toc. Several correlation cycles are averaged to improve rate
 * resolution.
 *
 * @param b Processing context.
 * @param bph Nominal beats per hour, or zero for automatic estimation.
 * @return Zero on success; nonzero if the period cannot be established.
 */
static int compute_period(struct processing_buffers *b, int bph)
{
	double estimate;
	int tolerance = (int)(b->sample_rate * algorithm_parameters[ALG_GUESS_SEARCH] / 1000);
	if(bph)
		estimate = peak_detector(b->samples_sc,
				/* Convert nominal beats per hour into the expected tic-to-tic or toc-to-toc interval in samples. */
				7200 * b->sample_rate / bph - tolerance,
				/* Convert nominal beats per hour into the expected tic-to-tic or toc-to-toc interval in samples. */
				7200 * b->sample_rate / bph + tolerance);
	else
		estimate = estimate_period(b);
	if(estimate == -1) {
		debug("failed to estimate period\n");
		return 1;
	}
	/* Allow the configured search tolerance around each predicted correlation peak. */
	double delta = b->sample_rate * algorithm_parameters[ALG_PERIOD_REFINE] / 1000;
	double new_estimate = estimate;
	double sum = 0;
	double sq_sum = 0;
	int count = 0;
	int cycle = 1;
	/* Refine the period from successively more distant autocorrelation cycles. */
	for(;;) {
		int inf = floor(new_estimate * cycle - delta);
		int sup = ceil(new_estimate * cycle + delta);
		if(sup > b->sample_count * 2 / 3)
			break;
		new_estimate = peak_detector(b->samples_sc,inf,sup);
		if(new_estimate == -1) {
			debug("cycle = %d peak not found\n",cycle);
			return 1;
		}
		new_estimate /= cycle;
		if(new_estimate < estimate - delta || new_estimate > estimate + delta) {
			debug("cycle = %d new_estimate = %f invalid peak\n",cycle,new_estimate/b->sample_rate);
			return 1;
		} else
			debug("cycle = %d new_estimate = %f\n",cycle,new_estimate/b->sample_rate);
		if(inf > b->sample_count / 3) {
			sum += new_estimate;
			/* Accumulate the second moment used to report period dispersion. */
			sq_sum += new_estimate * new_estimate;
			count++;
		}
		cycle++;
	}
	if(count > 0) estimate = sum / count;
	b->period = estimate;
	if(count > 1)
		/* Store timing instability as the standard deviation of the period estimate. */
		b->sigma = sqrt((sq_sum - count * estimate * estimate)/ (count-1));
	else
		/* Store timing instability as the standard deviation of the period estimate. */
		b->sigma = b->period;
	return 0;
}

/** Return the mean with the greatest value excluded.
 *
 * Finds the mean of the values in x with the greatest value excluded from the
 * list.  If n <= 5, then this is the same as a the mean of the lower four
 * quintiles, since the upper quintile is 1 value.
 *
 * @param[in] x The values.
 * @param[in] n Number of values.
 * @returns The mean with the greatest value excluded.
 */
/**
 * @brief Calculate a mean after excluding the greatest sample.
 *
 * For very small bins, this is the efficient equivalent of discarding the upper quintile.
 *
 * @param x Input values.
 * @param n Number of values.
 * @return Mean with the greatest value excluded.
 */
static float mean_less_greatest(const float *x, int n)
{
	float greatest = x[0], sum = x[0];
	int i;
	for(i = 1; i < n; i++) {
		sum += x[i];
		if (x[i] > greatest) greatest = x[i];
	}
	return (sum - greatest) / (n-1);
}

/** Return the mean with the greatest two values excluded.
 *
 * Finds the mean of the values in x with the greatest and 2nd greatest values
 * excluded from the list.  If 5 < n <= 10, then this is the same as a the mean
 * of the lower four quintiles, since the upper quintile is 2 values.
 *
 * @param[in] x The values.
 * @param[in] n Number of values.
 * @returns The mean with the greatest two values excluded.
 */
/**
 * @brief Calculate a mean after excluding the two greatest samples.
 *
 * This small-bin implementation supports robust folding of repeated escapement cycles.
 *
 * @param x Input values.
 * @param n Number of values.
 * @return Mean with the two greatest values excluded.
 */
static float mean_less_two_greatest(const float *x, int n)
{
	float sum = x[0], greatest[2] = {x[0], -FLT_MAX};
	int i;
	for(i = 1; i < n; i++) {
		sum += x[i];
		if (x[i] > greatest[0]) {
			greatest[1] = greatest[0];
			greatest[0] = x[i];
		} else if (x[i] > greatest[1]) {
			greatest[1] = x[i];
		}
	}
	return (sum - greatest[0] - greatest[1]) / (n-2);
}

/**
 * @brief Calculate the mean after removing the configured percentage of highest values.
 *
 * Discarding high values prevents occasional impacts, speech, or handling noise
 * from becoming part of the representative tic/toc waveform.
 *
 * @param x Values; ordering may be modified.
 * @param n Number of values.
 * @return Trimmed mean.
 */
static float tmean(float *x, int n)
{
	/* Use a hybrid approach, where for small lists a specialized function
	 * is used, since only a few items will be looked at and just one or two
	 * excluded.  So the simple and linear processing is faster.  For larger
	 * lists, use a quickselect based algorithm with average case O(n)
	 * performance rather than the O(n^2) that the first algorithm has.
	 *
	 * */
	int excluded = (n * (int)algorithm_parameters[ALG_TRIM] + 99) / 100;
	if(n <= 1) return x[0];
	if(excluded >= n) excluded = n - 1;
	if (excluded == 1)
		return mean_less_greatest(x, n);
	else if (excluded == 2)
		return mean_less_two_greatest(x, n);
	else {
		int k = excluded;
		quickselect(x, n, k);
		/* Now x[0] to x[k-1] should be the values to exclude */

		double sum = 0;
		int i;
		for(i = k; i < n; i++) sum += x[i];
		return sum / (n-k);
	}
}

/**
 * @brief Determine the folding phase of the periodic signal.
 *
 * Samples separated by one candidate period are averaged, and the fundamental Fourier
 * component supplies a stable alignment for repeated acoustic events.
 *
 * @param p Processing context.
 * @param period Folding period in samples.
 */
static void compute_phase(struct processing_buffers *p, double period)
{
	int i;
	double x = 0, y = 0;
	for(i = 0; i < period; i++) {
		int j;
		/* Start one phase bin of the folded periodic waveform. */
		p->waveform[i] = 0;
		for(j=0;;j++) {
			int n = round(i + j * period);
			if(n >= p->sample_count) break;
			/* Average samples occupying the same phase in successive periods. */
			p->waveform[i] += p->samples[n];
		}
		p->waveform[i] /= j;
	}
	for(i=0; i<period; i++) {
		double a = i * 2 * M_PI / period;
		x += p->waveform[i] * cos(a);
		y += p->waveform[i] * sin(a);
	}
	/* Convert the fundamental Fourier vector into the folding offset. */
	p->phase = period * (M_PI + atan2(y,x)) / (2 * M_PI);
}

/**
 * @brief Fold repeated beats into one representative waveform.
 *
 * For every phase position, corresponding samples from multiple cycles are combined with a
 * trimmed mean. The result preserves repeatable unlocking, impulse, and locking/banking
 * features while rejecting isolated disturbances.
 *
 * @param p Processing context.
 * @param wf_size Number of samples in the folded period.
 */
static void compute_waveform(struct processing_buffers *p, int wf_size)
{
	int i;
	for(i=0; i<2*p->sample_rate; i++)
		p->waveform[i] = 0;
	for(i=0; i < wf_size; i++) {
		/* Collect all samples occupying one phase position across repeated beats. */
		float bin[(int)ceil(1 + p->sample_count / wf_size)];
		int j;
		double k = fmod(i+p->phase,wf_size);
		for(j=0;;j++) {
			int n = round(k+j*wf_size);
			if(n >= p->sample_count) break;
			bin[j] = p->samples[n];
		}
		/* Use the lower-four-quintile mean to reject occasional non-repeating impacts. */
		p->waveform[i] = tmean(bin,j);
	}

	int step = ceil(wf_size / 100);
	for(i=0; i * step < wf_size; i++)
		p->waveform_sc[i] = p->waveform[i * step];
	quickselect(p->waveform_sc, i, i/2);
	/* Estimate the folded waveform baseline from a sparse median. */
	double nl = p->waveform_sc[i/2];
	for(i=0; i<wf_size; i++)
		p->waveform[i] -= nl;

	/* Retain the dominant folded pulse for display and calibration. */
	p->waveform_max = vmax(p->waveform, 0, wf_size, &p->waveform_max_i);
}

/**
 * @brief Build and autocorrelate the representative tic/toc cycle.
 *
 * Phase is first aligned at half the equivalent-event period so that both alternating pallet
 * actions remain available for beat-error measurement.
 *
 * @param p Processing context.
 */
static void prepare_waveform(struct processing_buffers *p)
{
	compute_phase(p,p->period/2);
	compute_waveform(p,ceil(p->period));

	int i;
	fftwf_execute(p->plan_c);
	for(i=0; i < p->sample_rate+1; i++)
			p->sc_fft[i] *= conj(p->sc_fft[i]);
	fftwf_execute(p->plan_d);
}

/**
 * @brief Build a one-second folded waveform for calibration.
 *
 * The calibration input uses a fixed 7200 BPH reference, whose equivalent-event period is one
 * second.
 *
 * @param p Processing context.
 */
static void prepare_waveform_cal(struct processing_buffers *p)
{
	compute_phase(p,p->sample_rate);
	compute_waveform(p,p->sample_rate);
}

/**
 * @brief Generate a peak-following smoothed envelope.
 *
 * The decaying maximum follower retains short escapement shocks while reducing high-frequency
 * ripple, making acoustic lift-time boundaries easier to detect.
 *
 * @param in Input waveform.
 * @param out Output envelope.
 * @param window Smoothing window in samples.
 * @param size Number of input samples.
 */
static void smooth(float *in, float *out, int window, int size)
{
	int i;
	double k = 1 - (1. / window);
	double r_av = 0;
	double u = 0;
	for(i=0; i < window; i++) {
		u *= k;
		float x = in[i];
		if(x > u) u = x;
		r_av += u;
	}
	double w = 0;
	for(i=0; i + window < size; i++) {
		out[i] = r_av;
		u *= k;
		w *= k;
		float x = in[i+window];
		float y = in[i];
		if(x > u) u = x;
		if(y > w) w = y;
		r_av += u - w;
	}
}

/**
 * @brief Locate tic and toc and calculate the preliminary beat error.
 *
 * The opposite pallet action is expected near half the tic-to-tic period. The deviation of the
 * measured tic-to-toc interval from that midpoint provides the initial beat-error estimate.
 *
 * @param p Processing context containing the folded waveform and its autocorrelation.
 * @return Zero on success; nonzero if tic and toc cannot be separated.
 */
static int compute_parameters(struct processing_buffers *p)
{
	int tolerance = (int)(p->sample_rate * algorithm_parameters[ALG_BE_SEARCH] / 1000);
	int low = floor(p->period / 2) - tolerance;
	int high = floor(p->period / 2) + tolerance;
	if(low < 0 || high >= 2 * p->sample_rate) return 1;
	/* Search near half a period, where the opposite pallet action should occur. */
	int tic_to_toc = peak_detector(p->waveform_sc, low, high);
	if(tic_to_toc < 0) {
		debug("beat error = ---\n");
		return 1;
	} else {
		/* Express the initial asymmetry between the two semiperiods in samples. */
		p->be = p->period/2 - tic_to_toc;
		debug("beat error = %.1f\n",fabs(p->be)*1000/p->sample_rate);
	}

	int wf_size = ceil(p->period);
	int window = MAX(1, (int)(p->sample_rate * algorithm_parameters[ALG_TIC_SMOOTH] / 1000));
	if(tic_to_toc >= wf_size || wf_size - tic_to_toc <= window) return 1;
	float fold_wf[wf_size - tic_to_toc];
	int i;
	for(i = 0; i < wf_size - tic_to_toc; i++)
		/* Superimpose tic and shifted toc to obtain a stronger common timing reference. */
		fold_wf[i] = p->waveform[i] + p->waveform[i+tic_to_toc];
	float smooth_wf[wf_size - tic_to_toc - window];
	smooth(fold_wf, smooth_wf, window, wf_size - tic_to_toc);
	int max_i;
	float max = vmax(smooth_wf, 0, wf_size - tic_to_toc - window, &max_i);
	if(max <= 0) return 1;
	/* Assign the maximum of the common envelope as the first event reference. */
	p->tic = max_i;
	p->toc = p->tic + tic_to_toc;

	/* Convert folded phase into the absolute timestamp of the most recent tic. */
	double phase = p->timestamp - p->last_tic;
	double apparent_phase = p->sample_count - (p->phase + p->tic);
	/* Resolve which folded reference is tic by comparison with the previous absolute event. */
	double shift = fmod(apparent_phase - phase, p->period);
	if(shift < 0) shift += p->period;
	debug("shift = %.3f\n",shift / p->period);

	if(fabs(shift - p->period/2) < p->period/4) {
		p->last_toc = p->timestamp - (uint64_t)round(fmod(apparent_phase, p->period));
		int t = p->tic;
		/* Assign the maximum of the common envelope as the first event reference. */
		p->tic = p->toc;
		p->toc = t;
		apparent_phase = p->sample_count - (p->phase + p->tic);
	} else
		p->last_toc = p->timestamp - (uint64_t)round(fmod(p->sample_count - (p->phase + p->toc), p->period));

	/* Convert folded phase into the absolute timestamp of the most recent tic. */
	p->last_tic = p->timestamp - (uint64_t)round(fmod(apparent_phase, p->period));

	return 0;
}

/**
 * @brief Locate repeated events of one tic/toc polarity by template correlation.
 *
 * A half-cycle template is correlated against successive audio slices, and each predicted
 * event is searched within a narrow window around the period estimate.
 *
 * @param events Output event candidates.
 * @param p Processing context.
 * @param waveform Tic or toc template.
 * @param last Predicted position of the latest event.
 * @param offset Template timing correction.
 * @param count Number of events to locate.
 * @param tictoc Polarity label stored with each event.
 */
static void do_locate_events(struct detected_event *events, struct processing_buffers *p, float *waveform, int last, int offset, int count, unsigned char tictoc)
{
	int i;
	double tolerance = p->sample_rate * algorithm_parameters[ALG_EVENT_SEARCH] / 1000;
	/* Zero-pad the selected half-cycle template to one second. */
	memset(p->tic_wf, 0, p->sample_rate * sizeof(float));
	for(i=0; i<floor(p->period)/2; i++)
		p->tic_wf[i] = waveform[i];
	fftwf_execute(p->plan_e);

	int s;
	memset(p->tic_c, 0, 2 * p->sample_count * sizeof(float));
	for(s = p->sample_count - p->sample_rate/2; s >= 0; s -= p->sample_rate/2) {
		for(i=0; i < p->sample_rate; i++)
			p->slice_wf[i] = p->samples[i+s];
		fftwf_execute(p->plan_f);
		for(i=0; i < p->sample_rate/2+1; i++)
			/* Form FFT cross-correlation between the template and each audio slice. */
			p->slice_fft[i] *= conj(p->tic_fft[i]);
		fftwf_execute(p->plan_g);
		for(i=0; i < p->sample_rate/2; i++)
			p->tic_c[i+s] = p->slice_wf[i];
		/* Search each predicted event within the configured tolerance. */
		if(s < last - offset - (count-1)*p->period - tolerance) break;
	}

	for(i=0; i<count; i++) {
		int a = round(last - offset - i*p->period - tolerance);
		int b = round(last - offset - i*p->period + tolerance);
		/* Attach the tic/toc polarity to the located event. */
		events[i].tictoc = tictoc;
		if(a < 0 || b >= p->sample_count - p->period/2)
			events[i].pos = -1;
		else {
			int peak = peak_detector(p->tic_c,a,b);
			events[i].pos = peak >= 0 ? offset + peak : -1;
		}
	}
}

/**
 * @brief Locate and merge all tic and toc events.
 *
 * Separate templates are corrected by the measured tic and toc lift times, correlated with the
 * recording, sorted chronologically, and converted to absolute sample timestamps.
 *
 * @param p Processing context.
 */
static void locate_events(struct processing_buffers *p)
{
	/* Determine how many periods overlap the requested event interval. */
	int count = 1 + ceil((p->timestamp - p->events_from) / p->period);
	if(count <= 0 || 2*count >= EVENTS_MAX) {
		p->events[0] = 0;
		return;
	}

	struct detected_event events[2*count];
	int half = p->tic < p->period/2 ? 0 : round(p->period / 2);
	/* Correct the template phase for unequal tic and toc lift times. */
	int offset = p->tic - half - (p->tic_pulse - p->toc_pulse) / 2;
	do_locate_events(events, p, p->waveform + half, (int)(p->last_tic + p->sample_count - p->timestamp), offset, count, 1);
	half = p->toc < p->period/2 ? 0 : round(p->period / 2);
	offset = p->toc - half - (p->toc_pulse - p->tic_pulse) / 2;
	do_locate_events(events+count, p, p->waveform + half, (int)(p->last_toc + p->sample_count - p->timestamp), offset, count, 0);
	/* Merge both pallet directions into chronological order. */
	qsort(events, 2*count, sizeof(struct detected_event), detected_event_cmp);

	int i,j;
	for(i=0, j=0; i < 2*count; i++) {
		uint64_t detected_pos = (uint64_t)events[i].pos;
		if(events[i].pos < 0 ||
				detected_pos + p->timestamp < (uint64_t)p->sample_count ||
				detected_pos + p->timestamp - (uint64_t)p->sample_count < p->events_from)
			continue;
		/* Convert buffer-relative positions into absolute stream timestamps. */
		p->events[j] = detected_pos + p->timestamp - (uint64_t)p->sample_count;
		p->events_tictoc[j] = events[i].tictoc;
		j++;
	}
	/* Convert buffer-relative positions into absolute stream timestamps. */
	p->events[j] = 0;
	if(j < EVENTS_MAX)
		p->events_tictoc[j] = 0;
}

/**
 * @brief Estimate balance amplitude from acoustic lift time.
 *
 * For each tic and toc, the algorithm finds the detectable onset of the composite lift sound
 * before the main event reference. The measured lift time and configured lift angle are then
 * related through sinusoidal balance motion.
 *
 * @param p Processing context.
 * @param la Movement lift angle in degrees.
 */
static void compute_amplitude(struct processing_buffers *p, double la)
{
	int i,j,k;

	int wf_size = ceil(p->period);
	/* Smooth the lift-sound envelope using the configured window. */
	int window = MAX(1, (int)(p->sample_rate * algorithm_parameters[ALG_AMP_SMOOTH] / 1000));
	for(i = 0; i < window; i++)
		/* Extend the waveform circularly because a lift pulse may cross the period boundary. */
		p->waveform[i + wf_size] = p->waveform[i];
	float smooth_wf[wf_size];
	smooth(p->waveform, smooth_wf, window, wf_size + window);

	double max = 0;
	/* Evaluate tic and toc separately because the two pallet directions can have different acoustic levels. */
	for(k = 0; k < 2; k++) {
		j = floor(fmod((k ? p->tic : p->toc) + p->period/8, p->period));
		for(i = 0; i < p->period/8; i++) {
			if(smooth_wf[j] > max) max = smooth_wf[j];
			if(++j > p->period) j = 0;
		}
	}
	double glob_max = vmax(smooth_wf, 0, ceil(p->period), NULL);
	/* Set the initial detection threshold from both global pulse strength and local background. */
	double threshold = fmax(algorithm_parameters[ALG_AMP_INITIAL] / 100 * glob_max,
		algorithm_parameters[ALG_AMP_NOISE] * max);
	debug("amp threshold from %s\n",
		algorithm_parameters[ALG_AMP_INITIAL] / 100 * glob_max >
		algorithm_parameters[ALG_AMP_NOISE] * max ? "global maximum" : "noise level");

	p->amp = -1;
	p->tic_pulse = p->toc_pulse = -1;
	enum detection_status rejection = DETECTION_STATUS_AMPLITUDE_NOT_FOUND;
	/* Try progressively higher thresholds until both directions produce plausible amplitudes. */
	while(threshold < algorithm_parameters[ALG_AMP_LIMIT] / 100 * glob_max) {
		debug("amp threshold = %f%% glob max\n", threshold * 100 / glob_max);
		double tic_pulse = -1;
		double toc_pulse = -1;
		/* Evaluate tic and toc separately because the two pallet directions can have different acoustic levels. */
		for(k = 0; k < 2; k++) {
			double max = 0;
			/* Start one eighth of a period before the central tic/toc reference. */
			j = floor(fmod((k ? p->tic : p->toc) + 7*p->period/8, p->period));
			for(i = 0; i < p->period/8; i++) {
				/* The first crossing marks the detectable onset of the composite lift sound, normally near unlocking. */
				if(smooth_wf[j] > threshold) break;
				if(++j > p->period) j = 0;
			}
			for(; i < p->period/8; i++) {
				double x = smooth_wf[j];
				/* Follow the rising envelope to a stable local maximum. */
				if(x > max) max = x;
				else break;
				if(++j > p->period) j = 0;
			}
			if(i < p->period/8) {
				/* Convert the detected boundary into tic or toc lift time in samples. */
				double pulse = p->period/8 - i - 1;
				if(k) tic_pulse = pulse;
				else toc_pulse = pulse;
				debug("amp %s pulse = %f\n", k ? "tic" : "toc", 1000 * pulse / p->sample_rate);
			} else
				goto next_threshold;
		}
		/* Apply the sinusoidal relation between lift time, event period, and balance amplitude. */
		double tic_amp_abs = .5 / sin(M_PI * tic_pulse / p->period);
		double toc_amp_abs = .5 / sin(M_PI * toc_pulse / p->period);
		/* Apply the sinusoidal relation between lift time, event period, and balance amplitude. */
		double tic_amp = la * tic_amp_abs;
		double toc_amp = la * toc_amp_abs;
		/* Accept only physically plausible and mutually consistent tic/toc amplitudes. */
		if(tic_amp <= algorithm_parameters[ALG_AMP_MIN] ||
				toc_amp <= algorithm_parameters[ALG_AMP_MIN]) {
			rejection = DETECTION_STATUS_AMPLITUDE_TOO_LOW;
		} else if(tic_amp >= 360 || toc_amp >= 360) {
			rejection = DETECTION_STATUS_AMPLITUDE_TOO_HIGH;
		} else if(fabs(tic_amp - toc_amp) >= algorithm_parameters[ALG_AMP_DIFFERENCE]) {
			rejection = DETECTION_STATUS_AMPLITUDE_INCONSISTENT;
		} else {
			/* Apply the sinusoidal relation between lift time, event period, and balance amplitude. */
			p->amp = (tic_amp_abs + toc_amp_abs) / 2;
			p->tic_pulse = tic_pulse;
			p->toc_pulse = toc_pulse;
			/* Refine beat error using the independently detected tic and toc lift times. */
			p->be = p->period/2 - fabs(p->toc - p->tic + p->tic_pulse - p->toc_pulse);
			debug("amp: be = %.1f\n",fabs(p->be)*1000/p->sample_rate);
			debug("amp = %f\n", la * p->amp);
			p->detection_status = DETECTION_STATUS_OK;
			break;
		}
		if(rejection != DETECTION_STATUS_AMPLITUDE_NOT_FOUND)
			debug("amp rejected\n");
next_threshold:	{
			double next = threshold * algorithm_parameters[ALG_AMP_STEP];
			if(next <= threshold) break;
			threshold = next;
		}
	}
	if(p->amp < 0) {
		p->detection_status = rejection;
		debug("amp failed\n");
	}
}

/**
 * @brief Allocate time-base calibration storage.
 *
 * The arrays hold observation times, signal phases, and absolute event positions.
 *
 * @param cd Calibration state to initialize.
 */
void setup_cal_data(struct calibration_data *cd)
{
	cd->size = (int)algorithm_parameters[ALG_CAL_COUNT];
	cd->times = malloc(cd->size * sizeof(double));
	cd->phases = malloc(cd->size * sizeof(double));
	cd->events = malloc(cd->size * sizeof(uint64_t));
}

/**
 * @brief Release time-base calibration storage.
 *
 * The observation arrays allocated by setup_cal_data() are released.
 *
 * @param cd Calibration state to destroy.
 */
void cal_data_destroy(struct calibration_data *cd)
{
	free(cd->times);
	free(cd->phases);
	free(cd->events);
}

/**
 * @brief Extract one phase observation from the calibration waveform.
 *
 * The reference pulse must lie near the center of the folded second. Its rising edge defines
 * the phase used to measure audio-clock drift.
 *
 * @param p Processing context.
 * @param cd Calibration observation set.
 * @return Zero when an observation is accepted; nonzero if lock or edge detection fails.
 */
static int add_sample_cal(struct processing_buffers *p, struct calibration_data *cd)
{
	int i;
	double phase = -1;
	int lock_low = (int)(p->sample_rate * algorithm_parameters[ALG_CAL_LOCK_LOW] / 100);
	int lock_high = (int)(p->sample_rate * algorithm_parameters[ALG_CAL_LOCK_HIGH] / 100);
	/* Require the calibration pulse to lie near the center of the folded second. */
	if(p->waveform_max_i < lock_low || p->waveform_max_i > lock_high) {
		debug("unable to lock on signal\n");
		return 1;
	}
	double maxa = vmax(p->waveform,0,p->sample_rate/4,NULL);
	double maxb = vmax(p->waveform,p->sample_rate*3/4,p->sample_rate,NULL);
	double max = fmax(maxa,maxb);
	/* Place the rising-edge threshold above the surrounding maxima. */
	double thd = max + (p->waveform_max - max) * algorithm_parameters[ALG_CAL_EDGE] / 100;
	for(i = p->sample_rate/4; i <= p->waveform_max_i; i++) {
		if(p->waveform[i] > thd) {
			/* Require the calibration pulse to lie near the center of the folded second. */
			if(i > lock_low)
				/* Convert the selected edge into fractional-second phase. */
				phase = fmod((p->timestamp + p->phase + i) / p->sample_rate, 1.);
			break;
		}
	}
	if(phase < 0) {
		debug("unable to find rising edge\n");
		return 1;
	}
	debug("Phase = %f\n",phase);
	if(cd->wp < cd->size) {
		if(cd->wp == 0)
			cd->start_time = p->timestamp;
		double time = (double)(p->timestamp - cd->start_time) / p->sample_rate;
		/* Keep at least the configured interval between phase observations. */
		if(cd->wp == 0 || time > cd->times[cd->wp-1] + algorithm_parameters[ALG_CAL_INTERVAL]) {
			cd->times[cd->wp] = time;
			cd->phases[cd->wp] = phase;
			cd->events[cd->wp] = (p->timestamp - p->timestamp % p->sample_rate) +
						(uint64_t)floor(phase * p->sample_rate);
			cd->wp++;
		}
	}
	return 0;
}

/**
 * @brief Estimate audio time-base correction from accepted phase observations.
 *
 * The regression is calculated from a local unwrapped view of the phases, so asking for an
 * intermediate estimate never changes the observations used by later calibration cycles.
 *
 * @param cd Calibration observations.
 * @param estimate Receives the fit and the observation span.
 * @return Nonzero when at least three valid observations produce a bounded finite fit.
 */
int estimate_calibration(const struct calibration_data *cd, struct calibration_estimate *estimate)
{
	if(!cd || !estimate)
		return 0;
	memset(estimate, 0, sizeof(*estimate));
	int count = cd->wp;
	if(count < 3 || count > cd->size || !cd->times || !cd->phases)
		return 0;

	double x = 0, y = 0;
	for(int i=0; i<count; i++) {
		if(!isfinite(cd->times[i]) || !isfinite(cd->phases[i]) ||
				cd->phases[i] < 0 || cd->phases[i] >= 1 ||
				(i && cd->times[i] <= cd->times[i-1]))
			return 0;
		x += cos(cd->phases[i] * 2 * M_PI);
		y += sin(cd->phases[i] * 2 * M_PI);
	}
	/* Find the circular mean phase before unwrapping the observations. */
	double c = (atan2(y,x) - M_PI) / (2 * M_PI);
	double x_av = 0, y_av = 0;
	for(int i=0; i<count; i++) {
		x_av += cd->times[i];
		y_av += fmod(cd->phases[i] - c, 1);
	}
	x_av /= count;
	y_av /= count;
	double xx = 0, xy = 0, yy = 0;
	for(int i=0; i<count; i++) {
		double x = cd->times[i] - x_av;
		double y = fmod(cd->phases[i] - c, 1) - y_av;
		/* Accumulate covariance terms for linear regression. */
		xx += x * x;
		xy += x * y;
		yy += y * y;
	}
	double determinant = xx * yy - xy * xy;
	double determinant_scale = fmax(fabs(xx * yy), fabs(xy * xy));
	if(!isfinite(xx) || !isfinite(xy) || !isfinite(yy) || xx <= 0 ||
			!isfinite(determinant) ||
			(determinant < 0 &&
			 determinant < -DBL_EPSILON * determinant_scale * 16))
		return 0;
	if(determinant < 0)
		determinant = 0;

	double calibration = xy * 3600 * 24 / xx;
	double uncertainty = sqrt(determinant / (count - 2)) / xx * 3600 * 24;
	double duration = cd->times[count-1] - cd->times[0];
	if(!isfinite(calibration) || !isfinite(uncertainty) || !isfinite(duration) ||
			duration <= 0 || calibration < MIN_CAL / 10.0 || calibration > MAX_CAL / 10.0)
		return 0;

	estimate->calibration = calibration;
	estimate->uncertainty = uncertainty;
	estimate->duration = duration;
	estimate->observations = count;
	return 1;
}

/**
 * @brief Finalize calibration using the same regression as provisional estimates.
 *
 * @param cd Completed calibration observation set.
 */
static void compute_cal(struct calibration_data *cd)
{
	struct calibration_estimate estimate;
	if(!estimate_calibration(cd, &estimate)) {
		cd->state = -1;
		return;
	}
	cd->calibration = estimate.calibration;
	debug("Calibration result: %f s/d +- %f\n", cd->calibration, estimate.uncertainty);
	cd->state = estimate.uncertainty < algorithm_parameters[ALG_CAL_PRECISION] ? 1 : -1;
}

/**
 * @brief Run the complete mechanical-watch analysis pipeline.
 *
 * The pipeline conditions the recording, measures tic-to-tic period and rate stability,
 * separates tic and toc, estimates beat error and amplitude, and locates individual events for
 * the trace.
 *
 * @param p Processing context.
 * @param bph Nominal beats per hour, or zero for automatic detection.
 * @param la Lift angle in degrees.
 * @param light Nonzero to skip sustained-noise suppression.
 */
void process(struct processing_buffers *p, int bph, double la, int light)
{
	/* Clear any alternative period candidate from the previous processing block. */
	p->candidate_period = 0;
	p->period_ready = 0;
	p->detection_status = DETECTION_STATUS_NO_WAVEFORM;
	/* Condition the audio and calculate its autocorrelation. */
	prepare_data(p, !light);
	/* Measure the interval between consecutive equivalent tic or toc events. */
	p->ready = !compute_period(p,bph);
	/* Limit the period with the configured multiplier, using typical BPH when guessing. */
	const int min_bph = bph ? bph : TYP_BPH;
	/* Reject periods more than 20 percent slower than the configured or typical beat rate. */
	const int max_period = (int)(algorithm_parameters[ALG_PERIOD_LIMIT] * 7200 * p->sample_rate / min_bph);
	/* Reject periods more than 20 percent slower than the configured or typical beat rate. */
	if(p->ready && p->period >= max_period) {
		debug("Detected period too long\n");
		p->ready = 0;
		p->detection_status = DETECTION_STATUS_PERIOD_TOO_LONG;
	}
	if(!p->ready && bph)
		p->candidate_period = estimate_period(p);
	if(!p->ready) {
		/* Measure the interval between consecutive equivalent tic or toc events. */
		debug("abort after compute_period()\n");
		return;
	}
	p->period_ready = 1;
	/* Fold repeated beats into one representative tic/toc cycle. */
	prepare_waveform(p);
	/* Separate tic from toc and calculate their timing asymmetry. */
	p->ready = !compute_parameters(p);
	if(!p->ready)
		p->detection_status = DETECTION_STATUS_NO_BEAT;
	if(bph && !p->ready)
		p->candidate_period = estimate_period(p);
	if(bph && p->ready && p->sigma > p->period * algorithm_parameters[ALG_PERIOD_SIGMA])
		p->candidate_period = estimate_period(p);
	if(!p->ready) {
		/* Separate tic from toc and calculate their timing asymmetry. */
		debug("abort after compute_parameters()\n");
		p->tic = p->toc = 0;
		p->tic_pulse = p->toc_pulse = -1;
		p->amp = -1;
		p->be = 0;
		memset(p->events, 0, EVENTS_MAX * sizeof(*p->events));
		memset(p->events_tictoc, 0, EVENTS_MAX * sizeof(*p->events_tictoc));
		return;
	}
	/* Measure acoustic lift times and infer balance amplitude from the configured lift angle. */
	compute_amplitude(p, la);
	/* Locate individual tic and toc occurrences for the trace. */
	locate_events(p);
}

/**
 * @brief Test whether a 7200 BPH calibration reference is detectable.
 *
 * The normal conditioning and period detector are used with the fixed calibration beat rate.
 *
 * @param p Processing context.
 * @return Result returned by compute_period().
 */
int test_cal(struct processing_buffers *p)
{
	prepare_data(p,0);
	return compute_period(p,7200);
}

/**
 * @brief Process one calibration block and update time-base correction.
 *
 * A phase observation is collected from each valid block; regression is performed when the
 * observation set is complete.
 *
 * @param p Processing context.
 * @param cd Calibration state.
 * @return Zero on success; nonzero if the calibration signal cannot be processed.
 */
int process_cal(struct processing_buffers *p, struct calibration_data *cd)
{
	prepare_data(p,0);
	if(compute_period(p,7200)) {
		debug("abort after compute_period()\n");
		return 1;
	}
	prepare_waveform_cal(p);
	if(add_sample_cal(p,cd))
		return 1;
	if(cd->wp == cd->size && cd->state == 0)
		compute_cal(cd);
	return 0;
}
