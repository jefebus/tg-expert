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

#include "tg.h"
#include <portaudio.h>

/* Huge buffer of audio */
float pa_buffers[PA_BUFF_SIZE];
int write_pointer = 0;
uint64_t timestamp = 0;
pthread_mutex_t audio_mutex;
static int audio_mutex_initialized = 0;
static PaDeviceIndex selected_input_device = paNoDevice;
static int selected_api_type = AUDIO_API_DEFAULT;	// PaHostApiTypeId, or AUDIO_API_DEFAULT
static int selected_sample_rate = PA_SAMPLE_RATE;
static float input_level = 0;
static int audio_errors_quiet = 0;
static int pa_init_count = 0;
static int stream_open = 0;	// an input stream is running on selected_input_device
static char *preferred_device_name = NULL;	// device to look up by name when none is selected

/* Report an audio error, unless the caller collects errors and reports one itself */
static void audio_error(const char *format, ...)
{
	char s[200];
	va_list args;
	va_start(args, format);
	vsnprintf(s, sizeof(s), format, args);
	va_end(args);
	if(audio_errors_quiet)
		fprintf(stderr, "%s\n", s);
	else
		error("%s", s);
}

void set_audio_errors_quiet(int quiet)
{
	audio_errors_quiet = quiet;
}

/* The host API in use: the selected one if this PortAudio build has it,
 * otherwise PortAudio's default. Requires PortAudio to be initialized. */
static PaHostApiIndex current_host_api(void)
{
	if(selected_api_type != AUDIO_API_DEFAULT) {
		PaHostApiIndex api = Pa_HostApiTypeIdToHostApiIndex((PaHostApiTypeId)selected_api_type);
		if(api >= 0)
			return api;
	}
	return Pa_GetDefaultHostApi();
}

/* Data for PA callback to use */
static struct callback_info {
	int 	channels;	//!< Number of channels
	bool	light;		//!< Light algorithm in use, copy half data
} info;

static int paudio_callback(const void *input_buffer,
			   void *output_buffer,
			   unsigned long frame_count,
			   const PaStreamCallbackTimeInfo *time_info,
			   PaStreamCallbackFlags status_flags,
			   void *data)
{
	UNUSED(output_buffer);
	UNUSED(time_info);
	UNUSED(status_flags);
	const float *input_samples = (const float*)input_buffer;
	unsigned long i;
	const struct callback_info *info = data;
	unsigned wp = write_pointer;
	double level_sum = 0;
	for(i = 0; i < frame_count; i++) {
		double frame_sum = 0;
		for(int channel = 0; channel < info->channels; channel++)
			frame_sum += input_samples[i * info->channels + channel] *
				input_samples[i * info->channels + channel];
		level_sum += frame_sum / info->channels;
	}
	float current_level = frame_count ? sqrt(level_sum / frame_count) : 0;

	if (info->light) {
		static bool even = true;
		/* Copy every other sample.  It would be much more efficient to
		 * just drop the sample rate if the sound hardware supports it.
		 * This would also avoid the aliasing effects that this simple
		 * decimation without a low-pass filter causes.  */
		if(info->channels == 1) {
			for(i = even ? 0 : 1; i < frame_count; i += 2) {
				pa_buffers[wp++] = input_samples[i];
				if (wp >= PA_BUFF_SIZE) wp -= PA_BUFF_SIZE;
			}
		} else {
			for(i = even ? 0 : 2; i < frame_count*2; i += 4) {
				pa_buffers[wp++] = input_samples[i] + input_samples[i+1];
				if (wp >= PA_BUFF_SIZE) wp -= PA_BUFF_SIZE;
			}
		}
		/* Keep track if we have processed an even number of frames, so
		 * we know if we should drop the 1st or 2nd frame next callback. */
		if(frame_count % 2) even = !even;
	} else {
		const unsigned len = MIN(frame_count, PA_BUFF_SIZE - wp);
		if(info->channels == 1) {
			memcpy(pa_buffers + wp, input_samples, len * sizeof(*pa_buffers));
			if(len < frame_count)
				memcpy(pa_buffers, input_samples + len, (frame_count - len) * sizeof(*pa_buffers));
		} else {
			for(i = 0; i < len; i++)
				pa_buffers[wp + i] = input_samples[2u*i] + input_samples[2u*i + 1u];
			if(len < frame_count)
				for(i = len; i < frame_count; i++)
					pa_buffers[i - len] = input_samples[2u*i] + input_samples[2u*i + 1u];
		}
		wp = (wp + frame_count) % PA_BUFF_SIZE;
	}
	pthread_mutex_lock(&audio_mutex);
	write_pointer = wp;
	timestamp += frame_count;
	input_level = 0.8f * input_level + 0.2f * current_level;
	pthread_mutex_unlock(&audio_mutex);
	return 0;
}

