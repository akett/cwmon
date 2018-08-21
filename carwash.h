#ifndef _CARWASH_H_INCLUDED_
#define _CARWASH_H_INCLUDED_

#include "bay.h"
#include <chrono>

class Carwash
{
public:

	int gui_delay = 10; // loop cycles (1 cycle = ~0.05 seconds)
	int gui_delay_count = 1;
	bool do_gui = false;

	int temp_average_to = 20; // cycles
	int temp_average_count = 0;
	double temp_average_list[20] = {0};
	double temp_average_agg = 0;
	double average_temp = 0;

	double total_revenue = 0;

	int bay = 1;

	Bay bays[4] = {
		{1, 7, 0, 3},
		{2, 1, 4, 6},
		{3, 21, 22, 24},
		{4, 26, 27, 29},
	};

	time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char* time_string = ctime(&current_time);

	// get Pi temperature
	FILE *temperatureFile;
	double temperature;


	int menu_select_pin = 25; // 37
	int menu_okay_pin = 14; // 23
	bool menu_select_pin_state = false;
	bool menu_okay_pin_state = false;

	bool menu_select = false;
	bool menu_open = false;
	bool menu_okay = false;
	int menu_options = 6;
	// 1 = shutdown
	// 2 = reboot
	// 3 = wipe everything
	// 4 = wipe pumps
	// 5 = wipe timers
	// 6 = wipe manual coins
	int menu_option = 1;
	struct timespec menu_start, menu_now, 
		menu_select_pin_start,
		menu_select_pin_end,
		menu_okay_pin_start,
		menu_okay_pin_end;

	void start() {
		wiringPiSetup();

		pinMode(menu_select_pin, INPUT);
		pinMode(menu_okay_pin, INPUT);
		pullUpDnControl(menu_select_pin, PUD_UP);
		pullUpDnControl(menu_okay_pin, PUD_UP);


		for(bay = 1; bay <= 4; bay++) {
			bays[bay-1].setupPins();
		}
	}

