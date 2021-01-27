#include "AudioDevice.h"

#include <functiondiscoverykeys.h>

namespace {
    const std::string UNAVAILABLE = "*unabailable*";
    const REFERENCE_TIME PERIOD_TIME = 480 * 10000; // 1 unit = 100-nanosecond

    AudioDeviceSummary get_summary_from_device(IMMDevice* device) {

        AudioDeviceSummary summary;
        LPWSTR deviceId;

        auto hr = device->GetId(&deviceId);
        if (FAILED(hr))
        {
            printf("Unable to get device id: %x\n", hr);
            summary.id = UNAVAILABLE;
        }
        else {
            std::wstring w_id(deviceId);
            summary.id = std::string(w_id.begin(), w_id.end());
            CoTaskMemFree(deviceId);
        }

        IPropertyStore* propertyStore;
        hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
        if (FAILED(hr))
        {
            printf("Unable to open device property store: %x\n", hr);
            summary.friendlyName = UNAVAILABLE;
        }
        else {
            PROPVARIANT friendlyName;
            PropVariantInit(&friendlyName);
            hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
            if (FAILED(hr))
            {
                printf("Unable to retrieve friendly name for device : %x\n", hr);
                summary.friendlyName = UNAVAILABLE;
            }
            else {
                std::wstring w_name(friendlyName.pwszVal);
                summary.friendlyName = friendlyName.vt != VT_LPWSTR ? "Unknown" : std::string(w_name.begin(), w_name.end());
            }

            PropVariantClear(&friendlyName);
            SafeRelease(&propertyStore);
        }

        return summary;
    }

    void get_info_from_audio_client_1(IMMDevice* device, AudioDeviceDetails& info) {

        IAudioClient* audioClient = nullptr;
        auto result = device->Activate(
            __uuidof(IAudioClient),
            CLSCTX_ALL,
            NULL,
            reinterpret_cast<void**>(&audioClient));

        if (FAILED(result))
        {
            printf("Unable to activate audio client: %x.\n", result);
        }
        else {
            AudioInfo1 info1;
            WAVEFORMATEX* deviceFormat = nullptr;
            result = audioClient->GetMixFormat(&deviceFormat);
            if (FAILED(result)) {
                printf("Unable to get mix format on audio client: %x.\n", result);
            }
            else {
                if (deviceFormat->cbSize >= 22 && deviceFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                    info1.extendedStreamFormat = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(deviceFormat);
                    info1.streamFormat = info1.extendedStreamFormat.value().Format;
                }
                else {
                    info1.streamFormat = *deviceFormat;
                }

                CoTaskMemFree(deviceFormat);
            }

            result = audioClient->GetDevicePeriod(&info1.defaultDevicePeriod, &info1.minDevicePeriod);
            if (FAILED(result)) {
                printf("Unable to get audio client GetDevicePeriod(): %x.\n", result);
            }

            info.extendedInfo1 = info1;
            SafeRelease(&audioClient);
        }
    }

    void get_info_from_audio_client_2(IMMDevice* device, AudioDeviceDetails& info) {
        IAudioClient2* audioClient = nullptr;
        auto result = device->Activate(
            __uuidof(IAudioClient2),
            CLSCTX_ALL,
            NULL,
            reinterpret_cast<void**>(&audioClient));

        if (FAILED(result))
        {
            printf("Unable to activate audio client 2: %x.\n", result);
        }
        else {
            WAVEFORMATEX* format = nullptr;
            AudioInfo2 extendedInfo{};

            audioClient->GetMixFormat(&format);

            // This doesn't seem to work...
            /*result = audioClient->GetBufferSizeLimits(format, false, &(extendedInfo.minSupportedBufferDuration), &(extendedInfo.maxSupportedBufferDuration));
            if (FAILED(result))
                printf("Unable to get audio client 2 GetBufferSizeLimits(): %x.\n", result);
            */
            result = audioClient->IsOffloadCapable(AUDIO_STREAM_CATEGORY::AudioCategory_Media, &extendedInfo.isOffloadCapable);
            if (FAILED(result))
                printf("Unable to get audio client 2 IsOffloadCapable(): %x.\n", result);

            info.extendedInfo2 = extendedInfo;

            CoTaskMemFree(format);
            SafeRelease(&audioClient);
        }
    }

