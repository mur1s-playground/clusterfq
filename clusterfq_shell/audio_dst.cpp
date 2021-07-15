#include "audio_dst.h"

#include "audio_device.h"

#include "smb_interface.h"

#include "../clusterfq/shared_memory_buffer.h"
#include "../clusterfq/util.h"

#include "../clusterfq_cl/clusterfq_cl.h"

#include <vector>
#include <iostream>



using namespace std;

vector<struct audio_device> audio_dst_devices;

void audio_dst_init_available_devices() {
	audio_dst_devices.clear();

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
}

void audio_dsts_list_devices() {
	audio_dst_init_available_devices();
	std::cout << std::endl;
	for (int i = 0; i < audio_dst_devices.size(); i++) {
		std::cout << audio_dst_devices[i].id << ": " << audio_dst_devices[i].name << ", " << audio_dst_devices[i].channels << std::endl;
	}
}

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

void audio_dst_prepare_hdr(struct audio_dst* as, int id);

void audio_dst_connect(struct audio_dst* as, int device_id, int channels, int samples_per_sec, int bits_per_sample, int lossy_pipe_id, int size, int slots) {
	audio_dst_init_available_devices();
	as->lp = lossy_pipes_by_id.find(lossy_pipe_id)->second;
	as->lp_size = size;
	as->lp_slots = slots;

	as->device_id = device_id;

	mutex_init(&as->out_lock);
	as->last_id = -1;

	as->wave_format.wFormatTag = WAVE_FORMAT_PCM;
	as->wave_format.nChannels = channels;
	as->wave_format.nSamplesPerSec = samples_per_sec;
	as->wave_format.wBitsPerSample = bits_per_sample;
	as->wave_format.nBlockAlign = (channels * bits_per_sample) / 8;
	as->wave_format.nAvgBytesPerSec = (channels * samples_per_sec * bits_per_sample) / 8;
	as->wave_format.cbSize = 0;

	as->buffer = (unsigned char*)malloc(slots * 2 * AUDIO_DEVICE_PACKETSIZE);
	for (int s = 0; s < slots * 2; s++) {
		int* len = (int*)&as->buffer[s * AUDIO_DEVICE_PACKETSIZE];
		*len = AUDIO_DEVICE_PACKETSIZE;
	}

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

	struct audio_dst_loop_params* aslp = new struct audio_dst_loop_params();
	aslp->as = as;
	//aslp->thread_id = thread_create(&main_thread_pool, &audio_source_loop, (void *)aslp);
	audio_dst_loop(aslp);

}

void audio_dst_prepare_hdr(struct audio_dst* as, int id) {
	as->wave_header_arr[id].lpData = (LPSTR)&as->buffer[id * AUDIO_DEVICE_PACKETSIZE + 2 * sizeof(int) + sizeof(unsigned long)];
	as->wave_header_arr[id].dwBufferLength = AUDIO_DEVICE_PACKETSIZE - 2 * sizeof(int) - sizeof(unsigned long);
	as->wave_header_arr[id].dwBytesRecorded = 0;
	as->wave_header_arr[id].dwUser = 0L;
	as->wave_header_arr[id].dwFlags = 0L;
	as->wave_header_arr[id].dwLoops = 0L;

	MMRESULT rc = waveOutPrepareHeader(as->wave_out_handle, &as->wave_header_arr[id], sizeof(WAVEHDR));
}

void audio_dst_loop(void* param) {
	struct audio_dst_loop_params* aslp = (struct audio_dst_loop_params*)param;
	struct audio_dst* as = aslp->as;

	std::cout << "audio_dst_loop_started: " << AUDIO_DEVICE_PACKETSIZE << std::endl;

	as->smb_last_used_id = 0;
	
	int delay = 0;
	bool reprepared = true;

	int local_slot_count = 0;

	int last_ct = 0;

	while (as->wave_status == MMSYSERR_NOERROR) {
		if (ClusterFQ::ClusterFQ::shared_memory_buffer_read_slot_i(as->lp, as->smb_last_used_id, &as->buffer[local_slot_count * AUDIO_DEVICE_PACKETSIZE])) {
			int* ct_src = (int*)&as->buffer[local_slot_count * AUDIO_DEVICE_PACKETSIZE + sizeof(int)];
			unsigned long* br_ptr = (unsigned long*)& as->buffer[local_slot_count * AUDIO_DEVICE_PACKETSIZE + 2*sizeof(int)];
			as->wave_header_arr[local_slot_count].dwBytesRecorded = *br_ptr;

			if (last_ct > *ct_src) continue;
			last_ct = *ct_src;
			if (as->last_id > -1) {
				mutex_wait_for(&as->out_lock);
			}
			as->wave_status = waveOutWrite(as->wave_out_handle, &as->wave_header_arr[local_slot_count], sizeof(WAVEHDR));

			as->last_id = local_slot_count;
			local_slot_count = (local_slot_count + 1) % (as->lp_slots * 2);
			as->smb_last_used_id = (as->smb_last_used_id + 1) % as->lp_slots;
			
		} else {
			util_sleep(1);
		}
	}
	std::cout << "audio_dst_loop_ended: " << std::endl;
}