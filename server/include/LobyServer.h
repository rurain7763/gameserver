#ifndef LOBY_SERVER_H
#define LOBY_SERVER_H

#include "Resources.h"
#include "TcpServer.h"
#include "PacketDataTypes.h"

#include <queue>

class DatabaseServer;

class LobyServer {
public:
	LobyServer(Resources& resources, boost::asio::io_context& ioContext, std::shared_ptr<DatabaseServer>& dbServer);

	void Start();
	void Update();
	void Shutdown();

	void OnTcpServerSessionStart(int sessionID);
	void OnTcpServerSessionEnd(int sessionID);
	void OnTcpServerPacketReceived(int sessionID, std::shared_ptr<flaw::Packet> packet);

private:
	void HandleDisconnectedUser();
	void HandlePacketQueue();

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

private:
	static const int SERVER_ID = 0;

	Resources& _resources;

	std::unique_ptr<flaw::TcpServer> _tcpServer;
	std::shared_ptr<DatabaseServer> _dbServer;

	std::mutex _packetQueueMutex;
	std::queue<std::function<void()>> _packetQueue;

	std::mutex _disconnSessionsMutex;
	std::queue<int> _disconnSessions;
};

#endif