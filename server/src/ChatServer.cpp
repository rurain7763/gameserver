#include "ChatServer.h"
#include "PacketDataTypes.h"

#include <iostream>

ChatServer::ChatServer(Config& config, boost::asio::io_context& ioContext) 
	: _config(config) 
{
	_udpServer = std::make_unique<flaw::UdpServer>(ioContext);
	_udpServer->SetOnPacketReceived(std::bind(&ChatServer::OnPacketReceived, this, std::placeholders::_1, std::placeholders::_2));
}

void ChatServer::Start() {
	_udpServer->Bind(_config.chatServerIp, _config.chatServerPort);
	_udpServer->StartRecv();
}

void ChatServer::Update() {
	while (true) {
		std::function<void()> work;

		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (_packetQueue.empty()) {
				break;
			}

			work = _packetQueue.front();
			_packetQueue.pop();
		}

		work();
	}
}

void ChatServer::Shutdown() {
	_udpServer->Shutdown();
}

void ChatServer::AddToPacketQueue(std::function<void()>&& work) {
	std::lock_guard<std::mutex> lock(_mutex);
	_packetQueue.push(std::move(work));
}

void ChatServer::OnPacketReceived(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet) {
	if (packet->ComparePacketId(PacketId::JoinChatServer)) {
		AddToPacketQueue([this, peer = std::move(peer), packet]() mutable { HandleJoinChatServerPacket(peer, packet); });
	}
	else if (packet->ComparePacketId(PacketId::LeaveChatServer)) {
		AddToPacketQueue([this, peer = std::move(peer), packet]() mutable { HandleLeaveChatServerPacket(peer, packet); });
	}
	else if (packet->ComparePacketId(PacketId::SendChat)) {
		AddToPacketQueue([this, peer = std::move(peer), packet]() mutable { HandleSendChatPacket(peer, packet); });
	}
	else {
		std::cout << "Unknown packet id: " << packet->header.packetId << std::endl;
	}
}

void ChatServer::HandleJoinChatServerPacket(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet) {
	JoinChatServerData joinChatServerData;
	packet->GetData(joinChatServerData);

	JoinChatServerResultData joinChatServerResultData;

	std::shared_ptr<flaw::Peer> exist;
	if (!TryGetPeer(joinChatServerData.userId, exist)) {
		_peers[joinChatServerData.userId] = std::make_shared<flaw::Peer>(std::move(peer));
		joinChatServerResultData.result = JoinChatServerResultData::Success;
		std::cout << "Peer joined: " << joinChatServerData.userId << std::endl;
	}
	else {
		joinChatServerResultData.result = JoinChatServerResultData::AlreadyJoined;
	}

	packet->header.packetId = PacketId::JoinChatServerResult;
	packet->SetData(joinChatServerResultData);

	_udpServer->Send(peer, packet);
}

void ChatServer::HandleLeaveChatServerPacket(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet) {
	LeaveChatServerData leaveChatServerData;
	packet->GetData(leaveChatServerData);

	LeaveChatServerResultData leaveChatServerResultData;

	std::shared_ptr<flaw::Peer> exist;
	if (TryGetPeer(leaveChatServerData.userId, exist)) {
		_peers.erase(leaveChatServerData.userId);
		leaveChatServerResultData.result = LeaveChatServerResultData::Success;
		std::cout << "Peer left: " << leaveChatServerData.userId << std::endl;
	}
	else {
		leaveChatServerResultData.result = LeaveChatServerResultData::NotRegistered;
	}

	packet->header.packetId = PacketId::LeaveChatServerResult;
	packet->SetData(leaveChatServerResultData);

	_udpServer->Send(peer, packet);
}

void ChatServer::HandleSendChatPacket(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet) {
	SendChatData sendChatData;
	packet->GetData(sendChatData);

	std::shared_ptr<flaw::Peer> exist;
	if (TryGetPeer(sendChatData.receiverId, exist)) {
		_udpServer->Send(*exist, packet);
		std::cout << "Chat sent: " << sendChatData.senderId << " -> " << sendChatData.receiverId << std::endl;
	}
	else {
		std::cout << "Receiver not found: " << sendChatData.receiverId << std::endl;
	}
}

bool ChatServer::TryGetPeer(const std::string& userID, std::shared_ptr<flaw::Peer>& peer) {
	auto it = _peers.find(userID);
	if (it == _peers.end()) {
		return false;
	}
	peer = it->second;
	return true;
}