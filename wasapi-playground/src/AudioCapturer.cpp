#include "AudioCapturer.h"
#include <future>

AudioCapturer::AudioCapturer(std::unique_ptr<AudioDevice> devicePointer) :
	device(std::move(devicePointer)),
	audioClient(device->get_audio_client()),
	deviceFormat(get_working_format()),
	captureClient(nullptr),
	streamInfo(std::nullopt),
	streamingThread(std::nullopt)
{
}

AudioCapturer::~AudioCapturer()
{
	stop();

	SafeRelease(&audioClient);
	SafeRelease(&captureClient);
	CoTaskMemFree(deviceFormat);
}

std::optional<HRESULT> AudioCapturer::initialize(unsigned int bufferTimeSizeMs)
{
	if (audioClient == nullptr)
	{
		return S_FALSE;
	}

	auto result = audioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_NOPERSIST,
		(REFERENCE_TIME)bufferTimeSizeMs * 10000,		// 1 unit --> 100 nanoseconds
		0,												// only used for EXCLUSIVE mode
		deviceFormat,
		NULL);

	if (FAILED(result))
	{
		printf("Unable to initialize audio client: %x.\n", result);
		return result;
	}


	result = audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&captureClient));
	if (FAILED(result))
	{
		printf("Unable to get new capture client: %x.\n", result);
		return result;
	}

	return std::nullopt;
}

void AudioCapturer::capture_data(const std::function<void(BYTE*, UINT32, DWORD)> dataReader)
{
	UINT32 packetLength = 0;
	BYTE* buffData = nullptr;
	UINT32 framesAvailable = 0;
	DWORD flags = 0;

	auto result = captureClient->GetNextPacketSize(&packetLength);
	if (FAILED(result))
	{
		printf("Failed to GetNextPacketSize(): %x.\n", result);
		return;
	}

	while (packetLength != 0)
	{
		// Get the available data in the shared buffer.
		result = captureClient->GetBuffer(
			&buffData,
			&framesAvailable,
			&flags,
			NULL,
			NULL);

		if (FAILED(result))
		{
			printf("Failed to GetBuffer(): %x.\n", result);
			return;
		}

		// data is just silence
		if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
		{
			buffData = nullptr;
		}

		if (buffData != nullptr)
			dataReader(buffData, framesAvailable, flags);

		result = captureClient->ReleaseBuffer(framesAvailable);
		if (FAILED(result))
		{
			printf("Failed to ReleaseBuffer(): %x.\n", result);
			return;
		}

		result = captureClient->GetNextPacketSize(&packetLength);
		if (FAILED(result))
		{
			printf("Failed to GetNextPacketSize(): %x.\n", result);
			return;
		}
	}
}

std::future<AudioRecording> AudioCapturer::start_recording()
{
	if (running)
		return std::future<AudioRecording>();

	HRESULT hr = audioClient->Start();
	if (FAILED(hr))
	{
		printf("FAILED TO START AUDIOCLIENT: %x.\n", hr);
		return std::future<AudioRecording>();
	}

	streamInfo = get_stream_info(audioClient);
	running = true;
	
	return std::async(std::launch::async, [this]() {
		auto latency = streamInfo.has_value() ? streamInfo.value().latency / 10000 : 0;
		auto interval = (long)latency / 2;

		AudioRecording recordingData;
		recordingData.channels = deviceFormat->nChannels;
		recordingData.samplesPerSecond = deviceFormat->nSamplesPerSec;

		while (running) {
			Sleep(interval);
			capture_data([&recordingData](BYTE* buffData, UINT32 framesAvailable, DWORD flags) {
				float* bufferFloat = reinterpret_cast<float*>(buffData);

				// assume 32 bit format
				for (size_t i = 0; i < framesAvailable; i++)
					recordingData.data.push_back(bufferFloat[i]);
			});
		}
		
		recordingData.durationMs = (recordingData.data.size() * 1000 / recordingData.samplesPerSecond);
		return recordingData;
	});
}

void AudioCapturer::start_streaming(const std::function<void(BYTE*, UINT32)> callback)
{
	if (running)
		return;

	HRESULT hr = audioClient->Start();
	if (FAILED(hr))
	{
		printf("FAILED TO START AUDIOCLIENT: %x.\n", hr);
		return;
	}

	running = true;
	userCallback = callback;
	streamInfo = get_stream_info(audioClient);

	streamingThread = std::thread([this]() {
		auto latency = streamInfo.has_value() ? streamInfo.value().latency / 10000 : 0;
		auto interval = (long)latency / 2;

		AudioRecording recordingData;
		recordingData.channels = deviceFormat->nChannels;
		recordingData.samplesPerSecond = deviceFormat->nSamplesPerSec;

		while (running) {
			Sleep(interval);
  			capture_data([this](BYTE* buffData, UINT32 framesAvailable, DWORD flags) {
				userCallback(buffData, framesAvailable);
			});
		}

		recordingData.durationMs = (recordingData.data.size() * 1000 / recordingData.samplesPerSecond);
		return recordingData;
	});
}

void AudioCapturer::stop()
{
	if (!running)
		return;

	running = false;

	auto result = audioClient->Stop();
	if (FAILED(result))
	{
		printf("FAILED TO stop AudioCapturer: %x.\n", result);
	}

	if (streamingThread.has_value())
		streamingThread.value().join();
}

WAVEFORMATEX* AudioCapturer::get_working_format() const
{
	auto defaultFormat = device->get_device_format();
	WAVEFORMATEX* outFormat = nullptr;

	auto result = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, defaultFormat, &outFormat);
	if (FAILED(result))
	{
		CoTaskMemFree(defaultFormat);
		if (outFormat == nullptr) {
			printf("[AudioRenderer] Failed to call IsFormatSupported()\n");
			return nullptr;
		}
		printf("Replace waveformat with another working format\n");
	}
	else {
		CoTaskMemFree(outFormat);
		outFormat = defaultFormat;
	}
	return outFormat;
}