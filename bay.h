#ifndef _BAY_H_INCLUDED
#define _BAY_H_INCLUDED

#include <wiringPi.h>

double getElapsedTime(timespec *start, timespec *end)
{
	return (double)(end->tv_sec - start->tv_sec) + (double)end->tv_nsec / 1000000000.0 - (double)start->tv_nsec / 1000000000.0;
}

class Bay
{
public:
	int bay_id;
	int timer_pin;
	int pump_pin;
	int manual_coin_pin;

	bool timer_pin_state = false;
	bool pump_pin_state = false;
	bool manual_coin_pin_state = true;

	struct timespec now,
		timer_pin_start,
		timer_pin_end,
		pump_pin_start,
		pump_pin_end,
		manual_coin_pin_start,
		manual_coin_pin_end;

	bool timer_running = false;
	bool pump_running = false;
	bool manual_coin_pressed = true;

	double current_timer_time = 0;
	double current_pump_time = 0;

	double last_timer_runtime = 0;
	double last_pump_runtime = 0;

	double total_pump_runtime = 0;

	int db_manual_coin_count = 0;
	double db_total_timer_time = 0;
	double db_total_pump_time = 0;

	Bay(int bay_id, int timer_pin, int pump_pin, int manual_coin_pin)
	{
		this->bay_id = bay_id;
		this->timer_pin = timer_pin;
		this->pump_pin = pump_pin;
		this->manual_coin_pin = manual_coin_pin;
	}

	void setupPins()
	{
		pinMode(timer_pin, INPUT);
		pinMode(pump_pin, INPUT);
		pinMode(manual_coin_pin, INPUT);
		pullUpDnControl(timer_pin, PUD_UP);
		pullUpDnControl(pump_pin, PUD_UP);
		pullUpDnControl(manual_coin_pin, PUD_UP);
	}

	void takedownPins()
	{
		pullUpDnControl(timer_pin, PUD_DOWN);
		pullUpDnControl(pump_pin, PUD_DOWN);
		pullUpDnControl(manual_coin_pin, PUD_DOWN);
	}

	void checkPins(Database *db, Timer *timer)
	{
		clock_gettime(CLOCK_REALTIME, &now);

		if(digitalRead(timer_pin) == LOW) {
			if(timer_pin_state == false) {
				if(timer_running == false) {
					clock_gettime(CLOCK_REALTIME, &timer_pin_start);
				}
				timer_pin_state = true;
			}

			if(timer_running == false) {
				if(getElapsedTime(&timer_pin_start, &now) >= 0.5) {
					timer_running = true;
				}
			}
		} else {
			if(timer_pin_state == true) {
				if(timer_running == true) {
					clock_gettime(CLOCK_REALTIME, &timer_pin_end);
				}
				timer_pin_state = false;
			}

			if(timer_running == true) {
				if(getElapsedTime(&timer_pin_end, &now) >= 0.1) {
					last_timer_runtime = timer->roundToMinute(getElapsedTime(&timer_pin_start, &timer_pin_end));
					double pump_time;
					if(current_pump_time > 0) {
						if(pump_running == false) {
							pump_time = total_pump_runtime;
						} else {
							if(pump_pin_state == false) {
								pump_time = getElapsedTime(&pump_pin_start, &pump_pin_end) + total_pump_runtime;
							} else {
								pump_time = getElapsedTime(&pump_pin_start, &now) + total_pump_runtime;
							}
						}
					} else {
						pump_time = 0;
					}
					last_pump_runtime = pump_time;

					total_pump_runtime = 0;
					current_pump_time = 0;
					current_timer_time = 0;
					timer_running = false;

					if(last_timer_runtime > 0) {
						db->execPrepared(db->beginPrepared("INSERT_SESSION")(bay_id)(last_timer_runtime)(last_pump_runtime));
					}

					clock_gettime(CLOCK_REALTIME, &pump_pin_start);
					clock_gettime(CLOCK_REALTIME, &pump_pin_end);
					clock_gettime(CLOCK_REALTIME, &timer_pin_start);
					clock_gettime(CLOCK_REALTIME, &timer_pin_end);
				}
			}
		}

		if(digitalRead(pump_pin) == LOW) {
			if(pump_pin_state == false) {
				if(pump_running == false) {
					clock_gettime(CLOCK_REALTIME, &pump_pin_start);
				}
				pump_pin_state = true;
			}

			if(pump_running == false) {
				if(getElapsedTime(&pump_pin_start, &now) >= 0.5) {
					pump_running = true;
				}
			}
		} else {
			if(pump_pin_state == true) {
				if(pump_running == true) {
					clock_gettime(CLOCK_REALTIME, &pump_pin_end);
				}
				pump_pin_state = false;
			}

			if(pump_running == true) {
				if(getElapsedTime(&pump_pin_end, &now) >= 0.1) {
					total_pump_runtime += getElapsedTime(&pump_pin_start, &pump_pin_end);
					if(timer_running == false) {
						current_pump_time = 0;
						total_pump_runtime = 0;
					}
					pump_running = false;
				}
			}
		}

		if(digitalRead(manual_coin_pin) == HIGH) {
			if(manual_coin_pin_state == false) {
				if(manual_coin_pressed == false) {
					clock_gettime(CLOCK_REALTIME, &manual_coin_pin_start);
				}
				manual_coin_pin_state = true;
			}

			if(manual_coin_pressed == false) {
				if(getElapsedTime(&manual_coin_pin_start, &now) >= 0.025) {
					manual_coin_pressed = true;
					db->execPrepared(db->beginPrepared("INSERT_COIN")(bay_id));
				}
			}
		} else {
			if(manual_coin_pin_state == true) {
				if(manual_coin_pressed == true) {
					clock_gettime(CLOCK_REALTIME, &manual_coin_pin_end);
				}
				manual_coin_pin_state = false;
			}

			if(manual_coin_pressed == true) {
				if(getElapsedTime(&manual_coin_pin_end, &now) >= 0.025) {
					manual_coin_pressed = false;
				}
			}
		}

		if(timer_running) {
			current_timer_time = getElapsedTime(&timer_pin_start, &now);
		}
		if(pump_running) {
			current_pump_time = getElapsedTime(&pump_pin_start, &now) + total_pump_runtime;
		}
	}
};

#endif