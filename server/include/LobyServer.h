#ifndef LOBY_SERVER_H
#define LOBY_SERVER_H

#include "Config.h"
#include "TcpServer.h"
#include "PacketDataTypes.h"

#include <queue>

class DatabaseServer;

class LobyServer {
private:
	struct User {
		int sessionID;
		std::string id;
		std::string name;
		int joinedRoomSessionID;
	};

	struct Room {
		std::string title;
		int ownerSessionID;
		int opponentSessionID;
	};

public:
	LobyServer(Config& config, boost::asio::io_context& ioContext, std::shared_ptr<DatabaseServer>& dbServer);

	void Start();
	void Update();
	void Shutdown();

	void OnTcpServerSessionStart(int sessionID);
	void OnTcpServerSessionEnd(int sessionID);
	void OnTcpServerPacketReceived(int sessionID, std::shared_ptr<flaw::Packet> packet);

private:
	void AddToPacketQueue(std::function<void()>&& work);

	void HandleLoginPacket(int sessionID, std::shared_ptr<flaw::Packet> packet);
	void HandleMessagePacket(int sessionID, std::shared_ptr<flaw::Packet> packet);
	void HandleSignUpPacket(int sessionID, std::shared_ptr<flaw::Packet> packet);
	void HandleGetLobyPacket(int sessionID, std::shared_ptr<flaw::Packet> packet);
	void HandleCreateRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet);
	void HandleDestroyRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet);
	void HandleJoinRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet);
	void HandleLeaveRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet);
	void HandleGetGetChatServerPacket(int sessionID, std::shared_ptr<flaw::Packet> packet);

	bool TryGetUser(const int sessionID, std::shared_ptr<User>& user);
	bool TryGetUser(const std::string& id, std::shared_ptr<User>& user);
	bool TryGetRoom(const int sessionID, std::shared_ptr<Room>& room);

	void AddUser(const int sessionID, const UserData& userData);
	void RemoveUser(const int sessionID);

private:
	static const int SERVER_ID = 0;
	static const int INVALID_SESSION_ID = -1;

	Config& _config;

	std::unique_ptr<flaw::TcpServer> _tcpServer;
	std::shared_ptr<DatabaseServer> _dbServer;

	std::mutex _mutex;
	std::queue<std::function<void()>> _packetQueue;
	std::queue<int> _disconnectedSessions;

	std::unordered_map<int, std::shared_ptr<User>> _sessionToLoginUser;
	std::unordered_map<std::string, std::shared_ptr<User>> _idToLoginUser;

	std::unordered_map<int, std::shared_ptr<Room>> _rooms;
};

#endif