    void get_info_from_audio_client_3(IMMDevice* device, AudioDeviceDetails& info) {
        IAudioClient3* audioClient = nullptr;
        auto result = device->Activate(
            __uuidof(IAudioClient3),
            CLSCTX_ALL,
            NULL,
            reinterpret_cast<void**>(&audioClient));

        if (FAILED(result))
        {
            printf("Unable to activate audio client 3: %x.\n", result);
        }
        else {
            WAVEFORMATEX* format = nullptr;
            AudioInfo3 extendedInfo{};

            result = audioClient->GetCurrentSharedModeEnginePeriod(&format, &extendedInfo.currentSharedModePeriod);
            if (FAILED(result)) {
                printf("Unable to get audio client 3 GetCurrentSharedModeEnginePeriod(): %x.\n", result);
            }
            else {
                extendedInfo.currentSharedModeFormat = *format;

                result = audioClient->GetSharedModeEnginePeriod(
                    format,
                    &extendedInfo.defaultPeriodInFrames,
                    &extendedInfo.fundamentalPeriodInFrames,
                    &extendedInfo.minPeriodInFrames,
                    &extendedInfo.maxPeriodInFrames);
                if (FAILED(result))
                    printf("Unable to get audio client 3 GetSharedModeEnginePeriod(): %x.\n", result);
                
                CoTaskMemFree(format);
            }

            info.extendedInfo3 = extendedInfo;
            SafeRelease(&audioClient);
        }
    }
}

AudioDevice::AudioDevice(IMMDevice* devicePointer) : device(devicePointer){}

AudioDevice::~AudioDevice()
{
    SafeRelease(&device);
}

AudioDeviceSummary AudioDevice::get_summary() const
{
    return get_summary_from_device(device);
}

AudioDeviceDetails AudioDevice::get_info() const
{
    AudioDeviceDetails details;
    details.summary = get_summary_from_device(device);
    get_info_from_audio_client_1(device, details);
    get_info_from_audio_client_2(device, details);
    get_info_from_audio_client_3(device, details);
    return details;
}

WAVEFORMATEX* AudioDevice::get_device_format() const
{
    auto audioClient = get_audio_client();

    if (audioClient != nullptr) {
        WAVEFORMATEX* deviceFormat = nullptr;
        auto result = audioClient->GetMixFormat(&deviceFormat);
        if (FAILED(result)) {
            printf("[AudioDevice.get_device_format()] Unable to get mix format: %x.\n", result);
            return nullptr;
        }
        
        return deviceFormat;
    }
    return nullptr;
}

WAVEFORMATEXTENSIBLE* AudioDevice::get_device_format_extended() const
{
    auto deviceFormat = get_device_format();
    if (deviceFormat->cbSize >= 22 && deviceFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        return reinterpret_cast<WAVEFORMATEXTENSIBLE*>(deviceFormat);
    return nullptr;
}

IAudioClient3* AudioDevice::get_audio_client() const
{
    IAudioClient3* client = nullptr;
    HRESULT result = device->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&client));
    if (FAILED(result))
        printf("[AudioDevice.get_audio_client()] Unable to activate audio client: %x.\n", result);

    return client;
}
/*
void Initialize() {
    result = audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_NOPERSIST,
        PERIOD_TIME,
        0,
        waveFormat,
        NULL);

    if (FAILED(result)) {
        printf("Unable to initialize audio client: %x.\n", result);
    }
    else {

        AudioInfo1 extendedInfo;
        result = audioClient->GetBufferSize(&extendedInfo.bufferSizeInFrames);
        if (FAILED(result)) {
            printf("Unable to get audio client GetBufferSize(): %x.\n", result);
        }

        result = audioClient->GetStreamLatency(&extendedInfo.latency);
        if (FAILED(result)) {
            printf("Unable to get audio client GetStreamLatency(): %x.\n", result);
        }
    }
}*/