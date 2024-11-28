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
	DestroyRoomResult,
	JoinRoom,
	JoinRoomResult,
	LeaveRoom,
	LeaveRoomResult,
	RoomState,
	SendChat,
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
		OpponentExist
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

struct ChatData {
	std::string userId;
	std::string username;
	std::string message;

	MSGPACK_DEFINE(userId, username, message);
};

struct SendChatData {
	std::string senderId;
	std::string receiverId;
	std::string message;

	MSGPACK_DEFINE(message);
};

struct EmptyData {
	MSGPACK_DEFINE();
};

#endif