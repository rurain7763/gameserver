#ifndef MYSQL_H
#define MYSQL_H

#include "Global.h"
#include "MySqlResult.h"

#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/connect_params.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/mysql/results.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/with_params.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>

#include <functional>

namespace flaw {
	class FLAW_API MySqlClient {
	private:
		struct Observer : public std::enable_shared_from_this<Observer> {
			void OnConnect(const boost::mysql::error_code& ec);
			void OnDisconnect(const boost::mysql::error_code& ec);

			std::function<void()> onConnect;
			std::function<void()> onDisconnect;
		};

	public:
		MySqlClient(boost::asio::io_context& ioContext);

		void Connect(
			const std::string& host, 
			const std::string& user, 
			const std::string& password, 
			const std::string& database, 
			const unsigned short port = 3306
		);

		void Disconnect();

		template<typename... TArgs>
		void Execute(const std::string& query, std::function<void(MySqlResult)> callback, TArgs&&... args) {
			_conn.async_execute(
				boost::mysql::with_params(query, std::forward<TArgs>(args)...),
				_results,
				_diag,
				[this, callback = std::move(callback)](const boost::mysql::error_code& ec) {
					if (ec) {
						std::cerr << "Error: " << ec.message() << std::endl;
						return;
					}

					MySqlResult result(_results);
					callback(result);
				}
			);
		}

		inline void SetOnConnect(std::function<void()> onConnect) {
			_observer->onConnect = onConnect;
		}

		inline void SetOnDisconnect(std::function<void()> onDisconnect) {
			_observer->onDisconnect = onDisconnect;
		}

	private:
		void OnConnect(const boost::mysql::error_code& ec);
		void OnDisconnect(const boost::mysql::error_code& ec);

	private:
		boost::mysql::connect_params _connParams;
		boost::mysql::any_connection _conn;
		boost::mysql::results _results;
		boost::mysql::diagnostics _diag;

		std::shared_ptr<Observer> _observer;
	};
}

#endif
