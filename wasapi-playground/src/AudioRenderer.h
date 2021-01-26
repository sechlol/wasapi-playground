#pragma once
#include <functional>
#include <atomic>
#include <thread>
#include <optional>

#include <MMDeviceAPI.h>
#include <AudioClient.h>

#include "common.h"

class AudioRenderer
{
public:
	AudioRenderer(IMMDevice* device);
	~AudioRenderer();

	std::optional<HRESULT> initialize(unsigned int bufferTimeSizeMs);
	void start(const std::function<double(FrameInfo)> renderCallback);
	void stop();
	void reset();

private:
	HRESULT write_to_buffer(const std::function<void(UINT32, BYTE*, DWORD*)> producer);
	UINT32 get_available_frames_number();
	
	IMMDevice* device;
	IAudioClient* audioClient;
	IAudioRenderClient* renderClient;
	WAVEFORMATEX* waveFormat;

	REFERENCE_TIME  devicePeriod;
	REFERENCE_TIME latency;
	UINT32 bufferSize;
	
	std::function<double(FrameInfo)> userCallback;
	std::atomic_bool running;
	std::thread renderThread;
};