#pragma once

#ifdef _WIN32
#include <windows.h>
#include <comdef.h>
#else
#include "/usr/include/alsa/asoundlib.h"
#include <string>
using namespace std;
#endif

#include "../clusterfq/mutex.h"

#ifdef _WIN32
#else
struct audio_format {
	snd_pcm_format_t pcm_format;
	unsigned int channels;
	unsigned int rate;
	unsigned int buffer_size;
	unsigned int period_size;
	int period_event;
};
#endif

struct audio_dst {
#ifdef _WIN32
	int device_id;
#else
	string device_name;
#endif

	bool copy_to_gmb;
	
	struct mutex out_lock;
	int last_id;

#ifdef _WIN32
	HWAVEOUT wave_out_handle;
	WAVEFORMATEX wave_format;

	WAVEHDR* wave_header_arr;

	MMRESULT wave_status;
#else
	snd_pcm_t *wave_out_handle;

	struct audio_format wave_out_format;

	snd_pcm_hw_params_t *wave_out_hw_params;
	snd_pcm_sw_params_t *wave_out_sw_params;


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

#ifdef _WIN32
void audio_dst_connect(struct audio_dst* as, int device_id, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots);
#else
void audio_dst_connect(struct audio_dst* as, string device_name, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots);
#endif

void audio_dst_loop(void* param);