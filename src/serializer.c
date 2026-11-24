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
#include <inttypes.h>

#define LABEL_SIZE 64

#define XSTR(X) STR(X)
#define STR(X) #X
#define LABEL_SIZE_STR XSTR(LABEL_SIZE)

#ifdef _WIN32
// My own implementation of %a
// because I'm sick of bugs in __mingw_fprintf()

static char hex_digit(uint64_t n)
{
	if(n < 10) return (char)n+'0';
	else return (char)n-10+'a';
}

static int write_hex_double(FILE *f, double d)
{
	uint64_t bd;
	memcpy(&bd, &d, sizeof(bd));
	uint64_t mantissa = bd << 12;
	uint64_t exponent = (bd >> 52) & 0x7ff;
	uint64_t sign = bd >> 63;

	// nan or inf = error
	if(exponent == 0x7ff) return 1;

	if(sign && fprintf(f, "-") < 0) return 1;

	if(exponent == 0) {
		if(mantissa) {
			exponent = 1;
			if(fprintf(f, "0x0") < 0) return 1;
		} else {
			return 0 > fprintf(f, "0x0p+0");
		}
	} else {
		if(fprintf(f, "0x1") < 0) return 1;
	}

	if(mantissa && fprintf(f, ".")  < 0) return 1;
	for(; mantissa; mantissa <<= 4)
		if(fprintf(f, "%c", hex_digit( mantissa >> 60 )) < 0) return 1;

	int exp = (int)exponent - 1023;
	if(fprintf(f, "p%c", exp < 0 ? '-' : '+') < 0) return 1;
	return 0 > fprintf(f,"%d",abs(exp));
}

static int parse_hex_digit(char c)
{
	if('0' <= c && c <= '9') return c - '0';
	if('a' <= c && c <= 'f') return 10 + c - 'a';
	if('A' <= c && c <= 'F') return 10 + c - 'A';
	return -1;
}

static int parse_hex_double(FILE *f, double *d)
{
	char s[64], *t = s;
	if(1 != fscanf(f, "%63[0-9a-fA-F.xXpP+-]", s)) return 1;
	double place = 1;
	if(*t == '+') t++;
	else if(*t == '-') {
		place = -1;
		t++;
	}
	if(*t != '0') return 1;
	if(*++t != 'x' && *t != 'X') return 1;
	int n = parse_hex_digit(*++t);
	if(n < 0) return 1;
	*d = place * n;
	if(!*++t) return 0;
	if(*t != '.') goto exponent;
	while(*++t && *t != 'p' && *t != 'P') {
		place /= 16;
		n = parse_hex_digit(*t);
		if(n < 0) return 1;
		*d += place * n;
	}
exponent:
	if(*t) {
		long int radix = strtol(t+1, &t, 10);
		if(*t) return 1;
		*d *= pow(2, radix);
	}
	return 0;
}
#endif

static int serialize_uint64_t(FILE *f, uint64_t x)
{
	return 0 > fprintf(f, "I%"PRIu64";\n", x);
}

static int scan_uint64_t(FILE *f, uint64_t *x)
{
	int n = 0;
	return 1 != fscanf(f, " I%"SCNu64";%n", x, &n) || !n;
}

static int serialize_int64_t(FILE *f, int64_t x)
{
	return 0 > fprintf(f, "I%"PRId64";\n", x);
}

static int scan_int64_t(FILE *f, int64_t *x)
{
	int n = 0;
	return 1 != fscanf(f, " I%"SCNd64";%n", x, &n) || !n;
}

static int serialize_int(FILE *f, int x)
{
	return serialize_int64_t(f, x);
}

static int scan_int(FILE *f, int *x)
{
	int64_t y;
	if(scan_int64_t(f, &y) || y < INT_MIN || y > INT_MAX) return 1;
	*x = y;
	return 0;
}

static int serialize_double(FILE *f, double x)
{
#ifdef _WIN32
	if(fprintf(f, "I") < 0) return 1;
	if(write_hex_double(f, x)) return 1;
	return 0 > fprintf(f, ";\n");
#else
	return 0 > fprintf(f, "I%a;\n", x);
#endif
}