int start_portaudio(int *nominal_sample_rate, double *real_sample_rate)
{
	PaStream *stream;

	if(!audio_mutex_initialized) {
		if(pthread_mutex_init(&audio_mutex,NULL)) {
			audio_error(_("Failed to setup audio mutex"));
			return 1;
		}
		audio_mutex_initialized = 1;
	}

	PaError err = Pa_Initialize();
	if(err!=paNoError)
		goto error;
	pa_init_count++;

#ifdef DEBUG
	if(testing) {
		*nominal_sample_rate = selected_sample_rate;
		*real_sample_rate = selected_sample_rate;
		goto end;
	}
#endif

	PaHostApiIndex api = current_host_api();
	const PaHostApiInfo *api_info = api >= 0 ? Pa_GetHostApiInfo(api) : NULL;
	if(!api_info) {
		audio_error(_("No audio system available"));
		return 1;
	}
	if(selected_api_type != AUDIO_API_DEFAULT && api_info->type != (PaHostApiTypeId)selected_api_type)
		fprintf(stderr, "Audio: selected audio system %d is not available, using %s\n",
			selected_api_type, api_info->name);

	/* Only devices of the current audio system are used. A device chosen
	 * explicitly does not need a default input. */
	PaDeviceIndex input_device = paNoDevice;
	if(selected_input_device != paNoDevice) {
		const PaDeviceInfo *selected_info = Pa_GetDeviceInfo(selected_input_device);
		if(selected_info && selected_info->maxInputChannels > 0 && selected_info->hostApi == api)
			input_device = selected_input_device;
		else
			fprintf(stderr, "Audio: selected device %d is not usable with %s, falling back to default\n",
				(int)selected_input_device, api_info->name);
	}
	/* A device saved by name (at startup): device numbers change between sessions */
	if(input_device == paNoDevice && selected_input_device == paNoDevice && preferred_device_name) {
		for(PaDeviceIndex i = 0; i < Pa_GetDeviceCount(); i++) {
			const PaDeviceInfo *di = Pa_GetDeviceInfo(i);
			if(di && di->hostApi == api && di->maxInputChannels > 0 && di->name &&
					!strcmp(di->name, preferred_device_name)) {
				input_device = i;
				break;
			}
		}
		if(input_device == paNoDevice)
			fprintf(stderr, "Audio: saved input '%s' not found in %s, using the default input\n",
				preferred_device_name, api_info->name);
	}
	if(input_device == paNoDevice) {
		input_device = api_info->defaultInputDevice;
		if(input_device == paNoDevice) {
			audio_error(_("No default audio input device found for %s"), api_info->name);
			return 1;
		}
	}

	const PaDeviceInfo *input_info = Pa_GetDeviceInfo(input_device);
	if(!input_info) {
		audio_error(_("Unable to query selected audio input device"));
		return 1;
	}

	long channels = input_info->maxInputChannels;
	if(channels == 0) {
		audio_error(_("Selected audio device has no input channels"));
		return 1;
	}
	if(channels > 2) channels = 2;
	info.channels = channels;
	info.light = false;

	PaStreamParameters input_parameters;
	input_parameters.device = input_device;
	input_parameters.channelCount = (int)channels;
	input_parameters.sampleFormat = paFloat32;
	input_parameters.suggestedLatency = input_info->defaultLowInputLatency;
	input_parameters.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream(&stream,&input_parameters,NULL,selected_sample_rate,paFramesPerBufferUnspecified,paNoFlag,paudio_callback,&info);
	if(err!=paNoError) {
		/* If opening the chosen device failed, try to find a fallback:
		 * - On Linux with JACK running, the ALSA device may be locked;
		 *   scan for a JACK host API device and retry.
		 * - On macOS, CoreAudio may fail if permissions aren't granted;
		 *   surface a specific diagnostic and fall through to the error. */
		const PaDeviceInfo *failed_info = Pa_GetDeviceInfo(input_device);
		PaHostApiIndex failed_host = failed_info ? failed_info->hostApi : -1;
		const PaHostApiInfo *failed_api = (failed_host >= 0) ? Pa_GetHostApiInfo(failed_host) : NULL;
		const char *failed_api_name = failed_api ? failed_api->name : "unknown";

		fprintf(stderr, "Audio: could not open '%s' [%s] at %d Hz: %s\n",
			failed_info && failed_info->name ? failed_info->name : "unknown device",
			failed_api_name, selected_sample_rate, Pa_GetErrorText(err));

		/* Try JACK fallback if ALSA device failed */
		PaHostApiIndex jack_api = Pa_HostApiTypeIdToHostApiIndex(paJACK);
		/* Only when no audio system was chosen explicitly: the device list
		 * shows a single audio system, and JACK would be outside it. */
		if(jack_api >= 0 && failed_api && failed_api->type == paALSA &&
				selected_api_type == AUDIO_API_DEFAULT) {
			const PaHostApiInfo *jack_info = Pa_GetHostApiInfo(jack_api);
			if(jack_info && jack_info->defaultInputDevice != paNoDevice) {
				PaDeviceIndex jack_device = jack_info->defaultInputDevice;
				const PaDeviceInfo *jdi = Pa_GetDeviceInfo(jack_device);
				if(jdi && jdi->maxInputChannels > 0) {
					debug("ALSA device failed; retrying with JACK device %d\n", (int)jack_device);
					long jchannels = jdi->maxInputChannels > 2 ? 2 : jdi->maxInputChannels;
					info.channels = (int)jchannels;
					PaStreamParameters jp;
					jp.device = jack_device;
					jp.channelCount = (int)jchannels;
					jp.sampleFormat = paFloat32;
					jp.suggestedLatency = jdi->defaultLowInputLatency;
					jp.hostApiSpecificStreamInfo = NULL;
					PaError jerr = Pa_OpenStream(&stream,&jp,NULL,selected_sample_rate,paFramesPerBufferUnspecified,paNoFlag,paudio_callback,&info);
					if(jerr == paNoError) {
						input_device = jack_device;
						err = paNoError;
					} else {
						fprintf(stderr, "Audio: JACK fallback also failed: %s\n", Pa_GetErrorText(jerr));
						/* Restore info.channels for the primary error path */
						info.channels = (int)channels;
					}
				}
			}
		}

		if(err != paNoError) {
			/* Emit platform-specific hint before the generic error */
			if(failed_api && failed_api->type == paALSA)
				audio_error(_("Audio device is busy. If JACK (jackd/PipeWire-JACK) is running, "
				      "try selecting JACK in the audio settings or stop JACK first."));
#ifdef __APPLE__
			if(failed_api && failed_api->type == paCoreAudio)
				audio_error(_("Could not open audio input on macOS. Check System Settings > "
				      "Privacy & Security > Microphone and allow access for this application."));
#endif
			goto error;
		}
	}

	err = Pa_StartStream(stream);
	if(err!=paNoError)
		goto error;

	const int used_channels = info.channels;
	const PaStreamInfo *info = Pa_GetStreamInfo(stream);
	*nominal_sample_rate = selected_sample_rate;
	*real_sample_rate = info->sampleRate;
	selected_input_device = input_device;
	stream_open = 1;
	{
		const PaDeviceInfo *di = Pa_GetDeviceInfo(input_device);
		const PaHostApiInfo *hi = di ? Pa_GetHostApiInfo(di->hostApi) : NULL;
		fprintf(stderr, "Audio input: %s [%s], %d Hz, %d channel%s\n",
			di && di->name ? di->name : "unknown device",
			hi && hi->name ? hi->name : "unknown API",
			selected_sample_rate, used_channels,
			used_channels > 1 ? "s" : "");
	}
#ifdef DEBUG
end:
#endif
	debug("sample rate: nominal = %d real = %f\n",*nominal_sample_rate,*real_sample_rate);

	return 0;

error:
	audio_error(_("Error opening audio input: %s"), Pa_GetErrorText(err));
	return 1;
}

