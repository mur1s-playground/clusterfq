#pragma once

#ifdef _WIN32
#include <windows.h>
#include <comdef.h>
#else
#endif

#include "../clusterfq/mutex.h"

struct audio_dst {
	int device_id;
	bool copy_to_gmb;
	
	struct mutex out_lock;
	int last_id;

#ifdef _WIN32
	HWAVEOUT wave_out_handle;
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

struct audio_dst_loop_params {
	struct audio_dst* as;
	int thread_id;
};

void audio_dsts_list_devices();
void audio_dst_connect(struct audio_dst* as, int device_id, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots);

void audio_dst_loop(void* param);