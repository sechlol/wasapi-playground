#include <optional>
#include <iostream>
#include <vector>
#include <string>
#include <future>
#include <bitset>
#include <deque>

#include "Synthesizer.h"
#include "DeviceEnumerator.h"
#include "AudioRenderer.h"
#include "AudioCapturer.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

const unsigned int bufferSizeLenghtMs = 16;

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

int render_main(DeviceEnumerator& deviceEnumerator, std::vector<AudioDeviceInfo>& outputDevices) {
    unsigned int deviceChoice = 0;
    cout << "Select an 0UTPUT device to play a sound to: " << endl;
    std::cin >> deviceChoice;

    if (deviceChoice < 0 || deviceChoice >= outputDevices.size())
    {
        cout << "Invalid selection" << endl;
        return -1;
    }

    auto deviceId = outputDevices[deviceChoice].id;
    auto devicePointer = deviceEnumerator.get_device_by_id(deviceId);

    AudioRenderer audioRenderer(devicePointer);

    if (auto error = audioRenderer.initialize(bufferSizeLenghtMs); error.has_value()) {
        cout << "Audio Renderer failed to initialize. Aborting" << endl;
        return -1;
    }

    Synthesizer synth;
    auto synthFunction = std::bind(&Synthesizer::sawtooth_from_keystrokes, &synth, std::placeholders::_1);
    audioRenderer.start(synthFunction);

    cout << "Press some keys to play. ESC to quit.\n";
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

int capture_main(DeviceEnumerator& deviceEnumerator, std::vector<AudioDeviceInfo> inputDevices) {
    unsigned int deviceChoice = 0;
    cout << "Select an INPUT device to play a sound to: " << endl;
    //std::cin >> deviceChoice;

    if (deviceChoice < 0 || deviceChoice >= inputDevices.size())
    {
        cout << "Invalid selection" << endl;
        return -1;
    }

    auto deviceId = inputDevices[deviceChoice].id;
    auto devicePointer = deviceEnumerator.get_device_by_id(deviceId);

    AudioCapturer capturer(devicePointer);
    AudioRenderer renderer(deviceEnumerator.get_default_output());

    if (auto error = capturer.initialize(bufferSizeLenghtMs); error.has_value()) {
        cout << "Audio Capturer failed to initialize. Aborting" << endl;
        return -1;
    }

    if (auto error = renderer.initialize(bufferSizeLenghtMs); error.has_value()) {
        cout << "Audio Renderer failed to initialize. Aborting" << endl;
        return -1;
    }

    cout << "- Keep pressed Q to record" << endl;
    cout << "- Keep pressed W to play last recorded audio" << endl;
    cout << "- Keep pressed SPACE to stream input to output" << endl;
    cout << "- Press ESC to quit" << endl << endl;

    auto readCommands = std::thread([&capturer, &renderer]() {
        bool isRecording = false;
        bool isPlaying = false;
        bool stopRenderer = false;
        bool isStreaming = false;

        AudioRecording lastRecording;
        std::future<AudioRecording> recordingFuture;
        std::deque<float> ringBuffer;

        while (true) {
            auto isRecDown = std::bitset<16>(GetAsyncKeyState('Q')).test(15);
            auto isPlayDown = std::bitset<16>(GetAsyncKeyState('W')).test(15);
            auto isStreamingDown = std::bitset<16>(GetAsyncKeyState(VK_SPACE)).test(15);
            
            //stop recording
            if (isRecording && !isRecDown) {
                isRecording = false;
                capturer.stop();
                lastRecording = recordingFuture.get();

                cout << "Got " << lastRecording.data.size() << " samples (" << (float)lastRecording.durationMs / 1000.0 << "s)" << endl;
            }

            //stop playing
            else if (isPlaying && !isPlayDown) {
                cout << " Stop playing!" << endl;
                isPlaying = false;
                renderer.stop();
            }

            // stop streaming
            else if (isStreaming && !isStreamingDown) {
                cout << " Stop streaming!" << endl;
                isStreaming = false;
                capturer.stop();
                renderer.stop();
                ringBuffer.clear();
            }

            // start recording
            else if (isRecDown && !isRecording && !isStreaming && !isPlaying) {
                cout << "Now recording... ";
                recordingFuture = capturer.start_recording();
                isRecording = true;
            }
           
            //start playing
            else if (isPlayDown && !isRecording && !isStreaming && !isPlaying) {

                isPlaying = true;
                if (lastRecording.durationMs != 0) {
                    cout << "Now playing... ";
                    renderer.start([&](FrameInfo info) {
                        auto currentFrame = info.ordinalNumber % lastRecording.data.size();
                        return (double)lastRecording.data[currentFrame];
                        });
                }
                else {
                    cout << "Nothing to play...";
                }
            }
            
            //start streaming
            else if (isStreamingDown && !isRecording && !isStreaming && !isPlaying) {
                cout << "Now Streaming... ";
                isStreaming = true;

                capturer.start_streaming([&ringBuffer](BYTE* inBuffer, UINT32 framesNum) {
                    if (inBuffer == nullptr || framesNum == 0)
                        return;

                    auto floatBuffer = reinterpret_cast<float*>(inBuffer);
                    for (size_t i = 0; i < framesNum; i++)
                    {
                        ringBuffer.push_back(floatBuffer[i]);
                    }
                 });

                renderer.start([&ringBuffer](FrameInfo frame) {
                    if (!ringBuffer.empty()) {
                        double frame;
                        try {
                            frame = ringBuffer.front();
                        }
                        catch (std::exception e) {
                            cout << "madonna demente"<<endl;
                            return 0.0;
                        }
                        ringBuffer.pop_front();
                        return frame;
                    }
                   
                    return 0.0;
                });
            }

            if (isStreaming)
                cout << ringBuffer.size()<< endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            if (GetAsyncKeyState(VK_ESCAPE) != 0)
                break;
        }
    });

    readCommands.join();
    return 0;
}

int main()
{
    DeviceEnumerator deviceEnumerator;
    auto allDevices = deviceEnumerator.enumerate_audio_devices();
    
    print_device_list(allDevices);
    if (!allDevices.has_value()) {
        return -1;
    }

    return capture_main(deviceEnumerator, allDevices.value().input_devices);
    //return render_main(deviceEnumerator, allDevices.value().output_devices);
}