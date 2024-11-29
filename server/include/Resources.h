#ifndef RESOURCES_H
#define RESOURCES_H

#include "PacketDataTypes.h"
#include "Config.h"
#include "EventBus.h"

#include <unordered_map>
#include <memory>
#include <string>

#define INVALID_SESSION_ID -1

struct Room;
struct User;

struct Room {
	std::string title;
	std::shared_ptr<User> ownerUser;
	std::shared_ptr<User> opponentUser;
};

struct User {
	int sessionID;
	std::string id;
	std::string name;
	std::shared_ptr<Room> joinedRoom;
};

struct UserLogoutEvent : public Event {
	std::shared_ptr<User> user;

	UserLogoutEvent(std::shared_ptr<User> user) : user(user) {}
};

class Resources {
public:
	Resources();
	~Resources() = default;

	void LoadConfig(const std::string& configPath);

	bool TryGetUser(const int sessionID, std::shared_ptr<User>& user);
	bool TryGetUser(const std::string& id, std::shared_ptr<User>& user);
	bool TryGetRoom(const int sessionID, std::shared_ptr<Room>& room);

	void AddUser(const int sessionID, const UserData& userData);
	void RemoveUser(const int sessionID);

	std::shared_ptr<Room> AddRoom(const int sessionID, std::shared_ptr<User> owner, RoomData& roomData);
	void RemoveRoom(const int sessionID);

	inline std::unordered_map<int, std::shared_ptr<Room>>::const_iterator RoomBegin() { return _rooms.begin(); }
	inline std::unordered_map<int, std::shared_ptr<Room>>::const_iterator RoomEnd() { return _rooms.end(); }

	inline const Config& GetConfig() const { return _config; }
	inline EventBus& GetEventBus() { return *_eventBus; }

private:
	Config _config;

	std::unique_ptr<EventBus> _eventBus;

	std::unordered_map<int, std::shared_ptr<User>> _sessionToLoginUser;
	std::unordered_map<std::string, std::shared_ptr<User>> _idToLoginUser;

	std::unordered_map<int, std::shared_ptr<Room>> _rooms;
};

#endif