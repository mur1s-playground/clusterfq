#pragma once

#ifdef _WIN32
#include <windows.h>
#include <comdef.h>
#else

#endif

#include "audio_device.h"

#include <vector>

#ifdef _WIN32

#else
#include <string>
#endif

using namespace std;

struct audio_source {
#ifdef _WIN32
	int device_id;
#else
	string device_name;
#endif
	bool copy_to_gmb;

	struct audio_format wave_format;

#ifdef _WIN32
	HWAVEIN wave_in_handle;

	WAVEHDR* wave_header_arr;

	MMRESULT wave_status;
#else
	snd_pcm_t* wave_in_handle;

	snd_pcm_hw_params_t* wave_in_hw_params;
	snd_pcm_sw_params_t* wave_in_sw_params;
#endif

	unsigned char* buffer;

	void* lp;
	int lp_size;
	int lp_slots;
	
	int smb_last_used_id;
};

struct audio_source_loop_params {
	struct audio_source* as;
	int thread_id;
};

extern vector<struct audio_device> audio_source_devices;

void audio_source_list_devices();

void audio_source_init_available_devices();
void audio_source_loop(void* param);

#ifdef _WIN32
void audio_source_connect(struct audio_source* as, int device_id, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots);
#else
void audio_source_connect(struct audio_source* as, string device_name, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots);
#endif