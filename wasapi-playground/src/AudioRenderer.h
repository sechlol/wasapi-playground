#pragma once
#include <functional>
#include <atomic>
#include <thread>
#include <optional>

#include <MMDeviceAPI.h>
#include <AudioClient.h>

#include "AudioDevice.h"
#include "common.h"

class AudioRenderer
{
public:
	AudioRenderer(std::unique_ptr<AudioDevice> device);
	AudioRenderer(const AudioRenderer& other) = delete;

	~AudioRenderer();

	std::optional<HRESULT> initialize(unsigned int bufferTimeSizeMs);
	void start(const std::function<double(FrameInfo)> renderCallback);
	void stop();
	void reset();

private:
	HRESULT write_to_buffer(const std::function<void(UINT32, BYTE*, DWORD*)> producer);
	UINT32 get_available_frames_number();
	WAVEFORMATEX* get_working_format();

	std::unique_ptr<AudioDevice> device;
	IAudioClient3* audioClient;
	WAVEFORMATEX* deviceFormat;
	IAudioRenderClient* renderClient;
	std::optional<AudioStreamInfo> streamInfo;

	std::function<double(FrameInfo)> userCallback;
	std::atomic_bool running;
	std::thread renderThread;
};