int list_audio_input_devices(int **devices, char ***names, int *count, int *default_device)
{
	if(!devices || !names || !count || !default_device)
		return 1;

	*devices = NULL;
	*names = NULL;
	*count = 0;
	PaHostApiIndex api = current_host_api();
	const PaHostApiInfo *api_info = api >= 0 ? Pa_GetHostApiInfo(api) : NULL;
	if(!api_info)
		return 1;
	*default_device = (int)api_info->defaultInputDevice;

	int ndevices = Pa_GetDeviceCount();
	if(ndevices < 0)
		return 1;

	int *devs = malloc(ndevices * sizeof(*devs));
	char **dev_names = malloc(ndevices * sizeof(*dev_names));
	if(!devs || !dev_names) {
		free(devs);
		free(dev_names);
		return 1;
	}

	int used = 0;
	for(int i = 0; i < ndevices; i++) {
		const PaDeviceInfo *di = Pa_GetDeviceInfo((PaDeviceIndex)i);
		if(!di || di->maxInputChannels <= 0 || di->hostApi != api)
			continue;

		devs[used] = i;
		dev_names[used] = strdup(di->name ? di->name : _("Unknown device"));
		if(!dev_names[used]) {
			for(int j = 0; j < used; j++)
				free(dev_names[j]);
			free(devs);
			free(dev_names);
			return 1;
		}
		used++;
	}

	if(!used) {
		free(devs);
		free(dev_names);
		return 1;
	}

	*devices = devs;
	*names = dev_names;
	*count = used;
	return 0;
}

