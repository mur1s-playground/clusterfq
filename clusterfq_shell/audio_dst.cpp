#include "audio_dst.h"

#include "audio_device.h"

#include "smb_interface.h"

#include "../clusterfq/shared_memory_buffer.h"
#include "../clusterfq/util.h"

#include "../clusterfq_cl/clusterfq_cl.h"

#include <vector>
#include <iostream>

#ifdef _WIN32
#else
#include "../clusterfq/util.h"
#endif



using namespace std;

vector<struct audio_device> audio_dst_devices;

void audio_dst_init_available_devices() {
	audio_dst_devices.clear();

#ifdef _WIN32
	unsigned int num_devices = waveOutGetNumDevs();

	for (int i = 0; i < num_devices; i++) {
		struct audio_device ad;

		WAVEOUTCAPSA device_capabilities = {};
		waveOutGetDevCapsA(i, &device_capabilities, sizeof(device_capabilities));
		ad.id = i;

		const char* name = device_capabilities.szPname;

		ad.name = new char[strlen(name) + 1];
		snprintf(ad.name, strlen(name) + 1, "%s", name);
		
		ad.channels = device_capabilities.wChannels;

		audio_dst_devices.push_back(ad);
	}
#else
	char *tmp = util_issue_command("aplay -L");
	std::cout << std::endl << tmp << std::endl;
	free(tmp);
#endif
}

void audio_dsts_list_devices() {
	audio_dst_init_available_devices();
	std::cout << std::endl;
	for (int i = 0; i < audio_dst_devices.size(); i++) {
		std::cout << audio_dst_devices[i].id << ": " << audio_dst_devices[i].name << ", " << audio_dst_devices[i].channels << std::endl;
	}
}

#ifdef _WIN32
void CALLBACK audio_dst_process_callback(
	HWAVEIN   hwi,
	UINT      uMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2) {

	struct audio_dst* as = (struct audio_dst*)dwInstance;
	if (uMsg == WOM_DONE) {
		int l_id = as->last_id;
		mutex_release(&as->out_lock);
		waveOutUnprepareHeader(as->wave_out_handle, &as->wave_header_arr[l_id], sizeof(WAVEHDR));
		waveOutPrepareHeader(as->wave_out_handle, &as->wave_header_arr[l_id], sizeof(WAVEHDR));
	}
}
#endif

#ifdef _WIN32
void audio_dst_prepare_hdr(struct audio_dst* as, int id);
#else
int audio_dst_set_hw_params(struct audio_dst *as) {
	snd_pcm_t* handle = as->wave_out_handle;
	snd_pcm_hw_params_t* params = as->wave_out_hw_params;
	int err = 0;

	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		std::cout << "no playback configuration" << std::endl;
		return err;
	}

	err = snd_pcm_hw_params_set_rate_resample(handle, params, 1);
	if (err < 0) {
		std::cout << "resampling setup failed for playback: " << snd_strerror(err) << std::endl;
		return err;
	}

	err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		std::cout << "err setting acces mode" << std::endl;
		return err;
	}

	err = snd_pcm_hw_params_set_format(handle, params, as->wave_out_format.pcm_format);
	if (err < 0) {
		std::cout << "err setting format" << std::endl;
		return err;
	}

	unsigned int rrate = as->wave_out_format.rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
	if (err < 0) {
		std::cout << "err setting rate" << std::endl;
		return err;
	}
	if (rrate != as->wave_out_format.rate) {
		std::cout << "err not as requested " << rrate << " != " << as->wave_out_format.rate << std::endl;
		return -1;
	}
	std::cout << "rate: " << rrate << std::endl;

	unsigned int buffer_time = 500000;
	int dir;
	err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
	if (err < 0) {
		std::cout << "err unable to set buffer time" << std::endl;
		return err;
	}

	snd_pcm_uframes_t size;
	err = snd_pcm_hw_params_get_buffer_size(params, &size);
	if (err < 0) {
		std::cout << "err get buffer size for playback: " << snd_strerror(err) << std::endl;
		return err;
	}
	as->wave_out_format.buffer_size = size;

	unsigned int period_time = 200000;
	err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
	if (err < 0) {
		std::cout << "err set period time for playback: " << snd_strerror(err) << std::endl;
		return err;
	}

	err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
	if (err < 0) {
		std::cout << "err to get period size for playback: " << snd_strerror(err) << std::endl;
		return err;
	}
	as->wave_out_format.period_size = size;
	std::cout << "period_size: " << size << std::endl;

	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		std::cout << "err unable to set hw params for playback: " << snd_strerror(err) << std::endl;
		return err;
	}
	
	return 0;
}

