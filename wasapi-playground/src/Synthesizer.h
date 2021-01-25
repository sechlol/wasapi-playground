#pragma once

#include <thread>
#include <atomic>

#include "common.h"

class Synthesizer
{
public:
	Synthesizer();
	~Synthesizer();

	double sine_from_keystrokes(FrameInfo frame) const;
	double square_from_keystrokes(FrameInfo frame) const;
	double sawtooth_from_keystrokes(FrameInfo frame) const;
	double triangle_from_keystrokes(FrameInfo frame) const;

private:
	void read_keystrokes();

	std::thread inputLoop;
	std::atomic_bool running;
	std::atomic<double> frequencyOutput;
};

