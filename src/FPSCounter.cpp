#include "FPSCounter.hpp"
#include <iostream>

FPSCounter::FPSCounter(double updateRateInMilliseconds): numFrames(0), prevTime(time(NULL)), currTime(time(NULL)), updateRateInMilliseconds(updateRateInMilliseconds) {}

int FPSCounter::getFPS()
{
	return FPS;
}

void FPSCounter::updateCounter()
{
	numFrames++;
	currTime = currTimerTime = time(NULL);
	if(!timerElapsed && currTimerTime - prevTimerTime >= timerFreq/1000.0)
	{
		timerElapsed = true;
	}
	if(currTime - prevTime > updateRateInMilliseconds/1000.0)
	{
		FPS = numFrames/(currTime - prevTime);
		millisecondsPerFrame = (currTime - prevTime)*1000.0/(double)numFrames;
		numFrames = 0;
		prevTime = currTime;
		return;
	}
}

void FPSCounter::resetCounter(double updateRateInMilliseconds)
{
	numFrames = 0;
	currTime = prevTime = time(NULL);
	this->updateRateInMilliseconds = updateRateInMilliseconds;
}

double FPSCounter::getMillisecondsPerFrame()
{
	return millisecondsPerFrame;
}

void FPSCounter::setTimerFreq(double timerFreq)
{
	this->timerFreq = timerFreq;
	prevTimerTime = time(NULL);
	timerElapsed = false;
}

bool FPSCounter::timer()
{
	if(!timerElapsed)
		return timerElapsed;
	else
	{
		timerElapsed = false;
		prevTimerTime = currTimerTime = time(NULL);
		return true;
	}
}