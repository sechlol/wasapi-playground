#pragma once

#include <vector>
#include <optional>
#include <MMDeviceAPI.h>

#include "common.h"

class DeviceEnumerator
{
public:
	DeviceEnumerator();
	~DeviceEnumerator();

	std::optional<AudioDevices> enumerate_audio_devices();
	IMMDevice* get_device_by_id(std::string deviceId);
	
private:
	std::vector<AudioDeviceInfo> get_audio_devices_of_direction(EDataFlow direction);
	std::optional<AudioDeviceInfo> get_device_info(IMMDeviceCollection* device_collection, unsigned int device_index);
	IMMDeviceEnumerator* deviceEnumerator;
};