int audio_dst_set_sw_params(struct audio_dst* as) {
	snd_pcm_t* handle = as->wave_out_handle;
	snd_pcm_sw_params_t* swparams = as->wave_out_sw_params;
	int err = 0;

	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		std::cout << "err unable to determine current swparams for playback: " << snd_strerror(err) << std::endl;
		return err;
	}

	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (as->wave_out_format.buffer_size / as->wave_out_format.period_size) * as->wave_out_format.period_size);
	if (err < 0) {
		std::cout << "err unable to set start threshold mode for playback: " << snd_strerror(err) << std::endl;
		return err;
	}

	/* allow the transfer when at least period_size samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, as->wave_out_format.period_event ? as->wave_out_format.buffer_size : as->wave_out_format.period_size);
	if (err < 0) {
		std::cout << "err unable to set avail min for playback: " << snd_strerror(err) << std::endl;
		return err;
	}
	/* enable period events when requested */
	if (as->wave_out_format.period_event) {
		err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
		if (err < 0) {
			std::cout << "err unable to set period event: " << snd_strerror(err) << std::endl;
			return err;
		}
	}

	/* write the parameters to the playback device */
	err = snd_pcm_sw_params(handle, swparams);
	if (err < 0) {
		std::cout << "err unable to set sw params for playback: " << snd_strerror(err) << std::endl;
		return err;
	}
	return 0;
}

int audio_dst_xrun_recovery(snd_pcm_t* handle, int err) {
	std::cout << "stream recovery" << std::endl;
	if (err == -EPIPE) {
		err = snd_pcm_prepare(handle);
		if (err < 0)
			std::cout << "Can't recovery from underrun, prepare failed: " <<  snd_strerror(err) << std::endl;
		return 0;
	} else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			sleep(1);
		if (err < 0) {
			err = snd_pcm_prepare(handle);
			if (err < 0)
				std::cout << "Can't recovery from suspend, prepare failed: " <<  snd_strerror(err) << std::endl;
		}
		return 0;
	}
	return err;
}
#endif

