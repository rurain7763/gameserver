#include "ChatServer.h"
#include "PacketDataTypes.h"

#include <iostream>

ChatServer::ChatServer(Resources& resources, boost::asio::io_context& ioContext) 
	: _resources(resources)
{
	_udpServer = std::make_unique<flaw::UdpServer>(ioContext);
	_udpServer->SetOnPacketReceived(std::bind(&ChatServer::OnPacketReceived, this, std::placeholders::_1, std::placeholders::_2));
}

void ChatServer::Start() {
	auto& config = _resources.GetConfig();

	_resources.GetEventBus().Subscribe<ChatServer, UserLogoutEvent>(this, &ChatServer::OnUserLogout);

	_udpServer->Bind(config.chatServerIp, config.chatServerPort);
	_udpServer->StartRecv();

	std::cout << "Chat server started" << std::endl;
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

void ChatServer::OnUserLogout(UserLogoutEvent& evn) {
	std::shared_ptr<flaw::Peer> peer;
	if (TryGetPeer(evn.user->id, peer)) {
		_peers.erase(evn.user->id);
		std::cout << "[Chat server] Peer left: " << evn.user->id << std::endl;
	}
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
		exist = _peers[joinChatServerData.userId];
		std::cout << "Peer joined: " << joinChatServerData.userId << std::endl;
	}
	else {
		joinChatServerResultData.result = JoinChatServerResultData::AlreadyJoined;
	}

	packet->header.packetId = PacketId::JoinChatServerResult;
	packet->SetData(joinChatServerResultData);

	_udpServer->Send(*exist, packet);
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
		leaveChatServerResultData.result = LeaveChatServerResultData::NotJoined;
	}

	packet->header.packetId = PacketId::LeaveChatServerResult;
	packet->SetData(leaveChatServerResultData);

	_udpServer->Send(*exist, packet);
}

void ChatServer::HandleSendChatPacket(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet) {
	SendChatData sendChatData;
	packet->GetData(sendChatData);

	std::shared_ptr<User> user;
	if (!_resources.TryGetUser(sendChatData.senderId, user)) {
		return;
	}

	std::shared_ptr<flaw::Peer> exist;
	if (TryGetPeer(sendChatData.receiverId, exist)) {
		RecvChatData recvChatData;
		recvChatData.userId = std::move(sendChatData.senderId);
		recvChatData.username = user->name;
		recvChatData.message = std::move(sendChatData.message);
		
		packet->header.senderId = SERVER_ID;
		packet->header.packetId = PacketId::RecvChat;
		packet->SetData(recvChatData);

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