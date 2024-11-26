#include "MySql.h"

#include <iostream>

namespace flaw {
	void MySql::Connect(const std::string& host, const std::string& user, const std::string& password) {
		try {
			if (_driver == nullptr) {
				_driver = get_driver_instance();
			}

			_connection = _driver->connect(host, user, password);
		}
		catch (sql::SQLException& e) {
			std::cerr << "Error connecting to MySQL: " << e.what() << std::endl;
		}
	}

	void MySql::Disconnect() {
		delete _connection;
	}

	void MySql::SetSchema(const std::string& schema) {
		_connection->setSchema(schema);
	}
}