void free_audio_input_devices(int *devices, char **names, int count)
{
	for(int i = 0; i < count; i++)
		free(names[i]);
	free(names);
	free(devices);
}

int list_audio_apis(int **types, char ***names, int *count, int *current_type)
{
	if(!types || !names || !count || !current_type)
		return 1;

	*types = NULL;
	*names = NULL;
	*count = 0;

	int napis = Pa_GetHostApiCount();
	PaHostApiIndex current = current_host_api();
	if(napis <= 0 || current < 0)
		return 1;

	int *t = malloc(napis * sizeof(*t));
	char **n = malloc(napis * sizeof(*n));
	if(!t || !n) {
		free(t);
		free(n);
		return 1;
	}

	int used = 0;
	for(int i = 0; i < napis; i++) {
		const PaHostApiInfo *hi = Pa_GetHostApiInfo((PaHostApiIndex)i);
		if(!hi)
			continue;
		t[used] = (int)hi->type;
		n[used] = strdup(hi->name ? hi->name : _("Unknown audio system"));
		if(!n[used]) {
			for(int j = 0; j < used; j++)
				free(n[j]);
			free(t);
			free(n);
			return 1;
		}
		if(i == current)
			*current_type = (int)hi->type;
		used++;
	}

	*types = t;
	*names = n;
	*count = used;
	return 0;
}

void free_audio_apis(int *types, char **names, int count)
{
	free_audio_input_devices(types, names, count);
}

void set_audio_api(int type)
{
	selected_api_type = type;
}

