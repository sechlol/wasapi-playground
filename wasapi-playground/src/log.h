#pragma once
#include <optional>
#include <MMDeviceAPI.h>

#include "DeviceEnumerator.h"

void log_all_devices_list(const AudioDeviceList& all_devices);
void log_device_details(const AudioDeviceDetails& deviceInfo);