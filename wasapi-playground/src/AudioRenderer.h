#pragma once
#include <functional>
#include <atomic>
#include <thread>
#include <optional>

#include <MMDeviceAPI.h>
#include <AudioClient.h>

class AudioRenderer
{
public:
	AudioRenderer(IMMDevice* device);
	~AudioRenderer();

	std::optional<HRESULT> initialize(unsigned int userBufferSize);
	void start(std::function<double(double)> renderCallback);
	void fill_buffer(BYTE* buffer, unsigned int bufferLengthInBytes, double* globalTime);
	void stop();

private:
	IMMDevice* device;
	IAudioClient* audioClient;
	IAudioRenderClient* renderClient;
	WAVEFORMATEX* waveFormat;

	REFERENCE_TIME  devicePeriod;
	REFERENCE_TIME latency;
	UINT32 bufferSize;
	
	std::function<double(double)> userCallback;
	std::atomic_bool running;
	std::thread renderThread;
};