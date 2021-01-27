#include "AudioRenderer.h"

AudioRenderer::AudioRenderer(std::unique_ptr<AudioDevice> devicePointer) :
	device(std::move(devicePointer)),
	audioClient(device->get_audio_client()),
	deviceFormat(get_working_format()),
	renderClient(nullptr)
{
}

AudioRenderer::~AudioRenderer()
{
	stop();

	SafeRelease(&audioClient);
	SafeRelease(&renderClient);
	CoTaskMemFree(deviceFormat);
}

std::optional<HRESULT> AudioRenderer::initialize(unsigned int bufferTimeSizeMs)
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

	result = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&renderClient));
	if (FAILED(result))
	{
		printf("Unable to get new render client: %x.\n", result);
		return result;
	}

	return std::nullopt;
}

void AudioRenderer::start(const std::function<double(FrameInfo)> renderCallback)
{
	if (running || renderClient == nullptr)
		return;

	userCallback = renderCallback;
	streamInfo = get_stream_info(audioClient);
	
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
		long frameCount = 0;
		auto latency = streamInfo.has_value() ? streamInfo.value().latency / 10000 : 0;
		
		double timeIncrement = 1.0 / (double)deviceFormat->nSamplesPerSec;
		auto interval = (long)latency / 2;

		while (running) {
			Sleep(interval);

			write_to_buffer([&](UINT32 framesAvailable, BYTE* buffer, DWORD* _) {
				auto bufferLengthInBytes = framesAvailable * deviceFormat->nBlockAlign;
				float* dataBuffer = reinterpret_cast<float*>(buffer);

				for (size_t i = 0; i < bufferLengthInBytes / sizeof(float); i += deviceFormat->nChannels)
				{
					float sample = (float)userCallback({ globalTime, frameCount });
					for (size_t j = 0; j < deviceFormat->nChannels; j++)
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

HRESULT AudioRenderer::write_to_buffer(const std::function<void(UINT32, BYTE*, DWORD*)> fill_buffer)
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

	return 0;
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

	return streamInfo.value().bufferSizeInFrames - numFramesPadding;
}

WAVEFORMATEX* AudioRenderer::get_working_format()
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
	streamInfo = std::nullopt;
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
