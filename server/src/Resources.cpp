#include "Resources.h"

#include <fstream>

Resources::Resources() {
	_eventBus = std::make_unique<EventBus>();
}

void Resources::LoadConfig(const std::string& configPath) {
	std::ifstream configFile(configPath, std::ios::in);
	if (!configFile.is_open()) {
		throw std::runtime_error("Failed to open config file");
	}

	configFile >> _config.dns;
	configFile >> _config.lobyServerIp;
	configFile >> _config.lobyServerPort;
	configFile >> _config.chatServerIp;
	configFile >> _config.chatServerPort;
	configFile >> _config.mysqlIp;
	configFile >> _config.mysqlUser;
	configFile >> _config.mysqlPassword;
	configFile >> _config.mysqlDatabase;
}

bool Resources::TryGetUser(const int sessionID, std::shared_ptr<User>& user) {
	auto it = _sessionToLoginUser.find(sessionID);
	if (it == _sessionToLoginUser.end()) {
		return false;
	}
	user = it->second;
	return true;
}

bool Resources::TryGetUser(const std::string& id, std::shared_ptr<User>& user) {
	auto it = _idToLoginUser.find(id);
	if (it == _idToLoginUser.end()) {
		return false;
	}
	user = it->second;
	return true;
}

bool Resources::TryGetRoom(const int sessionID, std::shared_ptr<Room>& room) {
	auto it = _rooms.find(sessionID);
	if (it == _rooms.end()) {
		return false;
	}
	room = it->second;
	return true;
}

void Resources::AddUser(const int sessionID, const UserData& userData) {
	std::shared_ptr<User> user = std::make_shared<User>();
	user->sessionID = sessionID;
	user->id = userData.id;
	user->name = userData.username;
	user->joinedRoom = nullptr;

	_sessionToLoginUser[sessionID] = user;
	_idToLoginUser[userData.id] = user;
}

void Resources::RemoveUser(const int sessionID) {
	std::shared_ptr<User> user;
	if (!TryGetUser(sessionID, user)) {
		return;
	}

	_sessionToLoginUser.erase(sessionID);
	_idToLoginUser.erase(user->id);
}

std::shared_ptr<Room> Resources::AddRoom(const int sessionID, std::shared_ptr<User> owner, RoomData& roomData) {
	std::shared_ptr<Room> room = std::make_shared<Room>();
	room->title = std::move(roomData.title);
	room->ownerUser = owner;
	room->opponentUser = nullptr;
	_rooms[sessionID] = room;
	return room;
}

void Resources::RemoveRoom(const int sessionID) {
	_rooms.erase(sessionID);
}