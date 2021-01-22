#pragma once

#include <MMDeviceAPI.h>
#include <memory>
#include <functional>
#include <future>

class AudioCapturer {
public:
	AudioCapturer(IMMDevice* device);
	~AudioCapturer();

	std::promise<int> start_recording();
	void start_streaming(std::function<void(BYTE*)> callback);

	void stop();

private:
};

