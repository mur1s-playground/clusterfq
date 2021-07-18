#include "audio_source.h"
#include "audio_device.h"

#include "smb_interface.h"

#include "../clusterfq/shared_memory_buffer.h"
#include "../clusterfq/util.h"

#include "../clusterfq_cl/clusterfq_cl.h"


#include <iostream>

vector<struct audio_device> audio_source_devices;

void audio_source_init_available_devices() {
	audio_source_devices.clear();

#ifdef _WIN32
	unsigned int num_devices = waveInGetNumDevs();

	for (int i = 0; i < num_devices; i++) {
		struct audio_device ad;

		WAVEINCAPS device_capabilities = {};
		waveInGetDevCaps(i, &device_capabilities, sizeof(device_capabilities));
		ad.id = i;

		_bstr_t b(device_capabilities.szPname);
		const char* name = b;

		ad.name = new char[strlen(name) + 1];
		snprintf(ad.name, strlen(name) + 1, "%s", name);

		_bstr_t c(device_capabilities.wChannels);
		const char* channels = c;
		ad.channels = stoi(channels);

		audio_source_devices.push_back(ad);
	}
#else 
	char* tmp = util_issue_command("arecord -L");
	std::cout << std::endl << tmp << std::endl;
	free(tmp);
#endif
}

void audio_source_list_devices() {
	audio_source_init_available_devices();
	std::cout << std::endl;
	for (int i = 0; i < audio_source_devices.size(); i++) {
		std::cout << audio_source_devices[i].id << ": " << audio_source_devices[i].name << ", " << audio_source_devices[i].channels << std::endl;
	}
}

#ifdef _WIN32
void audio_source_prepare_hdr(struct audio_source* as, int id);
#endif

