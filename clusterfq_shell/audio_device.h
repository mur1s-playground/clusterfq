#pragma once

#ifndef AUDIO_DEVICE_PACKETSIZE
#define AUDIO_DEVICE_PACKETSIZE 1024
#endif

struct audio_device {
	int id;
	char* name;
	int channels;
};