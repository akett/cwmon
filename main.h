#ifndef _MAIN_H_INCLUDED_
#define _MAIN_H_INCLUDED_

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <time.h>
#include <wiringPi.h>
#include <cmath>
#include <thread>

// declare global variables :DDD
bool stop_program = false;

void stopProgram ()
{
	stop_program = true;
}

void sig_handler(int signo)
{
	if (signo == SIGINT || signo == SIGUSR1) {
		stopProgram();
	}
}

void registerSignals()
{
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		std::cout << "can't catch SIGINT" << std::endl;
		stopProgram();
	}
	if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
		std::cout << "can't catch SIGUSER1" << std::endl;
		stopProgram();
	}
}

class Timer
{
public:
	long original_duration = 49220000;
	long duration_mod = 0;
	struct timespec loop_start, loop_end;
	double loop_actual = 0.0;

	void onLoopStart()
	{	
		clock_gettime(CLOCK_REALTIME, &loop_start);
	}

	void onLoopEnd()
	{
		struct timespec loop_delay = {0, original_duration + duration_mod};
		nanosleep(&loop_delay, NULL);
		clock_gettime(CLOCK_REALTIME, &loop_end);
		loop_actual = (double)(loop_end.tv_sec - loop_start.tv_sec) + (double)loop_end.tv_nsec / 1000000000.0 - (double)loop_start.tv_nsec / 1000000000.0;
	}

	std::string toDurationString(double seconds, bool includeSeconds)
	{
		std::string sec = (includeSeconds || seconds < 1) ? std::to_string(std::fmod(round(seconds*100)/100, 60)).substr(0,4) + "s" : "";
		std::string duration;
		if(seconds < 60) {
			duration = sec;
		} else if(seconds >= 60 && seconds < 3600) {
			duration = std::to_string((int)seconds / 60) + "m " + sec;
		} else if(seconds >= 3600) {
			duration = std::to_string((int)seconds / 3600) + "h " + std::to_string((int)seconds % 3600 / 60) + "m " + sec;
		}

		return duration;
	}

	double roundToMinute(double seconds)
	{
		int sec = (int)round(seconds);
		int mod = sec % 60;
		if(mod > 30) {
			return sec + 60 - mod;
		} else {
			return sec - mod;
		}
	}
};

#endif