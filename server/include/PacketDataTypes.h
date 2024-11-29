#ifndef PACKETTYPES_H
#define PACKETTYPES_H

#include "Serialization.h"

// packet id must be 0 ~ SHRT_MAX
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
	DestroyRoomResult,
	JoinRoom,
	JoinRoomResult,
	LeaveRoom,
	LeaveRoomResult,
	RoomState,
	GetChatServer,
	GetChatServerResult,
	JoinChatServer,
	JoinChatServerResult,
	LeaveChatServer,
	LeaveChatServerResult,
	SendChat,
	RecvChat,
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
		PasswordMismatch,
		AlreadyLoggedIn
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
		AlreadyExist,
	};

	Type result;

	MSGPACK_DEFINE(result);
};

MSGPACK_ADD_ENUM(CreateRoomResultData::Type);

struct DestroyRoomData {
	std::string id;

	MSGPACK_DEFINE(id);
};

struct DestroyRoomResultData {
	enum Type : char {
		Success = 0,
		NotExist,
		OpponentExist,
		NotOwner
	};

	Type result;

	MSGPACK_DEFINE(result);
};

MSGPACK_ADD_ENUM(DestroyRoomResultData::Type);

struct JoinRoomData {
	std::string ownerId;

	MSGPACK_DEFINE(ownerId);
};

struct JoinRoomResultData {
	enum Type : char {
		Success = 0,
		Full,
		NotExisRoom,
		NotValidOwner,
		AlreadyJoined,
	};

	Type result;

	MSGPACK_DEFINE(result);
};

MSGPACK_ADD_ENUM(JoinRoomResultData::Type);

struct LeaveRoomData {
	std::string ownerId;

	MSGPACK_DEFINE(ownerId);
};

struct LeaveRoomResultData {
	enum Type : char {
		Success = 0,
		NotExistRoom,
		NotValidOwner,
		NotJoined,
		OpponentExist
	};

	Type result;

	MSGPACK_DEFINE(result);
};

MSGPACK_ADD_ENUM(LeaveRoomResultData::Type);

struct RoomStateData {
	enum Type : char {
		MasterOut,
		OpponentOut,
		OpponentJoined
	};

	Type type;
	std::string userId;
	std::string userName;

	MSGPACK_DEFINE(type, userId, userName);
};

MSGPACK_ADD_ENUM(RoomStateData::Type);

struct GetChatServerData {
	std::string userId;

	MSGPACK_DEFINE(userId);
};

struct GetChatServerResultData {
	std::string ip;
	short port;

	MSGPACK_DEFINE(ip, port);
};

struct JoinChatServerData {
	std::string userId;

	MSGPACK_DEFINE(userId);
};

struct JoinChatServerResultData {
	enum Type : char {
		Success = 0,
		AlreadyJoined,
	};

	Type result;

	MSGPACK_DEFINE(result);
};

MSGPACK_ADD_ENUM(JoinChatServerResultData::Type);

struct LeaveChatServerData {
	std::string userId;

	MSGPACK_DEFINE(userId);
};

struct LeaveChatServerResultData {
	enum Type : char {
		Success = 0,
		NotJoined,
	};

	Type result;

	MSGPACK_DEFINE(result);
};

MSGPACK_ADD_ENUM(LeaveChatServerResultData::Type);

struct SendChatData {
	std::string senderId;
	std::string receiverId;
	std::string message;

	MSGPACK_DEFINE(senderId, receiverId, message);
};

struct RecvChatData {
	std::string userId;
	std::string username;
	std::string message;

	MSGPACK_DEFINE(userId, username, message);
};

struct EmptyData {
	MSGPACK_DEFINE();
};

#endif