#ifdef _WIN32
void audio_dst_connect(struct audio_dst* as, int device_id, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots) {
#else
void audio_dst_connect(struct audio_dst* as, string device_name, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots) {
#endif
	audio_dst_init_available_devices();
	as->lp = lossy_pipes_by_id.find(lossy_pipe_id)->second;
	as->lp_size = size;
	as->lp_slots = slots;

#ifdef _WIN32
	as->device_id = device_id;
#else
	as->device_name = device_name;
#endif

	mutex_init(&as->out_lock);
	as->last_id = -1;

#ifdef _WIN32
	as->wave_format.wFormatTag = WAVE_FORMAT_PCM;
	as->wave_format.nChannels = channels;
	as->wave_format.nSamplesPerSec = samples_per_sec;
	as->wave_format.wBitsPerSample = bits_per_sample;
	as->wave_format.nBlockAlign = (channels * bits_per_sample) / 8;
	as->wave_format.nAvgBytesPerSec = (channels * samples_per_sec * bits_per_sample) / 8;
	as->wave_format.cbSize = 0;
#else
	as->device_name = device_name;

	if (bits_per_sample == 8) {
		as->wave_out_format.pcm_format = SND_PCM_FORMAT_U8;
	} else if (bits_per_sample == 16) {
		as->wave_out_format.pcm_format = SND_PCM_FORMAT_S16_LE;
	}
	as->wave_out_format.channels = channels;
	as->wave_out_format.rate = samples_per_sec;
	as->wave_out_format.period_event = 0;
#endif

	as->buffer = (unsigned char*)malloc(slots * 2 * AUDIO_DEVICE_PACKETSIZE);
	for (int s = 0; s < slots * 2; s++) {
		int* len = (int*)&as->buffer[s * AUDIO_DEVICE_PACKETSIZE];
		*len = AUDIO_DEVICE_PACKETSIZE;
	}

#ifdef _WIN32
	as->wave_status = waveOutOpen(&as->wave_out_handle, device_id, &as->wave_format, (DWORD_PTR)&audio_dst_process_callback, (DWORD_PTR)(void*)as, CALLBACK_FUNCTION);

	if (as->wave_status != MMSYSERR_NOERROR) {
		std::cout << "wave status not ok: " << as->wave_status << std::endl;
	} else {
		std::cout << "wave status ok" << std::endl;
	}

	as->wave_header_arr = new WAVEHDR[as->lp_slots * 2];
	for (int wh = 0; wh < as->lp_slots * 2; wh++) {
		audio_dst_prepare_hdr(as, wh);
	}
#else
	int err = 0;
	
	if ((err = snd_pcm_open(&as->wave_out_handle, as->device_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
		std::cout << "error opening device" << std::endl;
		return;
	}
	snd_pcm_hw_params_alloca(&as->wave_out_hw_params);
	
	if (audio_dst_set_hw_params(as) < 0) {
		std::cout << "error setting hw params" << std::endl;
		return;
	}
	
	snd_pcm_sw_params_alloca(&as->wave_out_sw_params);
	if (audio_dst_set_sw_params(as) < 0) {
		std::cout << "error setting sw params" << std::endl;
		return;
	}

#endif

	struct audio_dst_loop_params* aslp = new struct audio_dst_loop_params();
	aslp->as = as;
	//aslp->thread_id = thread_create(&main_thread_pool, &audio_source_loop, (void *)aslp);
	audio_dst_loop(aslp);

}

#ifdef _WIN32
void audio_dst_prepare_hdr(struct audio_dst* as, int id) {
	as->wave_header_arr[id].lpData = (LPSTR)&as->buffer[id * AUDIO_DEVICE_PACKETSIZE + 2 * sizeof(int) + sizeof(unsigned long)];
	as->wave_header_arr[id].dwBufferLength = AUDIO_DEVICE_PACKETSIZE - 2 * sizeof(int) - sizeof(unsigned long);
	as->wave_header_arr[id].dwBytesRecorded = 0;
	as->wave_header_arr[id].dwUser = 0L;
	as->wave_header_arr[id].dwFlags = 0L;
	as->wave_header_arr[id].dwLoops = 0L;

	MMRESULT rc = waveOutPrepareHeader(as->wave_out_handle, &as->wave_header_arr[id], sizeof(WAVEHDR));
}
#endif

void audio_dst_loop(void* param) {
	struct audio_dst_loop_params* aslp = (struct audio_dst_loop_params*)param;
	struct audio_dst* as = aslp->as;

	std::cout << "audio_dst_loop_started: " << AUDIO_DEVICE_PACKETSIZE << std::endl;

	as->smb_last_used_id = 0;
	
	int delay = 0;
	bool reprepared = true;

	int local_slot_count = 0;

	int last_ct = 0;

#ifdef _WIN32
	while (as->wave_status == MMSYSERR_NOERROR) {
#else
	int format_bytes = as->wave_out_format.channels * snd_pcm_format_physical_width(as->wave_out_format.pcm_format) / 8;

	while (1) {
#endif
		if (ClusterFQ::ClusterFQ::shared_memory_buffer_read_slot_i(as->lp, as->smb_last_used_id, &as->buffer[local_slot_count * AUDIO_DEVICE_PACKETSIZE])) {
			int* ct_src = (int*)&as->buffer[local_slot_count * AUDIO_DEVICE_PACKETSIZE + sizeof(int)];
			unsigned long* br_ptr = (unsigned long*)&as->buffer[local_slot_count * AUDIO_DEVICE_PACKETSIZE + 2 * sizeof(int)];

#ifdef _WIN32
			as->wave_header_arr[local_slot_count].dwBytesRecorded = *br_ptr;
#endif

			if (last_ct > *ct_src) continue;
			last_ct = *ct_src;
			if (as->last_id > -1) {
				mutex_wait_for(&as->out_lock);
			}
#ifdef _WIN32
			as->wave_status = waveOutWrite(as->wave_out_handle, &as->wave_header_arr[local_slot_count], sizeof(WAVEHDR));
#else
			char* ptr = (char*)&as->buffer[local_slot_count * AUDIO_DEVICE_PACKETSIZE + 2 * sizeof(int) + sizeof(unsigned long)];
			int cptr = (int)*br_ptr / format_bytes;
			int err = 0;
			while (cptr > 0) {
				err = snd_pcm_writei(as->wave_out_handle, ptr, cptr);
				if (err == -EAGAIN)
					continue;
				if (err < 0) {
					if (audio_dst_xrun_recovery(as->wave_out_handle, err) < 0 ) {
						std::cout << "write error: " << snd_strerror(err) << std::endl;
						return;
					}
					break;
				}
				ptr += err * format_bytes;
				cptr -= err;
			}
			mutex_release(&as->out_lock);
#endif

			as->last_id = local_slot_count;
			local_slot_count = (local_slot_count + 1) % (as->lp_slots * 2);
			as->smb_last_used_id = (as->smb_last_used_id + 1) % as->lp_slots;
			
		} else {
			util_sleep(1);
		}
	}

	std::cout << "audio_dst_loop_ended: " << std::endl;
}