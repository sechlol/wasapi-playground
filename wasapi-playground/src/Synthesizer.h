#pragma once

#include <thread>
#include <atomic>

class Synthesizer
{
public:
	Synthesizer();
	~Synthesizer();

	double sine_from_keystrokes(double time) const;
	double square_from_keystrokes(double time) const;
	double sawtooth_from_keystrokes(double time) const;
	double triangle_from_keystrokes(double time) const;

private:
	void read_keystrokes();

	std::thread inputLoop;
	std::atomic_bool running;
	std::atomic<double> frequencyOutput;
};