	void run(Database *db, Timer *timer, Window *window)
	{
		handleMenu(db, timer, window);

		if(gui_delay_count >= gui_delay) gui_delay_count = 1;
		gui_delay_count++;
		do_gui = (gui_delay_count == gui_delay);

		if(do_gui) {
			timer->duration_mod = -1000000;
			pqxx::result coinResults = db->execPrepared(db->beginPrepared("SELECT_COINS"));
			pqxx::result sessionResults = db->execPrepared(db->beginPrepared("SELECT_SESSIONS"));
			pqxx::result lastSessionResult = db->execPrepared(db->beginPrepared("SELECT_LAST_SESSION"));

			for(bay = 1; bay <= 4; bay++) {
				bool coinRowExists = false;
				bool sessionRowExists = false;
				for(auto coinRow: coinResults) {
					int coinBay;
					coinRow[0].to(coinBay);
					if(coinBay == bays[bay-1].bay_id) {
						coinRow[1].to(bays[bay-1].db_manual_coin_count);
						coinRowExists = true;
					}
				}
				for(auto sessionRow: sessionResults) {
					int sessionBay;
					sessionRow[0].to(sessionBay);
					if(sessionBay == bays[bay-1].bay_id) {
						sessionRow[1].to(bays[bay-1].db_total_timer_time);
						sessionRow[2].to(bays[bay-1].db_total_pump_time);
						sessionRowExists = true;
					}
				}
				if(coinRowExists == false) {
					bays[bay-1].db_manual_coin_count = 0;
				}
				if(sessionRowExists == false) {
					bays[bay-1].db_total_timer_time = 0;
					bays[bay-1].db_total_pump_time = 0;
				}
			}
		} else {
			timer->duration_mod = 0;
		}

		// print time
		current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		time_string = ctime(&current_time);
		mvprintw(0, window->xmax/2 - 2 - strlen(time_string) / 2, "%s", time_string);

		// print operating temperature
		getTemperature();
		int temp_color = 2;
		if(average_temp > 104) temp_color = 3;
		if(average_temp > 120) temp_color = 4;
		if(average_temp > 127) temp_color = 5;
		attron(COLOR_PAIR(temp_color));
		mvprintw(0, 0, " TEMP: %6.3f F. ", average_temp);
		attroff(COLOR_PAIR(temp_color));

		// handle bays
		total_revenue = 0;
		for(bay = 1; bay <= 4; bay++) {
			Bay *currentBay = &bays[bay-1];

			currentBay->checkPins(db, timer);

			int vq_left = (window->vertical_quad_width * bay) - window->vertical_quad_width;
			int vq_right = (window->vertical_quad_width * bay);
			int vq_center = (window->vertical_quad_width * bay) - (window->vertical_quad_width / 2) + 1;

			int titleLength = 17;
			std::string timerTitle = " TIMER          ";
			std::string timerTitleDecoration = "       ";
			std::string bayTitle = " BAY " + std::to_string(bay) + " ";
			std::string pumpTitle = "           PUMP ";
			std::string pumpTitleDecoration = "      ";
			attron(COLOR_PAIR(currentBay->timer_running ? COLOR_GREEN_BLACK : COLOR_RED_WHITE));
			mvprintw(window->vertical_quad_top + 1,     vq_center - (titleLength / 2), timerTitle.c_str());
			mvprintw(window->vertical_quad_top + 2, vq_center - (titleLength / 2) - 2, timerTitleDecoration.c_str());
			attroff(COLOR_PAIR(currentBay->timer_running ? COLOR_GREEN_BLACK : COLOR_RED_WHITE));

			attron(COLOR_PAIR(currentBay->timer_running ? COLOR_BLACK_CYAN : COLOR_BLACK_GREEN));
			mvprintw(window->vertical_quad_top + 2, vq_center - (strlen(bayTitle.c_str()) / 2), bayTitle.c_str());
			attroff(COLOR_PAIR(currentBay->timer_running ? COLOR_BLACK_CYAN : COLOR_BLACK_GREEN));

			attron(COLOR_PAIR(currentBay->pump_running ? COLOR_GREEN_BLACK : COLOR_RED_WHITE));
			mvprintw(window->vertical_quad_top + 2, vq_center + (strlen(bayTitle.c_str()) / 2) + 1, pumpTitleDecoration.c_str());
			mvprintw(window->vertical_quad_top + 3, vq_center - (titleLength / 2), pumpTitle.c_str());
			attroff(COLOR_PAIR(currentBay->pump_running ? COLOR_GREEN_BLACK : COLOR_RED_WHITE));

			if(currentBay->timer_running) {
				attron(COLOR_PAIR(COLOR_BLACK_CYAN));
				mvprintw(window->vertical_quad_top, vq_left + 4, timer->toDurationString(currentBay->current_timer_time, true).c_str());
				attroff(COLOR_PAIR(COLOR_BLACK_CYAN));

				attron(COLOR_PAIR(COLOR_BLACK_CYAN));
				mvprintw(window->vertical_quad_top + 4, vq_right - 2 - strlen(timer->toDurationString(currentBay->current_pump_time, true).c_str()), timer->toDurationString(currentBay->current_pump_time, true).c_str());
				attroff(COLOR_PAIR(COLOR_BLACK_CYAN));
			}

			mvprintw(window->vertical_quad_top + 6, vq_left + 4, "LAST SESSION");
			attron(COLOR_PAIR(COLOR_BLACK_GREEN));
			mvprintw(window->vertical_quad_top + 7, vq_left + 4, (std::string("TIME: ") + timer->toDurationString(currentBay->last_timer_runtime, false)).c_str());
			mvprintw(window->vertical_quad_top + 8, vq_left + 4, (std::string("PUMP: ") + timer->toDurationString(currentBay->last_pump_runtime, true)).c_str());
			attroff(COLOR_PAIR(COLOR_BLACK_GREEN));


			mvprintw(window->vertical_quad_top + 10, vq_left + 4, "TIMER TOTAL");
			mvprintw(window->vertical_quad_top + 12, vq_left + 4, "PUMP TOTAL");

			attron(COLOR_PAIR(COLOR_BLACK_GREEN));
			mvprintw(window->vertical_quad_top + 11, vq_left + 5, timer->toDurationString(currentBay->db_total_timer_time, false).c_str());
			mvprintw(window->vertical_quad_top + 13, vq_left + 5, timer->toDurationString(currentBay->db_total_pump_time, true).c_str());
			attroff(COLOR_PAIR(COLOR_BLACK_GREEN));


			mvprintw(window->vertical_quad_top + 14, vq_left + 4, "MANUAL TOTAL");
			attron(COLOR_PAIR(COLOR_BLACK_GREEN));
			mvprintw(window->vertical_quad_top + 15, vq_left + 4, (std::string("-") + timer->toDurationString((double)(currentBay->db_manual_coin_count*60), false)).c_str());
			attroff(COLOR_PAIR(COLOR_BLACK_GREEN));

			mvprintw(window->vertical_quad_top + 17, vq_left + 4, "BAY REVENUE");
			attron(COLOR_PAIR(COLOR_BLACK_GREEN));
			mvprintw(window->vertical_quad_top + 18, vq_left + 5, "$%.2f", (((currentBay->db_total_timer_time + timer->roundToMinute(currentBay->current_timer_time)) - (double)(currentBay->db_manual_coin_count*60)) / 60) / 4);
			attroff(COLOR_PAIR(COLOR_BLACK_GREEN));

			total_revenue += (((currentBay->db_total_timer_time + timer->roundToMinute(currentBay->current_timer_time)) - (double)(currentBay->db_manual_coin_count*60)) / 60) / 4;

		}

		std::string tr_string = " TOTAL REVENUE: $"+std::to_string(total_revenue);
		tr_string = tr_string.substr(0, strlen(tr_string.c_str()) - 4) + " ";

		attron(COLOR_PAIR(COLOR_GREEN_BLACK));
		mvprintw(0, window->xmax - strlen(tr_string.c_str()), tr_string.c_str());
		attroff(COLOR_PAIR(COLOR_GREEN_BLACK));

		//mvprintw(window->ymax, 50, "Loop time: %lf seconds.", timer->loop_actual);
	}

