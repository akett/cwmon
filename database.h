#ifndef _DATABASE_H_INCLUDED_
#define _DATABASE_H_INCLUDED_

#include <pqxx/pqxx>
#include <iostream>
#include <signal.h>

class Database {
public:
	pqxx::connection *connection;
	pqxx::nontransaction *work;
	bool has_connection = false;

	void connect() {
		if(has_connection) return;
		try
		{
			this->connection = new pqxx::connection("dbname=carwash user=washman password=cotton");
			this->work = new pqxx::nontransaction(*this->connection);
			has_connection = true;
			std::cout << "DATABASE CONNECTED" << std::endl;
		}
		catch (const pqxx::sql_error &e)
		{
			std::cerr << "SQL error: " << e.what() << std::endl;
			std::cerr << "Query was: " << e.query() << std::endl;
			raise(SIGINT);
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
			raise(SIGINT);
		}
	}

	void disconnect() {
		if(!has_connection) return;
		this->connection->disconnect();
		std::cout << "DATABASE DISCONNECTED" << std::endl;
	}

	void prepareStatement(const char* statementName, const char* query)
	{
		try {
			this->connection->prepare(statementName, query);
		}
		catch (const pqxx::sql_error &e)
		{
			std::cerr << "SQL error: " << e.what() << std::endl;
			std::cerr << "Query was: " << e.query() << std::endl;
			raise(SIGINT);
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
			raise(SIGINT);
		}
	}

	pqxx::prepare::invocation beginPrepared(const char *statementName)
	{
		return this->work->prepared(statementName);
	}

	pqxx::result execPrepared(pqxx::prepare::invocation prepared)
	{
		pqxx::result R;

		try {
			R = prepared.exec();
		}
		catch (const pqxx::sql_error &e)
		{
			std::cerr << "SQL error: " << e.what() << std::endl;
			std::cerr << "Query was: " << e.query() << std::endl;
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}

		return R;
	}

	pqxx::result execQuery(std::string query)
	{
		pqxx::result R;

		try {
			R = this->work->exec(query);
		}
		catch (const pqxx::sql_error &e)
		{
			std::cerr << "SQL error: " << e.what() << std::endl;
			std::cerr << "Query was: " << e.query() << std::endl;
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}

		return R;
	}
};

#endif