/* The audio system in use (PaHostApiTypeId), or AUDIO_API_DEFAULT if unknown */
int get_audio_api(void)
{
	if(pa_init_count <= 0)
		return AUDIO_API_DEFAULT;
	const PaHostApiInfo *hi = Pa_GetHostApiInfo(current_host_api());
	return hi ? (int)hi->type : AUDIO_API_DEFAULT;
}

const char *get_audio_api_name(void)
{
	if(pa_init_count <= 0)
		return NULL;
	const PaHostApiInfo *hi = Pa_GetHostApiInfo(current_host_api());
	return hi ? hi->name : NULL;
}

/* The default input of the audio system in use, or AUDIO_DEVICE_DEFAULT if it has none */
int get_default_audio_input_device(void)
{
	if(pa_init_count <= 0)
		return AUDIO_DEVICE_DEFAULT;
	const PaHostApiInfo *hi = Pa_GetHostApiInfo(current_host_api());
	return hi && hi->defaultInputDevice != paNoDevice ? (int)hi->defaultInputDevice : AUDIO_DEVICE_DEFAULT;
}

/* Look up this device by name on the next start, when no device is selected */
void set_audio_input_device_name(const char *name)
{
	free(preferred_device_name);
	preferred_device_name = name && *name ? strdup(name) : NULL;
}

/* Name of the open input device, or NULL if no stream is open */
const char *get_audio_input_device_name(void)
{
	if(!stream_open)
		return NULL;
	const PaDeviceInfo *di = Pa_GetDeviceInfo(selected_input_device);
	return di ? di->name : NULL;
}

/* Whether rate is one tg offers. Faster rates do not fit the analysis buffers. */
int sample_rate_valid(int rate)
{
	static const int rates[] = SAMPLE_RATES;
	for(int i = 0; rates[i]; i++)
		if(rates[i] == rate && rate <= (int)MAX_SAMPLE_RATE)
			return 1;
	return 0;
}

/* Rates from SAMPLE_RATES that the device accepts, in increasing order.
 * PortAudio cannot list them, so each one is checked with Pa_IsFormatSupported. */
int list_supported_sample_rates(int device, int *rates, int max)
{
	static const int candidates[] = SAMPLE_RATES;
	if(pa_init_count <= 0 || device < 0 || device >= Pa_GetDeviceCount())
		return 0;
	const PaDeviceInfo *di = Pa_GetDeviceInfo((PaDeviceIndex)device);
	if(!di || di->maxInputChannels <= 0)
		return 0;

	PaStreamParameters p;
	p.device = (PaDeviceIndex)device;
	p.channelCount = di->maxInputChannels > 2 ? 2 : di->maxInputChannels;
	p.sampleFormat = paFloat32;
	p.suggestedLatency = di->defaultLowInputLatency;
	p.hostApiSpecificStreamInfo = NULL;

	int n = 0;
	for(int i = 0; candidates[i] && n < max; i++) {
		int rate = candidates[i];
		if(rate > (int)MAX_SAMPLE_RATE)
			continue;
		int ok = Pa_IsFormatSupported(&p, NULL, rate) == paFormatIsSupported;
		/* A device in use may refuse the check (exclusive access), but its rate works */
		if(!ok && stream_open && device == selected_input_device && rate == selected_sample_rate)
			ok = 1;
		if(ok)
			rates[n++] = rate;
	}
	return n;
}

void set_audio_input_device(int device)
{
	selected_input_device = device >= 0 ? (PaDeviceIndex)device : paNoDevice;
}

int get_audio_input_device(void)
{
	return selected_input_device == paNoDevice ? AUDIO_DEVICE_DEFAULT : (int)selected_input_device;
}

void set_audio_sample_rate(int sample_rate)
{
	if(sample_rate > 0)
		selected_sample_rate = sample_rate;
}

int get_audio_sample_rate(void)
{
	return selected_sample_rate;
}

float get_audio_level(void)
{
	pthread_mutex_lock(&audio_mutex);
	float level = input_level;
	pthread_mutex_unlock(&audio_mutex);
	return level;
}

