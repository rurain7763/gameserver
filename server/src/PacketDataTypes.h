#ifndef PACKETTYPES_H
#define PACKETTYPES_H

enum PacketId {
	Login = 0,
	Message
};

struct LoginData {
	std::string username;
	std::string password;
	MSGPACK_DEFINE(username, password);
};

struct MessageData {
	std::string message;
	MSGPACK_DEFINE(message);
};

#endif