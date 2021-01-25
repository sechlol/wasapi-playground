#include "AudioRenderer.h"

AudioRenderer::AudioRenderer(IMMDevice* devicePointer) :
	device(devicePointer),
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
	HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&audioClient));
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

	hr = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&renderClient));
	if (FAILED(hr))
	{
		printf("Unable to get new render client: %x.\n", hr);
		return hr;
	}

	//*
	printf("**** Details of RENDERING device\n");
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

void AudioRenderer::start(std::function<double(FrameInfo)> renderCallback)
{
	if (running || renderClient == nullptr)
		return;

	userCallback = renderCallback;
	
	// Write a packet of silence before starting the audio stream, to avoid glitches
	write_to_buffer([](UINT32 _, BYTE* __, DWORD* flags) {
		*flags = AUDCLNT_BUFFERFLAGS_SILENT;
	});

	auto result = audioClient->Start();
	if (FAILED(result))
	{
		printf("FAILED TO START AUDIOCLIENT: %x.\n", result);
		return;
	}

	running = true;

	renderThread = std::thread([&]() {
		double globalTime = 0;
		double timeIncrement = 1.0 / (double)waveFormat->nSamplesPerSec;
		long frameCount = 0;
		auto interval = (long)latency / 2;

		while (running) {
			Sleep(interval);

			write_to_buffer([&](UINT32 framesAvailable, BYTE* buffer, DWORD* _) {
				auto bufferLengthInBytes = framesAvailable * waveFormat->nBlockAlign;
				float* dataBuffer = reinterpret_cast<float*>(buffer);

				for (size_t i = 0; i < bufferLengthInBytes / sizeof(float); i += waveFormat->nChannels)
				{
					float sample = (float)userCallback({ globalTime, frameCount });
					for (size_t j = 0; j < waveFormat->nChannels; j++)
					{
						dataBuffer[i + j] = sample;
					}

					globalTime += timeIncrement;
					frameCount++;
				}
			});
		}
	});
}

HRESULT AudioRenderer::write_to_buffer(std::function<void(UINT32, BYTE*, DWORD*)> fill_buffer)
{
	DWORD flags = 0;
	BYTE* buffData = nullptr;
	auto framesAvailable = get_available_frames_number();

	auto result = renderClient->GetBuffer(framesAvailable, &buffData);
	if (FAILED(result))
	{
		printf("RENDER-THREAD Failed to GetBuffer(): %x.\n", result);
		return result;
	}

	fill_buffer(framesAvailable, buffData, &flags);

	result = renderClient->ReleaseBuffer(framesAvailable, flags);
	if (FAILED(result))
	{
		printf("RENDER-THREAD Failed to ReleaseBuffer(): %x.\n", result);
		return result;
	}
}

UINT32 AudioRenderer::get_available_frames_number()
{
	UINT32 numFramesPadding;
	HRESULT result = audioClient->GetCurrentPadding(&numFramesPadding);
	if (FAILED(result))
	{
		printf("RENDER-THREAD Failed to GetCurrentPadding(): %x.\n", result);
		return 0;
	}

	return bufferSize - numFramesPadding;
}

void AudioRenderer::stop()
{
	if (!running)
		return;

	running = false;
	auto hr = audioClient->Stop();
	if (FAILED(hr))
	{
		printf("FAILED TO stop AudioRenderer: %x.\n", hr);
		return;
	}

	renderThread.join();
}

void AudioRenderer::reset()
{
	if (running)
		return;

	auto hr = audioClient->Reset();
	if (FAILED(hr))
	{
		printf("FAILED TO reset AudioRenderer: %x.\n", hr);
		return;
	}
}
