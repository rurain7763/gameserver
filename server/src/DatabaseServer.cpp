#include "DatabaseServer.h"

#include <iostream>

DatabaseServer::DatabaseServer(Resources& resources, boost::asio::io_context& ioContex) 
	: _resources(resources)
{
	_mySql = std::make_unique<flaw::MySqlClient>(ioContex);
	_mySql->SetOnConnect(std::bind(&DatabaseServer::OnConnect, this));
	_mySql->SetOnDisconnect(std::bind(&DatabaseServer::OnDisconnect, this));
}

void DatabaseServer::Start() {
	auto& config = _resources.GetConfig();
	_mySql->Connect(config.mysqlIp, config.mysqlUser, config.mysqlPassword, config.mysqlDatabase);
}

void DatabaseServer::Update() {
	//
}

void DatabaseServer::Shutdown() {
	_mySql->Disconnect();
}

void DatabaseServer::OnConnect() {
	std::cout << "Database connected" << std::endl;
}

void DatabaseServer::OnDisconnect() {
	std::cout << "Database disconnected" << std::endl;
}

void DatabaseServer::GetUser(const std::string& id, std::function<void(bool, UserData&)> callback) {
	_mySql->Execute("SELECT id, nickname, password FROM users WHERE id = {}", [callback = std::move(callback)](flaw::MySqlResult result) {
		bool success = false;
		UserData userData;

		if (!result.Empty()) {
			success = true;
			userData.id = result.Get<std::string>(0, 0);
			userData.username = result.Get<std::string>(0, 1);
			userData.password = result.Get<std::string>(0, 2);
		}

		callback(success, userData);
	}, id);
}

void DatabaseServer::IsUserExist(const std::string& id, std::function<void(bool)> callback) {
	_mySql->Execute("SELECT id, password FROM users WHERE id = {}", [callback = std::move(callback)](flaw::MySqlResult result) {
		callback(!result.Empty());
	}, id);
}

void DatabaseServer::SignUp(
	const std::string& id,
	const std::string& password,
	const std::string& username,
	std::function<void(bool)> callback)
{
	// TODO: validate check id and password

	_mySql->Execute("INSERT INTO users (id, password, nickname, type) VALUES ({}, {}, {}, {})", [callback = std::move(callback)](flaw::MySqlResult result) {
		callback(true);
		}, id, password, username, "USER");
}