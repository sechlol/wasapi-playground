#pragma once

#include <functional>
#include <future>
#include <optional>

#include <MMDeviceAPI.h>
#include <AudioClient.h>

#include "common.h"

class AudioCapturer {
public:
	AudioCapturer(IMMDevice* devicePointer);
	~AudioCapturer();

	std::optional<HRESULT> initialize(unsigned int bufferTimeSizeMs);
	std::future<AudioRecording> start_recording();
	void start_streaming(std::function<void(BYTE*)> callback);

	void stop();
	
private:
	IMMDevice* device;
	IAudioClient* audioClient;
	IAudioCaptureClient* captureClient;
	WAVEFORMATEX* waveFormat;

	REFERENCE_TIME  devicePeriod;
	REFERENCE_TIME latency;
	UINT32 bufferSize;

	std::future<AudioRecording> recordingFuture;
	std::atomic_bool running;
};

