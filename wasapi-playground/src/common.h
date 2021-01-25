#pragma once

#include <string>
#include <vector>

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = nullptr;
    }
};

std::wstring string_to_wstring(const std::string& s);

enum class AudioDeviceDirection {
    Input,
    Output
};

struct AudioDeviceInfo {
    std::string id;
    std::string friendly_name;
    AudioDeviceDirection direction;
};

struct AudioDevices {
    std::vector<AudioDeviceInfo> input_devices;
    std::vector<AudioDeviceInfo> output_devices;
};

struct AudioRecording {
    unsigned short channels;
    unsigned int samplesPerSecond;
    unsigned long durationMs;

    std::vector<float> data;
};

struct FrameInfo {
    double time;
    long ordinalNumber;
};