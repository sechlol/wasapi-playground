#include "DeviceEnumerator.h"
#include "common.h"
#include "log.h"

int main_log() {
	DeviceEnumerator enumerator;
	auto devices = enumerator.get_all_devices_summary();
	
	for (const auto& deviceInfo : devices.inputDevices) {
		log_device_details(enumerator.get_device_by_id(deviceInfo.id)->get_info());
	}

	for (const auto& deviceInfo : devices.outputDevices)
		log_device_details(enumerator.get_device_by_id(deviceInfo.id)->get_info());

	return 0;
}