	void getTemperature()
	{
		temperatureFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
		if (temperatureFile == NULL) {
	        printf("Error opening file\n");
	        stopProgram();
	    }

		fscanf (temperatureFile, "%lf", &temperature);
		temperature /= 1000;
		fclose (temperatureFile);

		if(temp_average_count > temp_average_to) temp_average_count = 0;
		temp_average_list[temp_average_count] = (double)temperature;
		temp_average_count ++;
		temp_average_agg = 0;
		for (int x = 0; x < temp_average_to; x++)
		{
			temp_average_agg += temp_average_list[x];
		}
		average_temp = (temp_average_agg / temp_average_to) * 9/5 + 32;
	}

	void handleMenu(Database *db, Timer *timer, Window *window)
	{
		clock_gettime(CLOCK_REALTIME, &menu_now);

		if(digitalRead(menu_select_pin) == LOW) {
			if(menu_select_pin_state == false) {
				clock_gettime(CLOCK_REALTIME, &menu_select_pin_start);
				menu_select_pin_state = true;
			}

			if(menu_open == false) {
				if(getElapsedTime(&menu_select_pin_start, &menu_now) >= 0.5) {
					menu_open = true;
				}
			} else {
				if(menu_select == false) {
					clock_gettime(CLOCK_REALTIME, &menu_select_pin_start);
					menu_select = true;
				} else {
					if(getElapsedTime(&menu_select_pin_start, &menu_now) >= 0.2 && getElapsedTime(&menu_select_pin_start, &menu_now) <= 0.24) {
						menu_option ++;
						if(menu_option > menu_options) {
							menu_option = 1;
						}
					}
				}
			}

		} else {
			if(menu_select_pin_state == true) {
				clock_gettime(CLOCK_REALTIME, &menu_select_pin_end);
				menu_select_pin_state = false;
			}
			if(menu_open == true) {
				if(getElapsedTime(&menu_select_pin_end, &menu_now) >= 10) {
					menu_open = false;
				}
			}
			if(menu_select == true) {
				if(getElapsedTime(&menu_select_pin_end, &menu_now) >= .5) {
					menu_select = false;
				}
			}
		}

		if(digitalRead(menu_okay_pin) == LOW) {
			if(menu_okay_pin_state == false) {
				clock_gettime(CLOCK_REALTIME, &menu_okay_pin_start);
				menu_okay_pin_state = true;
			}

			if(menu_okay == false) {
				if(getElapsedTime(&menu_okay_pin_start, &menu_now) >= 1 && menu_open == true) {
					menu_okay = true;
					switch(menu_option) {
						case 1:
							system("shutdown now");
						break;
						case 2:
							system("shutdown -r now");
						break;
						case 3:
							db->execPrepared(db->beginPrepared("WIPE_ALL_BAY_TIME"));
							db->execPrepared(db->beginPrepared("WIPE_MANUAL_TIME"));
						break;
						case 4:
							db->execPrepared(db->beginPrepared("WIPE_TIMER_TIME"));
						break;
						case 5:
							db->execPrepared(db->beginPrepared("WIPE_PUMP_TIME"));
						break;
						case 6:
							db->execPrepared(db->beginPrepared("WIPE_MANUAL_TIME"));
						break;
					}
					menu_okay_pin_state = false;
					menu_select_pin_state = false;
					menu_okay = false;
					menu_select = false;
					menu_open = false;
					menu_option = 1;
				}
			}
		} else {
			if(menu_okay_pin_state == true) {
				clock_gettime(CLOCK_REALTIME, &menu_okay_pin_end);
				menu_okay_pin_state = false;
			}
			if(menu_okay == true) {
				if(getElapsedTime(&menu_okay_pin_end, &menu_now) >= 10) {
					menu_okay = false;
				}
			}
		}

		if(menu_open) {
			mvprintw(window->ymax - 2, 10, "CONTROL MENU");
			std::string option_name;
			switch(menu_option) {
				case 1:
					option_name = " Shutdown ";
					break;
				case 2:
					option_name = " Reboot ";
					break;
				case 3:
					option_name = " Wipe everything ";
					break;
				case 4:
					option_name = " Wipe Timers ";
					break;
				case 5:
					option_name = " Wipe Pumps ";
					break;
				case 6:
					option_name = " Wipe Manual Time ";
					break;
			}
			attron(COLOR_PAIR(COLOR_GREEN_BLACK));
			mvprintw(window->ymax - 1, 10, option_name.c_str());
			attroff(COLOR_PAIR(COLOR_GREEN_BLACK));
		}
	}

	void shutdown() {
		pullUpDnControl(menu_select_pin, PUD_DOWN);
		pullUpDnControl(menu_okay_pin, PUD_DOWN);

		for(bay = 1; bay <= 4; bay++) {
			bays[bay-1].takedownPins();
		}
	}
};

#endif