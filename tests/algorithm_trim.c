#include <assert.h>
#include <math.h>

#include "../src/algo.c"

double algorithm_parameters[ALG_COUNT];

int main(void)
{
	algorithm_parameters[ALG_TRIM] = 30;
	float one[] = {7};
	float two[] = {7, 3};
	float six[] = {1, 2, 3, 4, 5, 6};
	float eleven[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
	assert(tmean(one, 1) == 7);
	assert(tmean(two, 2) == 3);
	assert(tmean(six, 6) == 2.5f);
	assert(tmean(eleven, 11) == 4);

	algorithm_parameters[ALG_TRIM] = 100;
	float short_bin[] = {4, 2, 1};
	assert(tmean(short_bin, 3) == 1);

	float waveform[400] = {0};
	waveform[110] = 5;
	struct processing_buffers p = {0};
	p.sample_rate = 1000;
	p.period = 200;
	p.waveform = waveform;
	p.tic = 50;
	p.toc = 150;
	algorithm_parameters[ALG_AMP_SMOOTH] = 1;
	algorithm_parameters[ALG_AMP_NOISE] = 1.4;
	algorithm_parameters[ALG_AMP_LIMIT] = 20;
	algorithm_parameters[ALG_AMP_STEP] = 1.4;
	algorithm_parameters[ALG_AMP_INITIAL] = 0;
	compute_amplitude(&p, 52);
	assert(p.amp < 0);
	assert(p.detection_status != DETECTION_STATUS_OK);
	algorithm_parameters[ALG_AMP_INITIAL] = 1;
	algorithm_parameters[ALG_AMP_STEP] = 1;
	compute_amplitude(&p, 52);
	assert(p.amp < 0);
	assert(p.detection_status != DETECTION_STATUS_OK);

	float low_waveform[2000] = {0};
	for(int i = 0; i < 10; i++) {
		low_waveform[125 + i] = i + 1;
		low_waveform[625 + i] = i + 1;
	}
	p.waveform = low_waveform;
	p.period = 1000;
	p.tic = 250;
	p.toc = 750;
	algorithm_parameters[ALG_AMP_MIN] = 100;
	algorithm_parameters[ALG_AMP_DIFFERENCE] = 100;
	compute_amplitude(&p, 52);
	assert(p.amp < 0);
	assert(p.detection_status == DETECTION_STATUS_AMPLITUDE_TOO_LOW);
	algorithm_parameters[ALG_AMP_MIN] = 60;
	compute_amplitude(&p, 52);
	assert(p.amp > 0);
	assert(p.detection_status == DETECTION_STATUS_OK);
	return 0;
}
