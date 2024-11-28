#ifndef CHATSERVER_H
#define CHATSERVER_H

#include "Config.h"
#include "UdpServer.h"

#include <memory>
#include <queue>
#include <mutex>
#include <functional>

class ChatServer {
public:
	ChatServer(Config& config, boost::asio::io_context& ioContext);

	void Start();
	void Update();
	void Shutdown();

	void OnPacketReceived(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet);

private:
	void AddToPacketQueue(std::function<void()>&& work);

	void HandleJoinChatServerPacket(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet);
	void HandleLeaveChatServerPacket(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet);
	void HandleSendChatPacket(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet);

	bool TryGetPeer(const std::string& userID, std::shared_ptr<flaw::Peer>& peer);

private:
	Config&	_config;

	std::unique_ptr<flaw::UdpServer> _udpServer;

	std::mutex _mutex;
	std::queue<std::function<void()>> _packetQueue;

	std::map<std::string, std::shared_ptr<flaw::Peer>> _peers;
};

#endif