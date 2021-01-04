#ifndef GLOBAL_DEF_H
#define GLOBAL_DEF_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

inline long long getCurrentTime();

long long getCurrentTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000) + tv.tv_usec/1000;
}

#define	SAMPLERATE	16000
#define	IN_CHANNELS	8
#define	OUT_CHANNELS	1
#define	OUT_8CHANNELS	8
#define	SAMPLEBYTES	2
#define	SAMPLENUM	160	//20ms @ 16kHz

#if ONE_AEC
#define	NEAR_IN_CH	1
#else
#define	NEAR_IN_CH	6
#endif

#define	AEC_OUT_CH	NEAR_IN_CH
#define	REF_CH_NUM	6
#define	PRIORITY_URGENT_AUDIO -19
#define	SP_FOREGROUND 1

#define OUTPUT_GAIN_TO_INT16 (32768.0f)
#define SAT32(a) ((a) > 32767 ? 32767 : ((a) < -32768 ? -32768 : (a)))

#endif
