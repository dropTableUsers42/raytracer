#ifndef _FPSCOUNTER_H
#define _FPSCOUNTER_H

#include <time.h>

class FPSCounter
{
	int numFrames;
	double prevTime;
	double currTime;
	int FPS;
	double millisecondsPerFrame;
	double updateRateInMilliseconds;
	double prevTimerTime;
	double currTimerTime;
	double timerFreq;
	bool timerElapsed;


public:
	FPSCounter(double updateRateInMilliseconds=250.0);
	int getFPS();
	void updateCounter();
	void resetCounter(double updateRateInMilliseconds=250.0);
	double getMillisecondsPerFrame();
	void setTimerFreq(double timerFreq=1000.0);
	bool timer();
};

#endif