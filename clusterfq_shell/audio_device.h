#pragma once

#ifndef AUDIO_DEVICE_PACKETSIZE
#define AUDIO_DEVICE_PACKETSIZE 1024
#endif

struct audio_device {
	int id;
	char* name;
	int channels;
};

#ifdef _WIN32
#include <windows.h>

struct audio_format {
	WAVEFORMATEX wave_format;
};

#else
#include "/usr/include/alsa/asoundlib.h"

struct audio_format {
	snd_pcm_format_t pcm_format;
	unsigned int channels;
	unsigned int rate;
	unsigned int buffer_size;
	unsigned int period_size;
	int period_event;
};

int audio_device_set_hw_params(snd_pcm_t* handle, snd_pcm_hw_params_t* params, struct audio_format* af);
int audio_device_set_sw_params(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams, struct audio_format* af);
#endif

void audio_device_set_format(struct audio_format *af, int channels, int samples_per_sec, int bits_per_sample);