int terminate_portaudio()
{
	debug("Closing portaudio\n");
	stream_open = 0;
	if(pa_init_count <= 0)
		return 0;
	pa_init_count--;
	PaError err = Pa_Terminate();
	if(err != paNoError) {
		audio_error(_("Error closing audio: %s"), Pa_GetErrorText(err));
		return 1;
	}
	return 0;
}

uint64_t get_timestamp(int light)
{
	pthread_mutex_lock(&audio_mutex);
	uint64_t ts = light ? timestamp / 2 : timestamp;
	pthread_mutex_unlock(&audio_mutex);
	return ts;
}

static void fill_buffers(struct processing_buffers *ps, int light)
{
	pthread_mutex_lock(&audio_mutex);
	uint64_t ts = timestamp;
	int wp = write_pointer;
	pthread_mutex_unlock(&audio_mutex);

	if(light)
		ts /= 2;

	int i;
	for(i = 0; i < NSTEPS; i++) {
		ps[i].timestamp = ts;

		int start = wp - ps[i].sample_count;
		if (start < 0) start += PA_BUFF_SIZE;
		int len = MIN((unsigned)ps[i].sample_count, PA_BUFF_SIZE - start);
		memcpy(ps[i].samples, pa_buffers + start, len * sizeof(*pa_buffers));
		if (len < ps[i].sample_count)
			memcpy(ps[i].samples + len, pa_buffers, (ps[i].sample_count - len) * sizeof(*pa_buffers));
	}
}

int analyze_pa_data(struct processing_data *pd, int bph, double la, uint64_t events_from)
{
	struct processing_buffers *p = pd->buffers;
	fill_buffers(p, pd->is_light);

	int i;
	for(i = 0; i < NSTEPS; i++) {
		p[i].candidate_period = 0;
		p[i].ready = 0;
		p[i].period_ready = 0;
	}
	debug("\nSTART OF COMPUTATION CYCLE\n\n");
	int last_ready = -1;
	for(i=0; i<NSTEPS; i++) {
		p[i].last_tic = pd->last_tic;
		p[i].events_from = events_from;
		process(&p[i], bph, la, pd->is_light);
		if(!p[i].period_ready) break;
		if(p[i].ready) last_ready = i;
		debug("step %d : %f +- %f\n",i,p[i].period/p[i].sample_rate,p[i].sigma/p[i].sample_rate);
	}
	if(last_ready >= 0) {
		pd->last_tic = p[last_ready].last_tic;
		debug("%f +- %f\n",p[last_ready].period/p[last_ready].sample_rate,p[last_ready].sigma/p[last_ready].sample_rate);
	} else
		debug("---\n");
	return i;
}

int analyze_pa_data_cal(struct processing_data *pd, struct calibration_data *cd)
{
	struct processing_buffers *p = pd->buffers;
	fill_buffers(p, pd->is_light);

	int i,j;
	int failed_signal = NSTEPS-1;
	debug("\nSTART OF CALIBRATION CYCLE\n\n");
	for(j=0; p[j].sample_count < 2*p[j].sample_rate; j++);
	for(i=0; i+j<NSTEPS-1; i++) {
		if(test_cal(&p[i+j])) {
			failed_signal = i ? i+j : 0;
			break;
		}
	}
	if(process_cal(&p[NSTEPS-1], cd))
		return failed_signal;
	return NSTEPS;
}

/** Change to light mode
 *
 * Call to enable or disable light mode.  Changing the mode will empty the audio
 * buffer.  Nothing will happen if the mode doesn't actually change.
 *
 * @param light True for light mode, false for normal
 */
void set_audio_light(bool light)
{
	if(info.light != light) {
		pthread_mutex_lock(&audio_mutex);
		info.light = light;
		memset(pa_buffers, 0, sizeof(pa_buffers));
		write_pointer = 0;
		timestamp = 0;
		pthread_mutex_unlock(&audio_mutex);
	}
}
