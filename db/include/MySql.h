#ifndef MYSQL_H
#define MYSQL_H

#include "Global.h"

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>

namespace flaw {
	class FLAW_API MySql {
	public:
		MySql() = default;

		void Connect(const std::string& host, const std::string& user, const std::string& password);
		void Disconnect();

		void SetSchema(const std::string& schema);

		template<typename... Args>
		void Select(const std::string& columns, const std::string& table, std::function<void(Args...)> callback) {
			std::string query = "SELECT " + columns + " FROM " + table;
			std::unique_ptr<sql::PreparedStatement> pstmt(_connection->prepareStatement(query));
			std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

			while (result->next()) {
				CallCallback(result, callback, std::index_sequence_for<Args...>{});
			}
		}

	private:
		template<typename... Args, std::size_t... Is>
		void CallCallback(const std::unique_ptr<sql::ResultSet>& result, std::function<void(Args...)>& callback, std::index_sequence<Is...>) {
			callback(GetValue<Args>(result, Is + 1)...);
		}

		template<typename T>
		T GetValue(const std::unique_ptr<sql::ResultSet>& result, size_t columnIndex);

		template<>
		double MySql::GetValue<double>(const std::unique_ptr<sql::ResultSet>& result, size_t columnIndex) {
			return result->getDouble(columnIndex);
		}

		template<>
		int MySql::GetValue<int>(const std::unique_ptr<sql::ResultSet>& result, size_t columnIndex) {
			return result->getInt(columnIndex);
		}

		template<>
		std::string MySql::GetValue<std::string>(const std::unique_ptr<sql::ResultSet>& result, size_t columnIndex) {
			return result->getString(columnIndex);
		}

	private:
		sql::Driver* _driver;
		sql::Connection* _connection;
	};
}

#endif
