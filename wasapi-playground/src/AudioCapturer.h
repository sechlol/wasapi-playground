#pragma once
#include <memory>
#include <functional>
#include <future>
#include <optional>

#include <MMDeviceAPI.h>
#include <AudioClient.h>

#include "common.h"
#include "AudioDevice.h"

class AudioCapturer {
public:
	AudioCapturer(std::unique_ptr<AudioDevice> devicePointer);
	AudioCapturer(const AudioCapturer& other) = delete;
	~AudioCapturer();

	std::optional<HRESULT> initialize(unsigned int bufferTimeSizeMs);
	std::future<AudioRecording> start_recording();
	void start_streaming(const std::function<void(BYTE*, UINT32)> callback);

	void stop();
	
private:
	void capture_data(const std::function<void(BYTE*, UINT32, DWORD)> dataReader);
	WAVEFORMATEX* get_working_format() const;

	std::unique_ptr<AudioDevice> device;
	IAudioClient3* audioClient;
	WAVEFORMATEX* deviceFormat;
	IAudioCaptureClient* captureClient;
	std::optional<AudioStreamInfo> streamInfo;

	std::function<void(BYTE*, UINT32)> userCallback;
	std::optional<std::thread> streamingThread;
	std::future<AudioRecording> recordingFuture;
	std::atomic_bool running;
};

