#ifndef DATABASE_SERVER_H
#define DATABASE_SERVER_H

#include "Resources.h"
#include "MySqlClient.h"
#include "PacketDataTypes.h"

class DatabaseServer {
public:
	DatabaseServer(Resources& resources, boost::asio::io_context& ioContex);

	void Start();
	void Update();
	void Shutdown();

	void OnConnect();
	void OnDisconnect();

	void GetUser(const std::string& id, std::function<void(bool, UserData&)> callback);

	void IsUserExist(const std::string& id, std::function<void(bool)> callback);

	void SignUp(
		const std::string& id,
		const std::string& password,
		const std::string& username,
		std::function<void(bool)> callback);

private:
	Resources& _resources;

	std::unique_ptr<flaw::MySqlClient> _mySql;
};

#endif