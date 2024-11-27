#include "MySqlClient.h"

#include <iostream>

namespace flaw {
	MySqlClient::MySqlClient(boost::asio::io_context& ioContext)
		: _conn(ioContext)
	{
		_observer = std::make_shared<Observer>();
	}

	void MySqlClient::Connect(
		const std::string& host, 
		const std::string& user, 
		const std::string& password, 
		const std::string& database, 
		const unsigned short port) 
	{
		_connParams.server_address.emplace_host_and_port(host, port);
		_connParams.username = user;
		_connParams.password = password;
		_connParams.database = database;

		try {
			_conn.async_connect(_connParams, std::bind(&Observer::OnConnect, _observer, std::placeholders::_1));
		}
		catch (const boost::mysql::error_code& ec) {
			std::cerr << "Error: " << ec.message() << std::endl;
		}
	}

	void MySqlClient::Disconnect() {
		_conn.async_close(_diag, std::bind(&Observer::OnDisconnect, _observer, std::placeholders::_1));
	}

	void MySqlClient::Observer::OnConnect(const boost::mysql::error_code& ec) {
		if (ec) {
			std::cerr << "Error: " << ec.message() << std::endl;
			return;
		}

		if (onConnect) {
			onConnect();
		}
	}

	void MySqlClient::Observer::OnDisconnect(const boost::mysql::error_code& ec) {
		if (ec) {
			std::cerr << "Error: " << ec.message() << std::endl;
			return;
		}

		if (onDisconnect) {
			onDisconnect();
		}
	}
}


