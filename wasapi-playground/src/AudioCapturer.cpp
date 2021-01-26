#include "AudioCapturer.h"
#include <future>

AudioCapturer::AudioCapturer(IMMDevice* devicePointer) :
	device(devicePointer),
	audioClient(nullptr),
	captureClient(nullptr),
	waveFormat(nullptr),
	streamingThread(std::nullopt)
{
}

AudioCapturer::~AudioCapturer()
{
	stop();

	SafeRelease(&device);
	SafeRelease(&audioClient);
	SafeRelease(&captureClient);
	CoTaskMemFree(waveFormat);
}

std::optional<HRESULT> AudioCapturer::initialize(unsigned int bufferTimeSizeMs)
{
	HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&audioClient));
	if (FAILED(hr))
	{
		printf("Unable to activate audio client: %x.\n", hr);
		return hr;
	}

	hr = audioClient->GetMixFormat(&waveFormat);
	if (FAILED(hr))
	{
		printf("Unable to get mix format on audio client: %x.\n", hr);
		return hr;
	}

	WAVEFORMATEX* workingFormat = nullptr;
	hr = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, waveFormat, &workingFormat);
	if (FAILED(hr))
	{
		if (workingFormat != nullptr)
		{
			CoTaskMemFree(waveFormat);
			waveFormat = workingFormat;
			printf("Replace waveformat with another working format\n");
		}
		else {
			printf("Mix Format not supported: %x.\n", hr);
			return hr;
		}
	}

	hr = audioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_NOPERSIST,
		(REFERENCE_TIME)bufferTimeSizeMs * 10000,		// 1 unit --> 100 nanoseconds
		0,												// only used for EXCLUSIVE mode
		waveFormat,
		NULL);

	if (FAILED(hr))
	{
		printf("Unable to initialize audio client: %x.\n", hr);
		return hr;
	}

	hr = audioClient->GetBufferSize(&bufferSize);
	if (FAILED(hr))
	{
		printf("Unable to get buffer size: %x.\n", hr);
		return hr;
	}

	hr = audioClient->GetStreamLatency(&latency);
	if (FAILED(hr))
	{
		printf("Unable to get latency: %x.\n", hr);
		return hr;
	}

	hr = audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&captureClient));
	if (FAILED(hr))
	{
		printf("Unable to get new render client: %x.\n", hr);
		return hr;
	}

	//*
	printf("**** Details of CAPTURING device\n");
	printf("\t- latency: %d\n", (long)latency);
	printf("\t- bufferSize (frames): %d\n", bufferSize);
	printf("\t- bufferSize (bytes): %d\n", bufferSize * waveFormat->nBlockAlign * waveFormat->nChannels);
	printf("\t- wBitsPerSample: %d\n", waveFormat->wBitsPerSample);
	printf("\t- nChannels: %d\n", waveFormat->nChannels);
	printf("\t- nSamplesPerSec: %d\n", waveFormat->nSamplesPerSec);
	printf("\t- nBlockAlign: %d\n", waveFormat->nBlockAlign);
	printf("\t- nAvgBytesPerSec: %d\n", waveFormat->nAvgBytesPerSec);
	printf("\t- cbSize: %d\n", waveFormat->cbSize);
	printf("\t- wFormatTag: %d\n", waveFormat->wFormatTag);
	printf("****\n");
	/**/

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

	running = true;
	
	return std::async(std::launch::async, [this]() {
		auto interval = (long)latency / 2;

		AudioRecording recordingData;
		recordingData.channels = waveFormat->nChannels;
		recordingData.samplesPerSecond = waveFormat->nSamplesPerSec;

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

	streamingThread = std::thread([this]() {
		auto interval = (long)latency / 2;

		AudioRecording recordingData;
		recordingData.channels = waveFormat->nChannels;
		recordingData.samplesPerSecond = waveFormat->nSamplesPerSec;

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
