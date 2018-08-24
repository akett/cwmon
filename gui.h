#ifndef _GUI_H_INCLUDED_
#define _GUI_H_INCLUDED_

const int COLOR_BLUE_BLACK   = 1;
const int COLOR_CYAN_BLACK   = 2;
const int COLOR_GREEN_BLACK  = 3;
const int COLOR_YELLOW_BLACK = 4;
const int COLOR_RED_BLACK    = 5;
const int COLOR_WHITE_BLACK  = 6;

const int COLOR_BLACK_BLUE   = 7;
const int COLOR_BLACK_CYAN   = 8;
const int COLOR_BLACK_GREEN  = 9;
const int COLOR_BLACK_YELLOW = 10;
const int COLOR_BLACK_RED    = 11;
const int COLOR_BLACK_WHITE  = 12;

const int COLOR_RED_WHITE    = 13;

#include <ncurses.h>

class Window
{
public:
	// get coordinates
	int xmax,ymax;

	int vertical_quad_width;
	int vertical_quad_height;
	int vertical_quad_top = 2;

	MEVENT event;
	bool mouse_pressed = false;
	int mouse_x, mouse_y;
	int c;

	Window () {
		WINDOW *win = initscr();
		cbreak();
		noecho();
		curs_set(0);
		nodelay(win, true);

		keypad(win, true);

		getmaxyx(win, ymax, xmax);

		ymax = ymax - 1;
		xmax = xmax - 1;

		vertical_quad_width = ((xmax+4) / 4) - 1;
		vertical_quad_height = ymax - 6;

		start_color();
		// black text color background
		init_pair(1, COLOR_BLACK, COLOR_BLUE);
		init_pair(2, COLOR_BLACK, COLOR_CYAN);
		init_pair(3, COLOR_BLACK, COLOR_GREEN);
		init_pair(4, COLOR_BLACK, COLOR_YELLOW);
		init_pair(5, COLOR_BLACK, COLOR_RED);
		init_pair(6, COLOR_BLACK, COLOR_WHITE);

		// color text black background
		init_pair(7, COLOR_BLUE, COLOR_BLACK);
		init_pair(8, COLOR_CYAN, COLOR_BLACK);
		init_pair(9, COLOR_GREEN, COLOR_BLACK);
		init_pair(10, COLOR_YELLOW, COLOR_BLACK);
		init_pair(11, COLOR_RED, COLOR_BLACK);
		init_pair(12, COLOR_WHITE, COLOR_BLACK);

		// color text color background
		init_pair(13, COLOR_WHITE, COLOR_RED);

		mousemask(ALL_MOUSE_EVENTS, NULL);
	}

	void checkInput() {
		c = getch();
		switch(c) {
			case KEY_MOUSE:
			if(getmouse(&event) == OK)
			{
				if(event.bstate & BUTTON1_PRESSED) mouse_pressed = true;
				if(event.bstate & BUTTON1_RELEASED) mouse_pressed = false;
			}
		}
	}

	void onLoopStart() {
		erase();
	}

	void onLoopEnd() {
		refresh();
	}

	void end() {
		endwin();
	}
};

#endif