#include <optional>
#include <iostream>
#include <vector>
#include <string>
#include <future>
#include <bitset>
#include <deque>

#include "log.h"
#include "DeviceEnumerator.h"
#include "AudioRenderer.h"
#include "AudioCapturer.h"

using std::cout;
using std::endl;

const unsigned int bufferSizeLenghtMs = 16;

int main_stream_capture() {
    DeviceEnumerator deviceEnumerator;
    const auto& allDevices = deviceEnumerator.get_all_devices_summary();

    log_all_devices_list(allDevices);

    const auto& [inDevices, outDevices] = allDevices;

    unsigned int inDeviceChoice = 0;
    unsigned int outDeviceChoice = 0;
    cout << "Select an INPUT device to play a sound to: ";
    //std::cin >> deviceChoice;
    cout << endl;

    if (inDeviceChoice < 0 || inDeviceChoice >= inDevices.size())
    {
        cout << "Invalid selection" << endl;
        inDeviceChoice = 0;
    }

    cout << "Select an OUTPUT device to play a sound to: ";
    //std::cin >> deviceChoice;
    cout << endl;

    if (outDeviceChoice < 0 || outDeviceChoice >= outDevices.size())
    {
        cout << "Invalid selection" << endl;
        outDeviceChoice = 0;
    }

    auto inDevicePointer = deviceEnumerator.get_device_by_id(inDevices[inDeviceChoice].id);
    auto outDevicePointer = deviceEnumerator.get_device_by_id(outDevices[outDeviceChoice].id);

    log_device_details(inDevicePointer->get_info());
    log_device_details(outDevicePointer->get_info());

    AudioCapturer capturer(std::move(inDevicePointer));
    AudioRenderer renderer(std::move(outDevicePointer));

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
        std::deque<float> dataBuffer;

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
                dataBuffer.clear();
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

                capturer.start_streaming([&dataBuffer](BYTE* inBuffer, UINT32 framesNum) {
                    if (inBuffer == nullptr || framesNum == 0)
                        return;

                    auto floatBuffer = reinterpret_cast<float*>(inBuffer);
                    for (size_t i = 0; i < framesNum; i++)
                    {
                        dataBuffer.push_back(floatBuffer[i]);
                    }
                });

                renderer.start([&dataBuffer](FrameInfo frame) {
                    if (!dataBuffer.empty()) {
                        double frame = dataBuffer.front();
                        dataBuffer.pop_front();
                        return frame;
                    }

                    return 0.0;
                });
            }

             if (isStreaming)
                 cout << dataBuffer.size()<< endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            if (GetAsyncKeyState(VK_ESCAPE) != 0)
                break;
        }
        });

    readCommands.join();
    return 0;
}