static int scan_double(FILE *f, double *x)
{
	int n = 0;
#ifdef _WIN32
	if( 0 != fscanf(f, " I%n", &n) || !n ) return 1;
	if( parse_hex_double(f,x) ) return 1;
	n = 0;
	if( 0 != fscanf(f, ";%n", &n) || !n ) return 1;
	return 0;
#else
	return 1 != fscanf(f, " I%la;%n", x, &n) || !n;
#endif
}

static int serialize_float(FILE *f, float x)
{
	return serialize_double(f, x);
}

static int scan_float(FILE *f, float *x)
{
	double y;
	if(scan_double(f, &y)) return 1;
	*x = y;
	return 0;
}

static int serialize_string(FILE *f, char *s)
{
	if(0 > fprintf(f, "S%"PRIu64";", (uint64_t)strlen(s))) return 1;
	return 0 > fprintf(f, "%s;\n", s);
}

static int scan_string(FILE *f, char **s, uint64_t max_l, uint64_t *len)
{
	uint64_t l;
	int n = 0;
	if(1 != fscanf(f, " S%"SCNu64";%n", &l, &n) || !n) return 1;
	if(max_l && l >= max_l) return 1;
	if(!*s) *s = malloc(l+1);
	if(l+1 != fread(*s, 1, l+1, f)) return 1;
	if((*s)[l] != ';') return 1;
	(*s)[l] = 0;
	if(len) *len = l;
	return 0;
}

static int serialize_uint64_t_array(FILE *f, uint64_t *a, uint64_t len)
{
	uint64_t i;
	if(0 > fprintf(f, "A%"PRIu64";\n", len)) return 1;
	for(i = 0; i < len; i++)
		if(serialize_uint64_t(f, a[i])) return 1;
	return 0;
}

static int scan_uint64_t_array(FILE *f, uint64_t **a, uint64_t max_l, uint64_t *len)
{
	uint64_t l,i;
	int n = 0;
	if(1 != fscanf(f, " A%"SCNu64";%n", &l, &n) || !n) return 1;
	if(max_l && l > max_l) return 1;
	if(!*a) *a = malloc(l*sizeof(uint64_t));
	for(i = 0; i < l; i++)
		if(scan_uint64_t(f, *a+i)) return 1;
	if(len) *len = l;
	return 0;
}

static int serialize_bool_array(FILE *f, const unsigned char *a, uint64_t len)
{
	uint64_t i;
	if(0 > fprintf(f, "A%"PRIu64";\n", len)) return 1;
	for(i = 0; i < len; i++)
		if(serialize_int(f, a[i])) return 1;
	return 0;
}

static int scan_bool_array(FILE *f, unsigned char **a, uint64_t max_l, uint64_t *len)
{
	uint64_t l,i;
	int n = 0;
	if(1 != fscanf(f, " A%"SCNu64";%n", &l, &n) || !n) return 1;
	if(max_l && l > max_l) return 1;
	if(!*a) *a = malloc(l*sizeof(**a));
	for(i = 0; i < l; i++) {
		int j;
		if(scan_int(f, &j)) return 1;
		(*a)[i] = !!j;
	}
	if(len) *len = l;
	return 0;
}

static int serialize_float_array(FILE *f, float *a, uint64_t len)
{
	uint64_t i;
	if(0 > fprintf(f, "A%"PRIu64";\n", len)) return 1;
	for(i = 0; i < len; i++)
		if(serialize_float(f, a[i])) return 1;
	return 0;
}

static int scan_float_array(FILE *f, float **a, uint64_t max_l, uint64_t *len)
{
	uint64_t l,i;
	int n = 0;
	if(1 != fscanf(f, " A%"SCNu64";%n", &l, &n) || !n) return 1;
	if(max_l && l > max_l) return 1;
	if(!*a) *a = malloc(l*sizeof(float));
	for(i = 0; i < l; i++)
		if(scan_float(f, *a+i)) return 1;
	if(len) *len = l;
	return 0;
}

