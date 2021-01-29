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

int main()
{
	const int demo = 1;
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
	}
}
