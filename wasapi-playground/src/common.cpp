#include "common.h"
#include <string>

std::wstring string_to_wstring(const std::string& s)
{
    std::wstring temp;
    for (const auto& c : s) {
        temp += c;
    }
    return temp;
}

std::optional<AudioStreamInfo> get_stream_info(IAudioClient* audioClient)
{
    AudioStreamInfo info{};
    auto result = audioClient->GetBufferSize(&info.bufferSizeInFrames);
    if (FAILED(result)) {
        printf("[get_stream_info()] failed to GetBufferSize(): %x\n", result);
        return std::nullopt;
    }

    result = audioClient->GetStreamLatency(&info.latency);
    if (FAILED(result)) {
        printf("[get_stream_info()] failed to GetStreamLatency(): %x\n", result);
        return std::nullopt;
    }

    return info;
}