static int scan_int_array(FILE *f, int **a, uint64_t max_l, uint64_t *len)
{
	uint64_t l,i;
	int n = 0;
	if(1 != fscanf(f, " A%"SCNu64";%n", &l, &n) || !n) return 1;
	if(max_l && l > max_l) return 1;
	if(!*a) *a = malloc((l ? l : 1)*sizeof(**a));
	if(!*a) return 1;
	for(i = 0; i < l; i++)
		if(scan_int(f, *a+i)) return 1;
	if(len) *len = l;
	return 0;
}

static int make_label(FILE *f, char *l)
{
	return 0 > fprintf(f, "L%s;\n", l);
}

static int scan_label(FILE *f, char *l)
{
	int n = 0;
	return 1 != fscanf(f, " L%" LABEL_SIZE_STR "[^;];%n", l, &n) || !n;
}

static int eat_object(FILE *f)
{
	char c;
	if(1 != fscanf(f, " %c", &c)) return 1;
	uint64_t l;
	int n = 0;
	switch(c) {
		case 'I':
			return 0 != fscanf(f, "%*[^;];%n", &n) || !n;
		case 'S':
			if(1 != fscanf(f, "%"SCNu64";%n", &l, &n) || !n) return 1;
			if(fseek(f, l, SEEK_CUR)) return 1;
			return 1 != fscanf(f, "%c", &c) || c != ';';
		case 'A':
			if(1 != fscanf(f, "%"SCNu64";%n", &l, &n) || !n) return 1;
			for(;l;l--) if(eat_object(f)) return 1;
			return 0;
		case 'T':
			if(1 != fscanf(f, "%c", &c)) return 1;
			if(c != ';') return 1;
			for(;;) {
				char b[LABEL_SIZE+1];
				if(scan_label(f, b)) return 1;
				if(!strcmp("__end__", b)) return 0;
				if(eat_object(f)) return 1;
			}
		case 'U':
			if(1 != fscanf(f, "%c", &c)) return 1;
			if(c != ';') return 1;
			char b[LABEL_SIZE+1];
			if(scan_label(f, b)) return 1;
			if(eat_object(f)) return 1;
			return 0;
		default:
			return 1;
	}
}

static int serialize_struct_begin(FILE *f)
{
	return 0 > fprintf(f, "T;\n");
}

static int serialize_struct_end(FILE *f)
{
	return make_label(f, "__end__");
}

static int serialize_union_begin(FILE *f, char *s)
{
	if(fprintf(f, "U;\n") < 0) return 1;
	return make_label(f, s);
}

