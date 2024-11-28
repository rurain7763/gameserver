#ifndef PACKET_DATA_TYPES_H
#define PACKET_DATA_TYPES_H

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