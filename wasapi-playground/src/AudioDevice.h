#pragma once
#include <string>
#include <optional>

#include <AudioClient.h>
#include <endpointvolume.h>
#include <MMDeviceAPI.h>
#include <DeviceTopology.h>

#include "common.h"

struct AudioDeviceSummary {
    std::string id;
    std::string friendlyName;
    EDataFlow direction;
    ConnectorType type;
};

struct AudioInfo1 {
    REFERENCE_TIME defaultDevicePeriod;
    REFERENCE_TIME minDevicePeriod;
    std::optional<WAVEFORMATEX> streamFormat = std::nullopt;
    std::optional<WAVEFORMATEXTENSIBLE> extendedStreamFormat = std::nullopt;
};

struct AudioInfo2 {
    REFERENCE_TIME minSupportedBufferDuration;
    REFERENCE_TIME maxSupportedBufferDuration;
    BOOL isOffloadCapable;
};

struct AudioInfo3 {
    std::optional<WAVEFORMATEX> currentSharedModeFormat;
    UINT32 currentSharedModePeriodInFrames;
    UINT32 defaultPeriodInFrames;
    UINT32 minPeriodInFrames;
    UINT32 maxPeriodInFrames;
    UINT32 fundamentalPeriodInFrames;
};

struct VolumeInfo {
    bool muted;
    float masterVolume;
};

struct AudioDeviceDetails {
    std::optional<AudioDeviceSummary> summary = std::nullopt;
    std::optional<AudioInfo1> extendedInfo1 = std::nullopt;
    std::optional<AudioInfo2> extendedInfo2 = std::nullopt;
    std::optional<AudioInfo3> extendedInfo3 = std::nullopt;
    VolumeInfo volume;
};

class AudioDevice
{
public:
    AudioDevice(IMMDevice* device);
    AudioDevice(const AudioDevice& other) = delete;
    ~AudioDevice();

	AudioDeviceSummary get_summary() const;
	AudioDeviceDetails get_info() const;
    VolumeInfo get_volume() const;

    IAudioClient3* get_audio_client() const;
    WAVEFORMATEX* get_device_format() const;
    WAVEFORMATEXTENSIBLE* get_device_format_extended() const;

private:
	IMMDevice* device;
};

