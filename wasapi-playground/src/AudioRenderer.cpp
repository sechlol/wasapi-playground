#include "AudioRenderer.h"
#include "common.h"

AudioRenderer::AudioRenderer(IMMDevice* device_pointer) :
	device(device_pointer),
	audioClient(nullptr),
	renderClient(nullptr),
	waveFormat(nullptr),
	devicePeriod(0),
	latency(0),
	bufferSize(0)
{
}

AudioRenderer::~AudioRenderer()
{
	SafeRelease(&device);
	SafeRelease(&audioClient);
	SafeRelease(&renderClient);
	CoTaskMemFree(waveFormat);
}

std::optional<HRESULT> AudioRenderer::initialize(unsigned int bufferTimeSizeMs)
{
	HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void**>(&audioClient));
	if (FAILED(hr))
	{
		printf("Unable to activate audio client: %x.\n", hr);
		return hr;
	}

	REFERENCE_TIME  minimumDevicePeriod = 0;
	WAVEFORMATEX* workingFormat = nullptr;

	hr = audioClient->GetDevicePeriod(&devicePeriod, &minimumDevicePeriod);
	if (FAILED(hr))
	{
		printf("Unable to GetDevicePeriod: %x.\n", hr);
		return hr;
	}

	hr = audioClient->GetMixFormat(&waveFormat);
	if (FAILED(hr))
	{
		printf("Unable to get mix format on audio client: %x.\n", hr);
		return hr;
	}

	hr = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, waveFormat, &workingFormat);
	if (FAILED(hr))
	{
		if (workingFormat != nullptr)
		{
			CoTaskMemFree(waveFormat);
			waveFormat = workingFormat;
			printf("Replace waveformat with another working format");
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
		0,								// only used for EXCLUSIVE mode
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

	hr = audioClient->GetService(IID_PPV_ARGS(&renderClient));
	if (FAILED(hr))
	{
		printf("Unable to get new render client: %x.\n", hr);
		return hr;
	}

	//*
	printf("**** Details of device\n");
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

void AudioRenderer::start(std::function<double(double)> renderCallback)
{
	if (running || renderClient == nullptr)
		return;

	userCallback = renderCallback;
	BYTE* pData;

	// Grab the entire buffer for the initial fill operation.
	HRESULT hr = renderClient->GetBuffer(bufferSize, &pData);
	if (FAILED(hr))
	{
		printf("Unable to get buffer: %x.\n", hr);
		return;
	}

	hr = renderClient->ReleaseBuffer(bufferSize, AUDCLNT_BUFFERFLAGS_SILENT);
	if (FAILED(hr))
	{
		printf("Unable to release buffer: %x.\n", hr);
		return;
	}

	hr = audioClient->Start();
	if (FAILED(hr))
	{
		printf("FAILED TO START AUDIOCLIENT: %x.\n", hr);
		return;
	}

	running = true;

	renderThread = std::thread([&]() {
		BYTE* buffData;
		UINT32 numFramesPadding;
		HRESULT result;
		double time = 0;
		auto interval = (long)latency / 2;

		while (running) {
			Sleep(interval);

			result = audioClient->GetCurrentPadding(&numFramesPadding);
			if (FAILED(result))
			{
				printf("RENDER-THREAD Failed to GetCurrentPadding(): %x.\n", result);
				break;
			}

			auto framesAvailable = bufferSize - numFramesPadding;

			result = renderClient->GetBuffer(framesAvailable, &buffData);
			if (FAILED(result))
			{
				printf("RENDER-THREAD Failed to GetBuffer(): %x.\n", result);
				break;
			}

			fill_buffer(buffData, framesAvailable, &time);
			
			hr = renderClient->ReleaseBuffer(framesAvailable, 0);
			if (FAILED(result))
			{
				printf("RENDER-THREAD Failed to ReleaseBuffer(): %x.\n", result);
				break;
			}
		}
		});
}

void AudioRenderer::fill_buffer(BYTE* buffer, unsigned int framesAvailable, double* globalTime) {
	auto bufferLengthInBytes = framesAvailable * waveFormat->nBlockAlign;
	double timeIncrement = 1.0 / waveFormat->nSamplesPerSec;
	float* dataBuffer = reinterpret_cast<float*>(buffer);

	for (size_t i = 0; i < bufferLengthInBytes / sizeof(float); i += waveFormat->nChannels)
	{
		float sample = (float)userCallback(*globalTime);
		for (size_t j = 0; j < waveFormat->nChannels; j++)
		{
			dataBuffer[i + j] = sample;
		}

		*globalTime += timeIncrement;
	}
}

void AudioRenderer::stop()
{
	if (!running)
		return;

	running = false;
	auto hr = audioClient->Stop();
	if (FAILED(hr))
	{
		printf("FAILED TO stop AUDIOCLIENT: %x.\n", hr);
		return;
	}

	renderThread.join();
}