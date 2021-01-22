#include "AudioCapturer.h"
#include "common.h"

AudioCapturer::AudioCapturer(IMMDevice* device)
{
}

AudioCapturer::~AudioCapturer()
{
}

std::promise<int> AudioCapturer::start_recording()
{
	return std::promise<int>();
}

void AudioCapturer::start_streaming(std::function<void(BYTE*)> callback)
{
}
