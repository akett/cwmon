class Timer
{
public:
	long original_duration = 49220000;
	long duration_mod = 0;
	struct timespec loop_start, loop_end;
	double loop_actual = 0.0;
	int time_per_quarter = 50;

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

	double roundToQuarterTime(double seconds)
	{
		int sec = (int)round(seconds);
		int mod = sec % time_per_quarter;
		if(mod > (time_per_quarter/2)) {
			return sec + time_per_quarter - mod;
		} else {
			return sec - mod;
		}
	}
};