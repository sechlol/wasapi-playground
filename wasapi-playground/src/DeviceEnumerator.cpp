#include "DeviceEnumerator.h"

#include <optional>
#include <vector>
#include <string>

#include <functiondiscoverykeys.h>

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

optional<AudioDevices> DeviceEnumerator::enumerate_audio_devices() {

    return AudioDevices{
        this->get_audio_devices_of_direction(EDataFlow::eCapture),
        this->get_audio_devices_of_direction(EDataFlow::eRender)
    };
}

IMMDevice* DeviceEnumerator::get_device_by_id(std::string deviceId)
{
    IMMDevice* device = nullptr;
    auto temp_id = string_to_wstring(deviceId);
    LPCWSTR wid = (LPCWSTR)temp_id.c_str();
    HRESULT hr = this->deviceEnumerator->GetDevice(wid, &device);
    if (FAILED(hr)) {
        printf("Unable to retrieve device ID %s: %x\n", deviceId, hr);
    }

    return device;
}

IMMDevice* DeviceEnumerator::get_default_output()
{
    IMMDevice* device = nullptr;
    auto result = this->deviceEnumerator->GetDefaultAudioEndpoint(EDataFlow::eRender,ERole::eMultimedia, &device);
    if (FAILED(result)) {
        printf("Unable to retrieve default output device ID %x\n", result);
    }
    return device;
}

IMMDevice* DeviceEnumerator::get_default_input()
{
    IMMDevice* device = nullptr;
    auto result = this->deviceEnumerator->GetDefaultAudioEndpoint(EDataFlow::eCapture, ERole::eMultimedia, &device);
    if (FAILED(result)) {
        printf("Unable to retrieve default input device ID %x\n", result);
    }
    return device;
}

vector<AudioDeviceInfo> DeviceEnumerator::get_audio_devices_of_direction(EDataFlow direction) {
    IMMDeviceCollection* deviceCollection = NULL;

    HRESULT hr = this->deviceEnumerator->EnumAudioEndpoints(direction, DEVICE_STATE_ACTIVE, &deviceCollection);
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

    vector<AudioDeviceInfo> all_info;
    for (size_t i = 0; i < device_count; i++)
    {
        auto info = get_device_info(deviceCollection, i);
        if (info.has_value()) {
            info.value().direction = direction == EDataFlow::eCapture ? AudioDeviceDirection::Input : AudioDeviceDirection::Output;
            all_info.push_back(info.value());
        }
    }

    return all_info;
}

optional<AudioDeviceInfo> DeviceEnumerator::get_device_info(IMMDeviceCollection* device_collection, unsigned int device_index)
{
    IMMDevice* device;
    LPWSTR deviceId;
    HRESULT hr;

    hr = device_collection->Item(device_index, &device);
    if (FAILED(hr))
    {
        printf("Unable to get device %d: %x\n", device_index, hr);
        return std::nullopt;
    }

    hr = device->GetId(&deviceId);
    if (FAILED(hr))
    {
        printf("Unable to get device %d id: %x\n", device_index, hr);
        return std::nullopt;
    }

    IPropertyStore* propertyStore;
    hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    SafeRelease(&device);
    if (FAILED(hr))
    {
        printf("Unable to open device %d property store: %x\n", device_index, hr);
        return std::nullopt;
    }

    PROPVARIANT friendlyName;
    PropVariantInit(&friendlyName);
    hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
    SafeRelease(&propertyStore);

    if (FAILED(hr))
    {
        printf("Unable to retrieve friendly name for device %d : %x\n", device_index, hr);
        return std::nullopt;
    }

    std::wstring w_name(friendlyName.pwszVal);
    std::wstring w_id(deviceId);

    auto infos = AudioDeviceInfo{
        string(w_id.begin(), w_id.end()),
        friendlyName.vt != VT_LPWSTR ? "Unknown" : string(w_name.begin(), w_name.end())
    };

    PropVariantClear(&friendlyName);
    CoTaskMemFree(deviceId);

    return infos;
}


