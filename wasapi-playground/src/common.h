#pragma once

#include <string>
#include <vector>
#include <optional>
#include <AudioClient.h>

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = nullptr;
    }
};

struct AudioStreamInfo {
    UINT32 bufferSizeInFrames;
    REFERENCE_TIME latency;
};

std::wstring string_to_wstring(const std::string& s);
std::string LPCWSTR_to_string(const LPCWSTR& s);
std::optional<AudioStreamInfo> get_stream_info(IAudioClient* audioClient);

enum class AudioDeviceDirection {
    Input,
    Output
};

struct AudioRecording {
    unsigned short channels = 0;
    unsigned int samplesPerSecond = 0;
    unsigned long durationMs = 0;

    std::vector<float> data;
};

struct FrameInfo {
    double time;
    long ordinalNumber;
};