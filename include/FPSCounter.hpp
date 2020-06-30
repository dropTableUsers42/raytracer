#ifndef _FPSCOUNTER_H
#define _FPSCOUNTER_H

#include <time.h>

/**
 * Class to calculate the FPS, and have a separate frequency timer.
 * Call updateCounter() at end of each frame to update fps,
 * and call timer() at end of each frame to update timer.
*/
class FPSCounter
{
	int numFrames; /**< Keeps track of number of frames since last fps update*/
	double prevTime; /**< Keeps track of last time of fps update */
	double currTime; /**< Holds current time value. Updated in updateCounter() */
	int FPS; /**< Keeps track of FPS */
	double millisecondsPerFrame; /**< Keeps track of mspf */
	double updateRateInMilliseconds; /**< Stores update rate of fps */
	double prevTimerTime; /**< Stores previous time of timer tick*/
	double currTimerTime; /**< Stores current time, updated in updateCounter() and timer() */
	double timerFreq; /**< Stores frequency of timer tick */
	bool timerElapsed; /**< Stores if the timer is ticking at the current frame */


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