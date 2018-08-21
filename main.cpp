#include "main.h"
#include "database.h"
#include "gui.h"
#include "carwash.h"

int main ()
{
	std::cout << "INITIATING CWMON" << std::endl;

	registerSignals();

	// setup database
	Database* db = new Database();
	db->connect();
	db->execQuery("CREATE TABLE IF NOT EXISTS bay_sessions ( \
		id BIGSERIAL, \
		bay INT NOT NULL, \
		timer_time NUMERIC(14,2), \
		pump_time NUMERIC(14,2), \
		timestamp TIMESTAMP DEFAULT current_timestamp, \
		PRIMARY KEY (id)\
	);");
	db->execQuery("CREATE TABLE IF NOT EXISTS bay_manual_inserts ( \
		id BIGSERIAL, \
		bay INT NOT NULL, \
		timestamp TIMESTAMP DEFAULT current_timestamp, \
		PRIMARY KEY (id) \
	);");


	db->prepareStatement("SELECT_SESSIONS", "SELECT bay, SUM(timer_time) as total_timer_time, SUM(pump_time) as total_pump_time FROM bay_sessions GROUP BY bay ORDER BY bay ASC;");
	db->prepareStatement("SELECT_LAST_SESSION", "SELECT bay, timer_time, pump_time FROM bay_sessions ORDER BY bay DESC LIMIT 1;");
	db->prepareStatement("SELECT_COINS", "SELECT bay, count(*) as coins FROM bay_manual_inserts GROUP BY bay ORDER BY bay ASC;");
	
	db->prepareStatement("INSERT_SESSION", "INSERT INTO bay_sessions (bay, timer_time, pump_time) VALUES ($1, $2, $3)");
	db->prepareStatement("INSERT_COIN", "INSERT INTO bay_manual_inserts (bay) VALUES ($1);");

	db->prepareStatement("WIPE_ALL_BAY_TIME", "DELETE FROM bay_sessions;");
	db->prepareStatement("WIPE_TIMER_TIME", "UPDATE bay_sessions SET timer_time = 0;");
	db->prepareStatement("WIPE_PUMP_TIME", "UPDATE bay_sessions SET pump_time = 0;");
	db->prepareStatement("WIPE_MANUAL_TIME", "DELETE FROM bay_manual_inserts;");


	Timer* timer = new Timer();
	Carwash* carwash = new Carwash();
	Window* window = new Window();

	carwash->start();

    int gui_update_delay = 4; // loop cycles (1 cycle = ~.05 seconds)
    int gui_update_count = 1;

	while(!stop_program) {
		timer->onLoopStart();
		window->onLoopStart();

		if(gui_update_count >= gui_update_delay) gui_update_count = 1;
		gui_update_count++;

		carwash->run(db, timer, window);

		window->onLoopEnd();
		timer->onLoopEnd();
	}

	window->end();

	carwash->shutdown();

	db->disconnect();

	std::cout << "CWMON ENDED" << std::endl;

	return 0;
}
