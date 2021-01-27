#pragma once

#include <vector>
#include <optional>
#include <memory>

#include <MMDeviceAPI.h>

#include "AudioDevice.h"
#include "common.h"

struct AudioDeviceList {
	std::vector<AudioDeviceSummary> inputDevices;
	std::vector<AudioDeviceSummary> outputDevices;
};

class DeviceEnumerator
{
public:
	DeviceEnumerator();
	~DeviceEnumerator();
	
	std::unique_ptr<AudioDevice> get_default_output();
	std::unique_ptr<AudioDevice> get_default_input();
	std::unique_ptr<AudioDevice> get_device_by_id(const std::string& deviceId);

	std::vector<AudioDeviceSummary> get_output_devices_summary();
	std::vector<AudioDeviceSummary> get_input_devices_summary();
	AudioDeviceList get_all_devices_summary();
	
private:
	std::vector<AudioDeviceSummary> get_audio_devices_of_direction(EDataFlow direction);
	std::optional<AudioDeviceSummary> get_device_summary(IMMDeviceCollection* device_collection, unsigned int device_index);
	IMMDeviceEnumerator* deviceEnumerator;
};