#ifdef _WIN32
void audio_source_connect(struct audio_source* as, int device_id, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots) {
#else
void audio_source_connect(struct audio_source* as, string device_name, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots) {
#endif
	audio_source_init_available_devices();
	as->lp = lossy_pipes_by_id.find(lossy_pipe_id)->second;
	as->lp_size = size;
	as->lp_slots = slots;

#ifdef _WIN32
	as->device_id = device_id;
#else
	as->device_name = device_name;
#endif

	audio_device_set_format(&as->wave_format, channels, samples_per_sec, bits_per_sample);

	as->buffer = (unsigned char*) malloc(slots * AUDIO_DEVICE_PACKETSIZE);
	for (int s = 0; s < slots; s++) {
		int* len = (int *)&as->buffer[s * AUDIO_DEVICE_PACKETSIZE];
		*len = AUDIO_DEVICE_PACKETSIZE;
	}

#ifdef _WIN32
	as->wave_status = waveInOpen(&as->wave_in_handle, device_id, &as->wave_format.wave_format, 0, 0, CALLBACK_NULL);

	if (as->wave_status != MMSYSERR_NOERROR) {
		std::cout << "wave status not ok: " << as->wave_status << std::endl;
	} else {
		std::cout << "wave status ok" << std::endl;
	}

	as->wave_header_arr = new WAVEHDR[as->lp_slots];
	for (int wh = 0; wh < as->lp_slots; wh++) {
		audio_source_prepare_hdr(as, wh);
	}
#else
	int err = 0;

	if ((err = snd_pcm_open(&as->wave_in_handle, as->device_name.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		std::cout << "error opening device" << std::endl;
		return;
	}
	snd_pcm_hw_params_alloca(&as->wave_in_hw_params);

	if (audio_device_set_hw_params(as->wave_in_handle, as->wave_in_hw_params, &as->wave_format) < 0) {
		std::cout << "error setting hw params" << std::endl;
		return;
	}
#endif

	struct audio_source_loop_params* aslp = new struct audio_source_loop_params();
	aslp->as = as;
	//aslp->thread_id = thread_create(&main_thread_pool, &audio_source_loop, (void *)aslp);
	audio_source_loop(aslp);
}

#ifdef _WIN32
void audio_source_prepare_hdr(struct audio_source* as, int id) {
	as->wave_header_arr[id].lpData = (LPSTR)&as->buffer[id * AUDIO_DEVICE_PACKETSIZE + 3 * sizeof(int)];
	as->wave_header_arr[id].dwBufferLength = AUDIO_DEVICE_PACKETSIZE - 3 * sizeof(int);
	as->wave_header_arr[id].dwBytesRecorded = 0;
	as->wave_header_arr[id].dwUser = 0L;
	as->wave_header_arr[id].dwFlags = 0L;
	as->wave_header_arr[id].dwLoops = 0L;

	MMRESULT rc = waveInPrepareHeader(as->wave_in_handle, &as->wave_header_arr[id], sizeof(WAVEHDR));

	if (rc != MMSYSERR_NOERROR) {
		as->wave_status = rc;
		std::cout << "header prop failed" << std::endl;
	} else {
		rc = waveInAddBuffer(as->wave_in_handle, &as->wave_header_arr[id], sizeof(WAVEHDR));
	}
}
#endif

void audio_source_loop(void* param) {
	std::cout << "audio_source_loop_started: " << AUDIO_DEVICE_PACKETSIZE <<std::endl;

	struct audio_source_loop_params* aslp = (struct audio_source_loop_params*)param;
	struct audio_source* as = aslp->as;

	as->smb_last_used_id = 0;

	int ct = 0;

#ifdef _WIN32
	as->wave_status = waveInStart(as->wave_in_handle);
	while (as->wave_status == MMSYSERR_NOERROR) {
#else
	int format_bytes = as->wave_format.channels * snd_pcm_format_physical_width(as->wave_format.pcm_format) / 8;

	int frames_per_packet = (AUDIO_DEVICE_PACKETSIZE - 3 * sizeof(int)) / format_bytes;
	std::cout << "frames per packet: " << frames_per_packet << std::endl;

	int err = snd_pcm_start(as->wave_in_handle);
	if (err < 0) {
		std::cout << "error starting pcm" << std::endl;
	}

	while (1) {
#endif

#ifdef _WIN32
		do {
			util_sleep(1);
		} while (waveInUnprepareHeader(as->wave_in_handle, &as->wave_header_arr[as->smb_last_used_id], sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
#else

#endif
		int* ct_ptr = (int*)&as->buffer[as->smb_last_used_id * AUDIO_DEVICE_PACKETSIZE + sizeof(int)];
		*ct_ptr = ct;

		int* br_ptr = (int*)&as->buffer[as->smb_last_used_id * AUDIO_DEVICE_PACKETSIZE + 2 * sizeof(int)];
#ifdef _WIN32
		*br_ptr = (int)as->wave_header_arr[as->smb_last_used_id].dwBytesRecorded;
#else
		int read_f = 0;
		while (read_f < frames_per_packet) {
			err = snd_pcm_readi(as->wave_in_handle, &as->buffer[as->smb_last_used_id * AUDIO_DEVICE_PACKETSIZE + 3 * sizeof(int) + read_f * format_bytes], frames_per_packet - read_f);
			if (err == -EPIPE) {
				std::cout << "overrun occurred" << std::endl;
				snd_pcm_prepare(as->wave_in_handle);
			} else if (err == -ESTRPIPE) {
				std::cout << "-estripe" << std::endl;
				while ((err = snd_pcm_resume(as->wave_in_handle)) == -EAGAIN)
					sleep(1);
				if (err < 0) {
					err = snd_pcm_prepare(as->wave_in_handle);
					if (err < 0)
						std::cout << "Can't recovery from suspend, prepare failed: " << snd_strerror(err) << std::endl;
				}
			} else if (err < 0) {
				std::cout << "error from read: " << snd_strerror(err) << std::endl;
				break;
			} else {
				read_f += err;
			}
		}
		*br_ptr = frames_per_packet * format_bytes;
#endif
		int c_id = 0;
		do {
			c_id = ClusterFQ::ClusterFQ::shared_memory_buffer_write_slot_i(as->lp, as->smb_last_used_id, &as->buffer[as->smb_last_used_id * AUDIO_DEVICE_PACKETSIZE], 3 * sizeof(int) + *br_ptr);
			if (c_id == -1) util_sleep(1);
		} while (c_id == -1);


		int next_id = (c_id + 1) % as->lp_slots;

		if (next_id != (as->smb_last_used_id + 1) % as->lp_slots) std::cout << "skipping" << std::endl;
#ifdef _WIN32
		as->wave_status = waveInPrepareHeader(as->wave_in_handle, &as->wave_header_arr[as->smb_last_used_id], sizeof(WAVEHDR));

		if (as->wave_status == MMSYSERR_NOERROR) {
			as->wave_status = waveInAddBuffer(as->wave_in_handle, &as->wave_header_arr[as->smb_last_used_id], sizeof(WAVEHDR));
		}
#endif

		as->smb_last_used_id = next_id;
		ct++;
	}

	std::cout << "audio_source_loop_ended" << std::endl;
	//thread_terminated(&main_thread_pool, aslp->thread_id);
	free(aslp);
}
