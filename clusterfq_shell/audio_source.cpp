#include "audio_source.h"
#include "audio_device.h"

#include "smb_interface.h"

#include "../clusterfq/shared_memory_buffer.h"

#include "../clusterfq_cl/clusterfq_cl.h"

#include <iostream>

vector<struct audio_device> audio_source_devices;

void audio_source_init_available_devices() {
	audio_source_devices.clear();

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
}

void audio_source_list_devices() {
	audio_source_init_available_devices();
	std::cout << std::endl;
	for (int i = 0; i < audio_source_devices.size(); i++) {
		std::cout << audio_source_devices[i].id << ": " << audio_source_devices[i].name << ", " << audio_source_devices[i].channels << std::endl;
	}
}

void CALLBACK audio_source_process_callback(
	HWAVEIN   hwi,
	UINT      uMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2) {
	if (uMsg == WIM_CLOSE) {

	}
	else if (uMsg == WIM_DATA) {

	}
	else if (uMsg == WIM_CLOSE) {

	}
}

void audio_source_prepare_hdr(struct audio_source* as, int id);

void audio_source_connect(struct audio_source* as, int device_id, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots) {
	audio_source_init_available_devices();
	as->lp = lossy_pipes_by_id.find(lossy_pipe_id)->second;
	as->lp_size = size;
	as->lp_slots = slots;

	as->device_id = device_id;

	as->wave_format.wFormatTag = WAVE_FORMAT_PCM;
	as->wave_format.nChannels = channels;
	as->wave_format.nSamplesPerSec = samples_per_sec;
	as->wave_format.wBitsPerSample = bits_per_sample;
	as->wave_format.nBlockAlign = (channels * bits_per_sample) / 8;
	as->wave_format.nAvgBytesPerSec = (channels * samples_per_sec * bits_per_sample) / 8;
	as->wave_format.cbSize = 0;

	as->buffer = (unsigned char*) malloc(slots * AUDIO_DEVICE_PACKETSIZE);
	for (int s = 0; s < slots; s++) {
		int* len = (int *)&as->buffer[s * AUDIO_DEVICE_PACKETSIZE];
		*len = AUDIO_DEVICE_PACKETSIZE;
	}

	//as->smb_size_req = channels * (bits_per_sample / 8) * samples_per_sec;

	as->wave_status = waveInOpen(&as->wave_in_handle, device_id, &as->wave_format, (DWORD_PTR)&audio_source_process_callback, (DWORD_PTR)(void*)as, CALLBACK_FUNCTION);

	if (as->wave_status != MMSYSERR_NOERROR) {
		std::cout << "wave status not ok: " << as->wave_status << std::endl;
	} else {
		std::cout << "wave status ok" << std::endl;
	}

	as->wave_header_arr = new WAVEHDR[as->lp_slots];
	for (int wh = 0; wh < as->lp_slots; wh++) {
		audio_source_prepare_hdr(as, wh);
	}
	
	struct audio_source_loop_params* aslp = new struct audio_source_loop_params();
	aslp->as = as;
	//aslp->thread_id = thread_create(&main_thread_pool, &audio_source_loop, (void *)aslp);
	audio_source_loop(aslp);
}

void audio_source_prepare_hdr(struct audio_source* as, int id) {
	as->wave_header_arr[id].lpData = (LPSTR)&as->buffer[id * AUDIO_DEVICE_PACKETSIZE + sizeof(int)];
	as->wave_header_arr[id].dwBufferLength = AUDIO_DEVICE_PACKETSIZE - sizeof(int);
	as->wave_header_arr[id].dwBytesRecorded = 0;
	as->wave_header_arr[id].dwUser = 0L;
	as->wave_header_arr[id].dwFlags = 0L;
	as->wave_header_arr[id].dwLoops = 0L;

	MMRESULT rc = waveInPrepareHeader(as->wave_in_handle, &as->wave_header_arr[id], sizeof(WAVEHDR));

	if (rc != MMSYSERR_NOERROR) {
		as->wave_status = rc;
		std::cout << "header prep not ok" << std::endl;
	} else {
		std::cout << "header prep ok" << std::endl;
		rc = waveInAddBuffer(as->wave_in_handle, &as->wave_header_arr[id], sizeof(WAVEHDR));
	}
}

void audio_source_loop(void* param) {
	std::cout << "audio_source_loop_started: " << AUDIO_DEVICE_PACKETSIZE <<std::endl;

	struct audio_source_loop_params* aslp = (struct audio_source_loop_params*)param;
	struct audio_source* as = aslp->as;

	as->smb_last_used_id = 0;

	as->wave_status = waveInStart(as->wave_in_handle);

	while (as->wave_status == MMSYSERR_NOERROR) {
		do {
			Sleep(33);
		} while (waveInUnprepareHeader(as->wave_in_handle, &as->wave_header_arr[as->smb_last_used_id], sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);

		int next_id = (ClusterFQ::ClusterFQ::shared_memory_buffer_write_slot_i(as->lp, as->smb_last_used_id, &as->buffer[as->smb_last_used_id * AUDIO_DEVICE_PACKETSIZE], AUDIO_DEVICE_PACKETSIZE) + 1) % as->lp_slots;

		if (next_id != (as->smb_last_used_id + 1) % as->lp_slots) std::cout << "skipping" << std::endl;

		as->wave_status = waveInPrepareHeader(as->wave_in_handle, &as->wave_header_arr[as->smb_last_used_id], sizeof(WAVEHDR));

		if (as->wave_status == MMSYSERR_NOERROR) {
			as->wave_status = waveInAddBuffer(as->wave_in_handle, &as->wave_header_arr[as->smb_last_used_id], sizeof(WAVEHDR));
		}

		as->smb_last_used_id = next_id;
	}
	std::cout << "audio_source_loop_ended" << std::endl;
	//thread_terminated(&main_thread_pool, aslp->thread_id);
	free(aslp);
}
