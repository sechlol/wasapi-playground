#pragma once

#include <thread>
#include <atomic>

class Synthesizer
{
public:
	Synthesizer();
	~Synthesizer();
	double sine_from_keystrokes(double time) const;

private:
	void read_keystrokes();

	std::thread inputLoop;
	std::atomic_bool running;
	std::atomic<double> frequencyOutput;
};

