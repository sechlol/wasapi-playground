#include <iostream>

#include "main_capture.hpp"
#include "main_render.hpp"
#include "main_log.hpp"

#include "AudioDeviceProvider.h"
#include "DeviceEnumerator.h"

using std::cout;
using std::endl;

int device_events() {
	CMMNotificationClient cli;
	DeviceEnumerator deviceEnum;

	cout << " - Will log events when devices are added or removed" << endl;
	cout << " - Press ESC to exit" << endl;

	deviceEnum.register_notifications(&cli);

	while (GetAsyncKeyState(VK_ESCAPE) == 0)
		Sleep(10);

	deviceEnum.unregister_notifications(&cli);
	return 0;
}

int main()
{
	const int demo = 3;
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
