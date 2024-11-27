#ifndef PACKETTYPES_H
#define PACKETTYPES_H

enum PacketId : short {
	Login = 0,
	Message,
	LoginResult,
	SignUp,
	SignUpResult,
	GetLoby,
	GetLobyResult,
	CreateRoom,
	CreateRoomResult,
	DestroyRoom,
};

struct LoginData {
	std::string id;
	std::string password;

	MSGPACK_DEFINE(id, password);
};

struct MessageData {
	std::string message;

	MSGPACK_DEFINE(message);
};

struct LoginResultData {
	enum Type : char {
		Success = 0,
		NotFound,
		PasswordMismatch
	};

	Type result;

	MSGPACK_DEFINE(result);
};

MSGPACK_ADD_ENUM(LoginResultData::Type);

struct SignUpData {
	std::string id;
	std::string password;
	std::string username;

	MSGPACK_DEFINE(id, password, username);
};

struct SignUpResultData {
	bool result;

	MSGPACK_DEFINE(result);
};

struct UserData {
	std::string id;
	std::string username;
	std::string password;

	MSGPACK_DEFINE(id, username, password);
};

struct RoomData {
	std::string ownerId;
	std::string ownerName;
	std::string title;

	MSGPACK_DEFINE(ownerId, ownerName, title);
};

struct GetLobyResultData {
	std::vector<RoomData> rooms;

	MSGPACK_DEFINE(rooms);
};

struct CreateRoomData {
	std::string id;
	std::string title;

	MSGPACK_DEFINE(id, title);
};

struct CreateRoomResultData {
	enum Type : char {
		Success = 0,
		AlreadyExist
	};

	Type result;

	MSGPACK_DEFINE(result);
};

MSGPACK_ADD_ENUM(CreateRoomResultData::Type);

struct DestroyRoomData {
	std::string id;

	MSGPACK_DEFINE(id);
};

struct ChatData
{
	std::string userId;
	std::string username;
	std::string message;

	MSGPACK_DEFINE(userId, username, message);
};

struct EmptyData {
	MSGPACK_DEFINE();
};

#endif