#define SERIALIZE(T,A) {				\
	if(make_label(f, #A)) return 1;			\
	if(serialize_ ## T (f, s -> A)) return 1;	\
	}

/* Long-term trend: the settings, the elapsed time and one array per value.
 * The statistics are not stored, they are computed again on loading. */
static int serialize_trend(FILE *f, const struct trend *s)
{
	int i;
	if(serialize_struct_begin(f)) return 1;
	SERIALIZE(int,bph);
	SERIALIZE(double,la);
	SERIALIZE(int,cal);
	SERIALIZE(int,nominal_sr);
	SERIALIZE(int,is_light);
	SERIALIZE(int,algorithm_custom);
	SERIALIZE(int,full);
	if(make_label(f, "elapsed")) return 1;
	if(serialize_double(f, trend_elapsed(s))) return 1;
#define TREND_ARRAY(LABEL, FIELD, TYPE)					\
	if(make_label(f, LABEL)) return 1;				\
	if(0 > fprintf(f, "A%d;\n", s->count)) return 1;		\
	for(i = 0; i < s->count; i++)					\
		if(serialize_ ## TYPE (f, s->samples[i].FIELD)) return 1;
	TREND_ARRAY("t", t, float);
	TREND_ARRAY("rate", rate, float);
	TREND_ARRAY("amp", amp, float);
	TREND_ARRAY("be", be, float);
	TREND_ARRAY("position", position, int);
	TREND_ARRAY("settling", settling, int);
#undef TREND_ARRAY
	return serialize_struct_end(f);
}

/* Returns 1 on a format error. *trend is NULL if the data is not usable. */
static int scan_trend(FILE *f, struct trend **trend)
{
	char l[LABEL_SIZE+1];
	int n = 0;
	*trend = NULL;
	if(0 != fscanf(f, " T;%n", &n) || !n) return 1;
	struct trend *t = trend_new();
	if(!t) return 1;
	float *tm = NULL, *rate = NULL, *amp = NULL, *be = NULL;
	int *position = NULL, *settling = NULL;
	uint64_t n_t = 0, n_rate = 0, n_amp = 0, n_be = 0, n_position = 0, n_settling = 0;
	double elapsed = 0;
	int ok = 1;
	for(;;) {
		if(scan_label(f,l)) goto error;
		if(!strcmp("__end__", l)) break;
#define TREND_SCAN(LABEL, ARRAY, COUNT, TYPE)					\
		if(!strcmp(LABEL, l)) {						\
			if(ARRAY || scan_ ## TYPE ## _array(f, &ARRAY,		\
					TREND_MAX_SAMPLES, &COUNT)) goto error;	\
			continue;						\
		}
		TREND_SCAN("t", tm, n_t, float);
		TREND_SCAN("rate", rate, n_rate, float);
		TREND_SCAN("amp", amp, n_amp, float);
		TREND_SCAN("be", be, n_be, float);
		TREND_SCAN("position", position, n_position, int);
		TREND_SCAN("settling", settling, n_settling, int);
#undef TREND_SCAN
#define TREND_FIELD(A, TYPE)							\
		if(!strcmp(#A, l)) {						\
			if(scan_ ## TYPE (f, &t->A)) goto error;		\
			continue;						\
		}
		TREND_FIELD(bph, int);
		TREND_FIELD(la, double);
		TREND_FIELD(cal, int);
		TREND_FIELD(nominal_sr, int);
		TREND_FIELD(is_light, int);
		TREND_FIELD(algorithm_custom, int);
		TREND_FIELD(full, int);
#undef TREND_FIELD
		if(!strcmp("elapsed", l)) {
			if(scan_double(f, &elapsed)) goto error;
			continue;
		}
		if(eat_object(f)) goto error;
	}
	// Older histories may omit amplitudes, positions and settling markers.
	if(!tm || !rate || !be ||
			n_rate != n_t || n_be != n_t || (amp && n_amp != n_t) ||
			(position && n_position != n_t) ||
			(settling && n_settling != n_t))
		ok = 0;
	if(ok && trend_set_samples(t, tm, rate, amp, be, position, settling, (int)n_t, elapsed))
		ok = 0;
	if(!ok) {
		debug("serializer: unusable trend, ignored\n");
		trend_free(t);
		t = NULL;
	}
	free(tm); free(rate); free(amp); free(be); free(position); free(settling);
	*trend = t;
	return 0;
error:
	free(tm); free(rate); free(amp); free(be); free(position); free(settling);
	trend_free(t);
	return 1;
}

static int serialize_snapshot(FILE *f, struct snapshot *s, char *name, const struct trend *trend)
{
	if(serialize_union_begin(f, "realtime-snapshot")) return 1;
	if(serialize_struct_begin(f)) return 1;
	if(name) {
		if(make_label(f, "name")) return 1;
		if(serialize_string(f, name)) return 1;
	}
	if(make_label(f, "pb->waveform")) return 1;
	if(serialize_float_array(f, s->pb->waveform, s->pb->sample_count)) return 1;
	if(make_label(f, "events")) return 1;
	if(serialize_uint64_t_array(f, s->events, s->events_count)) return 1;
	if(make_label(f, "events_tictoc")) return 1;
	if(serialize_bool_array(f, s->events_tictoc, s->events_count)) return 1;
	SERIALIZE(int,pb->sample_rate);
	SERIALIZE(double,pb->period);
	SERIALIZE(double,pb->waveform_max);
	SERIALIZE(int,pb->tic);
	SERIALIZE(int,pb->toc);
	SERIALIZE(double,pb->tic_pulse);
	SERIALIZE(double,pb->toc_pulse);
	SERIALIZE(int,is_old);
	SERIALIZE(int,rate_only);
	SERIALIZE(uint64_t,timestamp);
	SERIALIZE(int,nominal_sr);
	SERIALIZE(int,bph);
	SERIALIZE(double,la);
	SERIALIZE(int,cal);
	SERIALIZE(int,events_wp);
	SERIALIZE(int,amps_wp);
	SERIALIZE(int,signal);
	SERIALIZE(int,audio_status);
	SERIALIZE(double,sample_rate);
	SERIALIZE(int,guessed_bph);
	SERIALIZE(int,candidate_bph);
	SERIALIZE(double,rate);
	SERIALIZE(double,be);
	SERIALIZE(double,amp);
	SERIALIZE(double,trace_centering);
	SERIALIZE(double,trace_zoom);
	SERIALIZE(int,is_light);
	SERIALIZE(int,position); // older versions skip unknown labels
	if(make_label(f, "amps")) return 1;
	if(serialize_float_array(f, s->amps, s->amps_count)) return 1;
	if(make_label(f, "amps_time")) return 1;
	if(serialize_uint64_t_array(f, s->amps_time, s->amps_count)) return 1;
	if(trend) { // older versions skip unknown labels
		if(make_label(f, "trend")) return 1;
		if(serialize_trend(f, trend)) return 1;
	}
	return serialize_struct_end(f);
}

#define SCAN(T,A) {						\
	if(!strcmp( #A , l )) {					\
		debug("serializer: scanning " #A "\n");		\
		if(scan_ ## T (f, &((*s) -> A))) goto error;	\
		continue;					\
	}}

static int scan_snapshot(FILE *f, struct snapshot **s, char **name, struct trend **trend)
{
	char l[LABEL_SIZE+1];
	int n = 0;
	*s = NULL;
	*name = NULL;
	*trend = NULL;
	if(0 != fscanf(f, " U;%n", &n) || !n) return 1;
	if(scan_label(f,l)) return 1;
	if(strcmp("realtime-snapshot", l))
		return eat_object(f);

	*s = malloc(sizeof(struct snapshot));
	memset(*s, 0, sizeof(struct snapshot));
	(*s)->audio_status = AUDIO_STATUS_UNAVAILABLE;
	(*s)->pb = malloc(sizeof(struct processing_buffers));
	memset((*s)->pb, 0, sizeof(struct processing_buffers));
	*name = NULL;

	n = 0;
	if(0 != fscanf(f, " T;%n", &n) || !n) goto error;
	for(;;) {
		if(scan_label(f,l)) goto error;
		if(!strcmp("__end__", l)) break;

		if(!strcmp("name", l)) {
			debug("serializer: scanning name\n");
			if(scan_string(f, name, 0, NULL)) goto error;
			continue;
		}
		if(!strcmp("pb->waveform", l)) {
			debug("serializer: scanning pb->waveform\n");
			uint64_t x;
			if(	(*s)->pb->waveform ||
				scan_float_array(f, &((*s)->pb->waveform), INT_MAX, &x)) goto error;
			(*s)->pb->sample_count = x;
			continue;
		}
		if(!strcmp("events", l)) {
			debug("serializer: scanning events\n");
			uint64_t x;
			if(	(*s)->events ||
				scan_uint64_t_array(f, &((*s)->events), INT_MAX, &x)) goto error;
			(*s)->events_count = x;
			continue;
		}
		if(!strcmp("events_tictoc", l)) {
			debug("serializer: scanning events_tictoc\n");
			uint64_t x;
			if(	(*s)->events_tictoc ||
				scan_bool_array(f, &((*s)->events_tictoc), INT_MAX, &x)) goto error;
			continue;
		}
		if(!strcmp("amps", l)) {
			debug("serializer: scanning amps\n");
			uint64_t x;
			if(	(*s)->amps ||
				scan_float_array(f, &((*s)->amps), INT_MAX, &x)) goto error;
			(*s)->amps_count = x;
			continue;
		}
		if(!strcmp("amps_time", l)) {
			debug("serializer: scanning amps_time\n");
			uint64_t x;
			if(	(*s)->amps_time ||
				scan_uint64_t_array(f, &((*s)->amps_time), INT_MAX, &x)) goto error;
			continue;
		}
		if(!strcmp("trend", l)) {
			debug("serializer: scanning trend\n");
			if(*trend || scan_trend(f, trend)) goto error;
			continue;
		}
		SCAN(int,pb->sample_rate);
		SCAN(double,pb->period);
		SCAN(double,pb->waveform_max);
		SCAN(int,pb->tic);
		SCAN(int,pb->toc);
		SCAN(double,pb->tic_pulse);
		SCAN(double,pb->toc_pulse);
		SCAN(int,is_old);
		SCAN(int,rate_only);
		SCAN(uint64_t,timestamp);
		SCAN(int,nominal_sr);
		SCAN(int,bph);
		SCAN(double,la);
		SCAN(int,cal);
		SCAN(int,events_wp);
		SCAN(int,amps_wp);
		SCAN(int,signal);
		SCAN(int,audio_status);
		SCAN(double,sample_rate);
		SCAN(int,guessed_bph);
		SCAN(int,candidate_bph);
		SCAN(double,rate);
		SCAN(double,be);
		SCAN(double,amp);
		SCAN(double,trace_centering);
		SCAN(double,trace_zoom);
		SCAN(int,is_light);
		SCAN(int,position);

		if(eat_object(f)) goto error;
	}
	debug("serializer: checking period\n");
	if((*s)->pb->period <= 0 || (*s)->pb->sample_count < ceil((*s)->pb->period)) goto error;
	debug("serializer: checking timestamp\n");
	if(!(*s)->timestamp) goto error;
	debug("serializer: checking nominal_sr\n");
	if(!(*s)->nominal_sr) goto error;
	debug("serializer: checking bph\n");
	if((*s)->bph && ( (*s)->bph < MIN_BPH || (*s)->bph > MAX_BPH )) goto error;
	debug("serializer: checking la\n");
	if((*s)->la  < MIN_LA  || (*s)->la  > MAX_LA ) goto error;
	debug("serializer: checking cal\n");
	if((*s)->cal < MIN_CAL || (*s)->cal > MAX_CAL) goto error;
	debug("serializer: checking events\n");
	if((*s)->events_count && (*s)->events_wp >= (*s)->events_count) goto error;
	if((*s)->signal > NSTEPS) (*s)->signal = NSTEPS;
	if((*s)->rate_only != 0 && (*s)->rate_only != 1) goto error;
	if((*s)->rate_only && !(*s)->is_old)
		(*s)->detection_status = DETECTION_STATUS_RATE_ONLY;
	if((*s)->audio_status < AUDIO_STATUS_UNAVAILABLE ||
		(*s)->audio_status > AUDIO_STATUS_OK) goto error;
	if((*s)->candidate_bph && ((*s)->candidate_bph < MIN_BPH ||
		(*s)->candidate_bph > MAX_BPH)) goto error;
	debug("serializer: checking events_tictoc\n");
	if((*s)->events_tictoc && (*s)->events_count && !(*s)->events)
		goto error;
	if(!(*s)->events_tictoc && (*s)->events_count)
		(*s)->events_tictoc = calloc((*s)->events_count, sizeof(*(*s)->events_tictoc));
	debug("serializer: checking sample_rate\n");
	if((*s)->sample_rate <= 0) goto error;
	debug("serializer: checking guessed_bph\n");
	if((*s)->guessed_bph < MIN_BPH || (*s)->guessed_bph > MAX_BPH) goto error;
	debug("serializer: checking rate\n");
	if((*s)->rate < -9999 || (*s)->rate > 9999) goto error;
	debug("serializer: checking beat error\n");
	if((*s)->be < 0 || (*s)->be > 99.9) goto error;
	debug("serializer: checking amplitude\n");
	if((*s)->amp < 0 || (*s)->amp > 360) goto error;
	if((*s)->trace_zoom <= 0)
		(*s)->trace_zoom = 1.0;
	// Files without a position (or with an unknown one) load as "not set"
	if((*s)->position < POS_NONE || (*s)->position >= POS_COUNT)
		(*s)->position = POS_NONE;
	debug("serializer: checking amplitudes\n");
	if((*s)->amps && (*s)->amps_count && !(*s)->amps_time)
		goto error;
	if(!(*s)->amps) {
		(*s)->amps_count = 0;
		(*s)->amps_wp = 0;
	}
	if((*s)->amps_count && (*s)->amps_wp >= (*s)->amps_count)
		goto error;
	(*s)->pb->events = NULL;
#ifdef DEBUG
	(*s)->pb->debug = NULL;
#endif
	return 0;

error:
	trend_free(*trend);
	*trend = NULL;
	free(*name);
	free((*s)->pb->waveform);
	free((*s)->pb);
	free((*s)->amps_time);
	free((*s)->amps);
	free((*s)->events_tictoc);
	free((*s)->events);
	free(*s);
	*s = NULL;
	*name = NULL;
	return 1;
}

static int scan_snapshot_list(FILE *f, struct snapshot ***s, char ***names, struct trend ***trends,
	uint64_t *cnt)
{
	debug("serializer: scanning snapshot list\n");
	uint64_t i;
	int n = 0;
	*s = NULL;
	*names = NULL;
	*trends = NULL;
	*cnt = 0;
	if(1 != fscanf(f, " A%"SCNu64";%n", &i, &n) || !n) goto error;
	*s = malloc(i*sizeof(struct snapshot *));
	*names = malloc(i*sizeof(char *));
	*trends = malloc(i*sizeof(struct trend *));
	if(i && (!*s || !*names || !*trends)) goto error;
	uint64_t j;
	for(j = 0; j < i; j++) {
		if(scan_snapshot(f, *s+*cnt, *names+*cnt, *trends+*cnt)) goto error;
		*cnt += !!(*s)[*cnt];
	}
	*s = realloc(*s, *cnt*sizeof(struct snapshot *));
	*names = realloc(*names, *cnt*sizeof(char *));
	*trends = realloc(*trends, *cnt*sizeof(struct trend *));
	return 0;
error:
	debug("serializer: error in snapshot list\n");
	for(; (*cnt)--;) {
		snapshot_destroy((*s)[*cnt]);
		free((*names)[*cnt]);
		trend_free((*trends)[*cnt]);
	}
	free(*s);
	free(*names);
	free(*trends);
	*s = NULL;
	*names = NULL;
	*trends = NULL;
	*cnt = 0;
	return 1;
}

int write_file(FILE *f, struct snapshot **s, char **names, struct trend **trends, uint64_t cnt)
{
	uint64_t i;
	if(make_label(f, "tg-timer-version")) return 1;
	if(serialize_string(f, VERSION)) return 1;
	if(make_label(f, "data")) return 1;
	if(serialize_struct_begin(f)) return 1;
	if(make_label(f, "snapshot-list")) return 1;
	if(0 > fprintf(f, "A%"PRIu64";\n", cnt)) return 1;
	for(i = 0; i < cnt; i++)
		if(serialize_snapshot(f, s[i], names[i], trends ? trends[i] : NULL)) return 1;
	if(serialize_struct_end(f)) return 1;
	return 0;
}

static int csv_write_number(FILE *f, double value, char decimal)
{
	char number[64];
	if(!g_ascii_formatd(number, sizeof(number), "%.3f", value))
		return 1;
	if(decimal != '.') {
		for(char *p = number; *p; p++)
			if(*p == '.') *p = decimal;
	}
	return fputs(number, f) == EOF;
}

int write_trend_csv(FILE *f, const struct trend *t, char separator, char decimal)
{
	const char *const headers[] = {
		_("Elapsed time (s)"), _("Position (code)"), _("Rate (s/day)"),
		_("Amplitude (degrees)"), _("Beat error (ms)"), _("Validity"),
		_("Beat rate (bph)"), _("Lift angle (degrees)"), _("Calibration (s/day)"),
		_("Sample rate (Hz)"), _("Algorithm")
	};
	static const char *const position_codes[POS_COUNT] = {
		"", "DU", "DD", "CU", "CD", "CL", "CR"
	};

	if(!f || !t || t->count < 0 || t->count > TREND_MAX_SAMPLES ||
			(t->count && !t->samples) ||
			(separator != ',' && separator != ';') ||
			(decimal != '.' && decimal != ',') ||
			(separator == ',' && decimal != '.') ||
			(separator == ';' && decimal != ','))
		return 1;

	if(fwrite("\xef\xbb\xbf", 1, 3, f) != 3)
		return 1;
	for(int i = 0; i < (int)(sizeof(headers) / sizeof(headers[0])); i++) {
		if((i && fputc(separator, f) == EOF) || fputs(headers[i], f) == EOF)
			return 1;
	}
	if(fputs("\r\n", f) == EOF)
		return 1;

	for(int i = 0; i < t->count; i++) {
		const struct trend_sample *x = &t->samples[i];
		if(x->position < POS_NONE || x->position >= POS_COUNT ||
				!isfinite(x->t) || !isfinite(x->rate) ||
				!isfinite(x->amp) || !isfinite(x->be))
			return 1;

		if(csv_write_number(f, x->t, decimal) || fputc(separator, f) == EOF ||
				fputs(position_codes[x->position], f) == EOF ||
				fputc(separator, f) == EOF ||
				csv_write_number(f, x->rate, decimal) || fputc(separator, f) == EOF)
			return 1;
		if(x->amp > 0 && csv_write_number(f, x->amp, decimal))
			return 1;
		const char *algorithm_name = t->algorithm_custom ?
				(t->is_light ? _("Light (custom)") : _("Full (custom)")) :
				(t->is_light ? _("Light") : _("Full"));
		if(fputc(separator, f) == EOF ||
				csv_write_number(f, x->be, decimal) ||
				fputc(separator, f) == EOF || fputs(x->settling ? _("Settling") : _("Valid"), f) == EOF ||
				fputc(separator, f) == EOF ||
				fprintf(f, "%d", t->bph) < 0 || fputc(separator, f) == EOF ||
				csv_write_number(f, t->la, decimal) || fputc(separator, f) == EOF ||
				csv_write_number(f, t->cal / 10.0, decimal) || fputc(separator, f) == EOF ||
				fprintf(f, "%d", t->nominal_sr) < 0 || fputc(separator, f) == EOF ||
				fprintf(f, "%s\r\n", algorithm_name) < 0)
			return 1;
	}
	return ferror(f) ? 1 : 0;
}

int read_file(FILE *f, struct snapshot ***s, char ***names, struct trend ***trends, uint64_t *cnt)
{
	debug("serializer: reading file\n");
	char lbl[LABEL_SIZE+1];
	char *l = lbl;
	int found_snapshot_list = 0;
	*cnt = 0;
	*s = NULL;
	*names = NULL;
	*trends = NULL;
	if(scan_label(f, l) || strcmp("tg-timer-version",l)) return 1;
	if(scan_string(f, &l, LABEL_SIZE, NULL)) return 1;
	debug("serializer: read version %s\n",l);
	if(scan_label(f, l) || strcmp("data",l)) return 1;
	debug("serializer: found data structure\n",l);
	int n = 0;
	if(0 != fscanf(f, " T;%n", &n) || !n) return 1;
	for(;;) {
		if(scan_label(f,l)) goto error;
		if(!strcmp("__end__",l)) break;
		if(!strcmp("snapshot-list",l)) {
			if(*s || scan_snapshot_list(f, s, names, trends, cnt)) goto error;
			found_snapshot_list = 1;
			continue;
		}
		if(eat_object(f)) goto error;
	}
	if(!found_snapshot_list) goto error;
	debug("serializer: end of data structure\n",l);
	char c;
	if(1 != fscanf(f, " %c", &c)) return 0;
#ifdef DEBUG
	debug("serializer: stray char %c (%d) after end\n", c, c);
#endif
error:
	debug("serializer: read error\n");
	if(*s) {
		uint64_t i;
		for(i = 0; i < *cnt; i++) {
			snapshot_destroy((*s)[i]);
			free((*names)[i]);
			trend_free((*trends)[i]);
		}
		free(*s);
		free(*names);
		free(*trends);
	}
	*s = NULL;
	*names = NULL;
	*trends = NULL;
	*cnt = 0;
	return 1;
}
