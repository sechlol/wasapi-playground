#include <optional>
#include <iostream>
#include <vector>
#include <string>
#include <future>

#include "log.h"
#include "Synthesizer.h"
#include "DeviceEnumerator.h"
#include "AudioRenderer.h"
#include "AudioCapturer.h"

int main_render() {
    DeviceEnumerator deviceEnumerator;
    const auto& outputDevices = deviceEnumerator.get_output_devices_summary();

    if (outputDevices.empty()) {
        cout << "No device to list";
        return -1;
    }

    cout << "Available output devices:" << endl;
    for (size_t i = 0; i < outputDevices.size(); i++)
        cout << "\t- " << i << " " << outputDevices[i].friendlyName << endl;
    
    unsigned int deviceChoice = 0;
    cout << endl <<"Select a device to play a sound to: ";
    std::cin >> deviceChoice;
    cout << endl;

    if (deviceChoice < 0 || deviceChoice >= outputDevices.size())
    {
        cout << "Invalid selection" << endl;
        return -1;
    }
    
    const auto& deviceId = outputDevices[deviceChoice].id;
    auto audioDevice = deviceEnumerator.get_device_by_id(deviceId);
    log_device_details(audioDevice->get_info());

    AudioRenderer audioRenderer(std::move(audioDevice));

    if (auto error = audioRenderer.initialize(bufferSizeLenghtMs); error.has_value()) {
        cout << "Audio Renderer failed to initialize. Aborting" << endl;
        return -1;
    }

    Synthesizer synth;
    auto synthFunction = std::bind(&Synthesizer::triangle_from_keystrokes, &synth, std::placeholders::_1);
    audioRenderer.start(synthFunction);

    cout << "- Press these keys to play audio: Z S X C F V G B N J M K , . /\n"; 
    cout << "- Press ESC to quit.\n";
    auto stopFuture = std::async(std::launch::async, []() {
        while (true) {
            Sleep(30);
            if (GetAsyncKeyState(VK_ESCAPE) != 0)
                return true;
        }
        });

    while (stopFuture.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready);

    audioRenderer.stop();
    return 0;
}