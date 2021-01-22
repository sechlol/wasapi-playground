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

double Synthesizer::sine_from_keystrokes(double time) const
{
    return sin((frequencyOutput * M_PI * 2) * time);
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
