#include <optional>
#include <iostream>
#include <vector>
#include <string>
#include <future>

#include "Synthesizer.h"
#include "DeviceEnumerator.h"
#include "AudioRenderer.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

void print_device_list(std::optional<AudioDevices>& all_devices) {
    if (all_devices.has_value()) {
        cout << "Listing INPUT devices:" << endl;
        for (size_t i = 0; i < all_devices.value().input_devices.size(); i++)
        {
            auto& device = all_devices.value().input_devices[i];
            cout << "\t - " << i << " " << device.friendly_name << " - " << device.id << endl;
        }

        cout << endl << "Listing OUTPUT devices:" << endl;
        for (size_t i = 0; i < all_devices.value().output_devices.size(); i++)
        {
            auto& device = all_devices.value().output_devices[i];
            cout << "\t - " << i << " " << device.friendly_name << " - " << device.id << endl;
        }
    }
    else {
        cout << "No devices to list" << endl;
    }
}

bool check_keystroke() {
    while (true)
    {
        Sleep(30);

        if (GetAsyncKeyState(VK_SPACE) != 0)
            return true;
    }
}

int main()
{
    const unsigned int bufferSizeLenghtMs = 16;
    auto device_enumerator = std::make_unique<DeviceEnumerator>();
    auto all_devices = device_enumerator->enumerate_audio_devices();
    
    print_device_list(all_devices);
    if (!all_devices.has_value()) {
        return -1;
    }

    unsigned int deviceChoice = 0;
    cout << "Select an 0UTPUT device to play a sound to: " << endl;
    std::cin >> deviceChoice;

    if (deviceChoice < 0 || deviceChoice >= all_devices.value().output_devices.size())
    {
        cout << "Invalid selection" <<endl;
        return -1;
    }

    auto deviceId = all_devices.value().output_devices[deviceChoice].id;
    auto devicePointer = device_enumerator->get_device_by_id(deviceId);

    AudioRenderer audioRenderer(devicePointer);

    if (auto error = audioRenderer.initialize(bufferSizeLenghtMs); error.has_value()) {
        cout << "Audio Renderer failed to initialize. Aborting"<<endl;
        return -1;
    }

    Synthesizer synth;
    auto synthFunction = std::bind(&Synthesizer::sawtooth_from_keystrokes, &synth, std::placeholders::_1);
    audioRenderer.start(synthFunction);

    cout << "Press some keys to play. SPACE to quit.\n";
    auto stopFuture = std::async(std::launch::async, check_keystroke);
    while (stopFuture.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready);

    audioRenderer.stop();
    return 0;
}