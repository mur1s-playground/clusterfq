#include "audio_device.h"

#include <iostream>

void audio_device_set_format(struct audio_format *af, int channels, int samples_per_sec, int bits_per_sample) {
#ifdef _WIN32
	af->wave_format.wFormatTag = WAVE_FORMAT_PCM;
	af->wave_format.nChannels = channels;
	af->wave_format.nSamplesPerSec = samples_per_sec;
	af->wave_format.wBitsPerSample = bits_per_sample;
	af->wave_format.nBlockAlign = (channels * bits_per_sample) / 8;
	af->wave_format.nAvgBytesPerSec = (channels * samples_per_sec * bits_per_sample) / 8;
	af->wave_format.cbSize = 0;
#else
	if (bits_per_sample == 8) {
		af->pcm_format = SND_PCM_FORMAT_U8;
	} else if (bits_per_sample == 16) {
		af->pcm_format = SND_PCM_FORMAT_S16_LE;
	}
	af->channels = channels;
	af->rate = samples_per_sec;
	af->period_event = 0;
#endif
}

#ifdef _WIN32

#else

int audio_device_set_hw_params(snd_pcm_t* handle, snd_pcm_hw_params_t* params, struct audio_format *af) {
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

	err = snd_pcm_hw_params_set_format(handle, params, af->pcm_format);
	if (err < 0) {
		std::cout << "err setting format" << std::endl;
		return err;
	}

	unsigned int rrate = af->rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
	if (err < 0) {
		std::cout << "err setting rate" << std::endl;
		return err;
	}
	if (rrate != af->rate) {
		std::cout << "err not as requested " << rrate << " != " << af->rate << std::endl;
		return -1;
	}
	std::cout << "rate: " << rrate << std::endl;

	int dir;
	unsigned int buffer_time = 1000000 / 16 * 16;

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
	af->buffer_size = size;
	std::cout << "buffer_size: " << size << std::endl;

	unsigned int period_time = 1000000 / 16;
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

	af->period_size = size;
	std::cout << "period_size: " << size << std::endl;

	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		std::cout << "err unable to set hw params for playback: " << snd_strerror(err) << std::endl;
		return err;
	}

	return 0;
}

int audio_device_set_sw_params(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams, struct audio_format *af) {
	int err = 0;

	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		std::cout << "err unable to determine current swparams for playback: " << snd_strerror(err) << std::endl;
		return err;
	}

	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (af->buffer_size / af->period_size) * af->period_size);
	if (err < 0) {
		std::cout << "err unable to set start threshold mode for playback: " << snd_strerror(err) << std::endl;
		return err;
	}

	snd_pcm_uframes_t boundary = (snd_pcm_uframes_t)0UL;
	snd_pcm_sw_params_get_boundary(swparams, &boundary);

	err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, boundary);
	if (err < 0) {
		std::cout << "err unable to set stop threshold" << std::endl;
		return err;
	}

	/* allow the transfer when at least period_size samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, af->period_event ? af->buffer_size : af->period_size);
	if (err < 0) {
		std::cout << "err unable to set avail min for playback: " << snd_strerror(err) << std::endl;
		return err;
	}
	/* enable period events when requested */
	if (af->period_event) {
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
#endif