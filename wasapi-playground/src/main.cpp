#include <iostream>

#include "main_capture.hpp"
#include "main_render.hpp"
#include "main_log.hpp"

#include "DeviceNotificationProvider.h"

using std::cout;
using std::endl;

int device_events() {
	DeviceNotificationProvider notifications;

	cout << " - Will log events when devices are added or removed" << endl;
	cout << " - Press ESC to exit" << endl;

	notifications.subscribe_to_global_events([](std::string id, DeviceEvent evt) {
		if (evt != DeviceEvent::PropertyChanged)
			cout << "Device event: " << (int)evt << " id: " << id<<endl;
	});

	while (GetAsyncKeyState(VK_ESCAPE) == 0)
		Sleep(10);

	return 0;
}

int log_volume_change()
{
	DeviceEnumerator enumerator;
	auto defaultOut = enumerator.get_default_output();
	log_device_details(defaultOut->get_info());

	VolumeInfo volume = defaultOut->get_volume();
	cout << "PRESS ESC TO EXIT." << endl;
	cout << "Current volume level: " << volume.masterVolume << (volume.muted ? " (muted)" : "") << "\r";

	auto subscription = defaultOut->subscribe_to_volume_changes([](VolumeInfo const& volume) {
		cout << "Current volume level: "<< volume.masterVolume << (volume.muted ? " (muted)" : "           ") << "\r";
	});

	auto stopFuture = std::async(std::launch::async, []() {
		while (true) {
			Sleep(30);
			if (GetAsyncKeyState(VK_ESCAPE) != 0)
				return true;
		}
		});

	while (stopFuture.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready);

	defaultOut->unsubscribe_volume_changes(subscription);
	return 0;
}

int main()
{
	const int demo = 4;
	switch (demo)
	{
	case 0:
		return main_log();
	case 1:
		return device_events();
	case 2:
		return main_render();
	case 3:
		return main_stream_capture();
	case 4:
		return log_volume_change();
	}
}
