#define _USE_MATH_DEFINES

#include "Synthesizer.h"
#include <array>
#include <windows.h>
#include <math.h>

namespace {
    const double octaveBaseFrequency = 440.0;     			// frequency of octave represented by keyboard
    const double d12thRootOf2 = pow(2.0, 1.0 / 12.0);		// assuming western 12 notes per ocatve
    const std::chrono::milliseconds threadPause = std::chrono::milliseconds(10);
    const std::array<unsigned int, 15> keys = { 'Z','S','X','C','F','V','G','B','N','J','M','K', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2 };

    double wavePeriodProgression(double currentTime, double frequency) {
        auto cycles = (long)(currentTime * frequency);
        auto currentCycleTime = currentTime - (cycles / frequency);
        return currentCycleTime * frequency;
    }
}

Synthesizer::Synthesizer()
{
    running = true;
    inputLoop = std::thread(&Synthesizer::read_keystrokes, this);
}

Synthesizer::~Synthesizer()
{
    running = false;
    inputLoop.join();
}

double Synthesizer::sine_from_keystrokes(FrameInfo frame) const
{
    return sin((frequencyOutput * M_PI * 2) * frame.time);
}

double Synthesizer::square_from_keystrokes(FrameInfo frame) const
{
    if (frequencyOutput == 0)
        return 0;

    return wavePeriodProgression(frame.time, frequencyOutput) <= 0.5 ? 1 : -1;
}

double Synthesizer::sawtooth_from_keystrokes(FrameInfo frame) const
{
    if (frequencyOutput == 0)
        return 0;

    return (wavePeriodProgression(frame.time, frequencyOutput) * 2) - 1;
}

double Synthesizer::triangle_from_keystrokes(FrameInfo frame) const
{
    if (frequencyOutput == 0)
        return 0;

    auto progress = wavePeriodProgression(frame.time, frequencyOutput);
    if (progress <= 0.25)
    {
        return progress / 0.25;
    }
    else if (progress <= 0.75) {
        return 2 - (progress * 4);
    }
    else {
        return (progress / 0.25) - 4;
    }
}

void Synthesizer::read_keystrokes()
{
    int currentKeyIndex = -1;
    bool isAnyKeyDown = false;

    while (running) {
        isAnyKeyDown = false;
        for (size_t k = 0; k < keys.size(); k++) {
            if (GetAsyncKeyState(keys[k]) & 0x8000) {
                if (currentKeyIndex != k){
                    frequencyOutput = octaveBaseFrequency * pow(d12thRootOf2, k);
                    currentKeyIndex = k;
                }

                isAnyKeyDown = true;
            }
        }

        if (!isAnyKeyDown) {
            if (currentKeyIndex != -1)
                currentKeyIndex = -1;

            frequencyOutput = 0.0;
        }

        std::this_thread::sleep_for(threadPause);
    }
}
