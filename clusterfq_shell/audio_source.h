#pragma once

#ifdef _WIN32
#include <windows.h>
#include <comdef.h>
#else

#endif

#include <vector>

using namespace std;

struct audio_source {
	int device_id;
	bool copy_to_gmb;
#ifdef _WIN32
	HWAVEIN wave_in_handle;
	WAVEFORMATEX wave_format;

	WAVEHDR* wave_header_arr;

	MMRESULT wave_status;
#else

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

void audio_source_connect(struct audio_source* as, int device_id, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots);