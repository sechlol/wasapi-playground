#include "DeviceEnumerator.h"

#include <optional>
#include <vector>
#include <string>

#include <Audioclient.h>

using std::string;
using std::optional;
using std::vector;

DeviceEnumerator::DeviceEnumerator() : deviceEnumerator(nullptr) 
{
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

    HRESULT hr = CoInitializeEx(NULL, NULL);
    if (FAILED(hr))
    {
        printf("Unable to initialize COM: %x\n", hr);
        return;
    }

    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        (void**)(&deviceEnumerator));

    if (FAILED(hr))
    {
        printf("Unable to instantiate device enumerator: %x\n", hr);
        return;
    }
}

DeviceEnumerator::~DeviceEnumerator()
{
    SafeRelease(&this->deviceEnumerator);
}

std::vector<AudioDeviceSummary> DeviceEnumerator::get_output_devices_summary()
{
    return get_audio_devices_of_direction(EDataFlow::eRender);
}

std::vector<AudioDeviceSummary> DeviceEnumerator::get_input_devices_summary()
{
    return get_audio_devices_of_direction(EDataFlow::eCapture);
}

AudioDeviceList DeviceEnumerator::get_all_devices_summary() {

    return AudioDeviceList{
        this->get_audio_devices_of_direction(EDataFlow::eCapture),
        this->get_audio_devices_of_direction(EDataFlow::eRender)
    };
}

void DeviceEnumerator::register_notifications(IMMNotificationClient* notifClient)
{
    auto result = deviceEnumerator->RegisterEndpointNotificationCallback(notifClient);
    if (FAILED(result))
        printf("Unable to RegisterEndpointNotificationCallback()");
}

void DeviceEnumerator::unregister_notifications(IMMNotificationClient* notifClient)
{
    auto result = deviceEnumerator->UnregisterEndpointNotificationCallback(notifClient);
    if (FAILED(result))
        printf("Unable to RegisterEndpointNotificationCallback()");
}

std::unique_ptr<AudioDevice> DeviceEnumerator::get_device_by_id(const std::string& deviceId)
{
    IMMDevice* device = nullptr;
    auto temp_id = string_to_wstring(deviceId);
    HRESULT hr = this->deviceEnumerator->GetDevice((LPCWSTR)temp_id.c_str(), &device);
    if (FAILED(hr)) {
        printf("Unable to retrieve device %s\n", deviceId);
        return nullptr;
    }
    return std::make_unique<AudioDevice>(device);
}

std::unique_ptr<AudioDevice> DeviceEnumerator::get_default_output()
{
    IMMDevice* device = nullptr;
    auto result = this->deviceEnumerator->GetDefaultAudioEndpoint(EDataFlow::eRender,ERole::eMultimedia, &device);
    if (FAILED(result)) {
        printf("Unable to retrieve default output device ID %x\n", result);
        return nullptr;
    }
    return std::make_unique<AudioDevice>(device);
}

std::unique_ptr<AudioDevice> DeviceEnumerator::get_default_input()
{
    IMMDevice* device = nullptr;
    auto result = this->deviceEnumerator->GetDefaultAudioEndpoint(EDataFlow::eCapture, ERole::eMultimedia, &device);
    if (FAILED(result)) {
        printf("Unable to retrieve default input device ID %x\n", result);
        return nullptr;
    }
    return std::make_unique<AudioDevice>(device);
}

vector<AudioDeviceSummary> DeviceEnumerator::get_audio_devices_of_direction(EDataFlow direction) {
    IMMDeviceCollection* deviceCollection = NULL;

    HRESULT hr = deviceEnumerator->EnumAudioEndpoints(direction, DEVICE_STATE_ACTIVE, &deviceCollection);
    if (FAILED(hr) && deviceCollection != nullptr)
    {
        printf("Unable to retrieve device collection: %x\n", hr);
        return {};
    }

    unsigned int device_count = 0;
    hr = deviceCollection->GetCount(&device_count);
    if (FAILED(hr)) {
        printf("Unable to retrieve device count: %x\n", hr);
        return {};
    }

    vector<AudioDeviceSummary> all_info;
    for (size_t i = 0; i < device_count; i++)
    {
        auto info = get_device_summary(deviceCollection, i);
        if (info.has_value()) {
            info.value().direction = direction == EDataFlow::eCapture ? AudioDeviceDirection::Input : AudioDeviceDirection::Output;
            all_info.push_back(info.value());
        }
    }

    return all_info;
}

optional<AudioDeviceSummary> DeviceEnumerator::get_device_summary(IMMDeviceCollection* deviceCollection, unsigned int deviceIndex)
{
    AudioDeviceSummary info;
    IMMDevice* devicePointer;

    auto hr = deviceCollection->Item(deviceIndex, &devicePointer);
    if (FAILED(hr))
    {
        printf("Unable to get device %d: %x\n", deviceIndex, hr);
        return std::nullopt;
    }

    AudioDevice device(devicePointer);
    return device.get_summary();
}
