#include "LobyServer.h"
#include "DatabaseServer.h"

#include <iostream>

LobyServer::LobyServer(Resources& resources, boost::asio::io_context& ioContext, std::shared_ptr<DatabaseServer>& dbServer) 
	: _resources(resources)
{
	_tcpServer = std::make_unique<flaw::TcpServer>(ioContext);
	_tcpServer->SetOnSessionStart(std::bind(&LobyServer::OnTcpServerSessionStart, this, std::placeholders::_1));
	_tcpServer->SetOnSessionEnd(std::bind(&LobyServer::OnTcpServerSessionEnd, this, std::placeholders::_1));
	_tcpServer->SetOnPacketReceived(std::bind(&LobyServer::OnTcpServerPacketReceived, this, std::placeholders::_1, std::placeholders::_2));

	_dbServer = dbServer;
}

void LobyServer::Start() {
	auto& config = _resources.GetConfig();

	_tcpServer->Bind(config.lobyServerIp, config.lobyServerPort);
	_tcpServer->StartListen();
	_tcpServer->StartAccept();

	std::cout << "Loby server started" << std::endl;
}

void LobyServer::Update() {
	HandleDisconnectedUser();
	HandlePacketQueue();
}

void LobyServer::HandleDisconnectedUser() {
	while (true) {
		int sessionID;

		{
			std::lock_guard<std::mutex> lock(_disconnSessionsMutex);
			if (_disconnSessions.empty()) {
				break;
			}

			sessionID = _disconnSessions.front();
			_disconnSessions.pop();
		}

		std::shared_ptr<User> disconnectedUser;
		if (!_resources.TryGetUser(sessionID, disconnectedUser)) {
			continue;
		}

		std::cout << "User logout: " << disconnectedUser->id << '\n';

		std::shared_ptr<Room> room = disconnectedUser->joinedRoom;
		if (room) {
			if (room->ownerUser == disconnectedUser) {
				// disconnected user is owner
				if (room->opponentUser)
				{
					// room owner is disconnected so recognize to opponent
					std::shared_ptr<flaw::Packet> packet = std::make_shared<flaw::Packet>();
					packet->header.senderId = SERVER_ID;
					packet->header.packetId = PacketId::RoomState;

					RoomStateData roomStateData;
					roomStateData.type = RoomStateData::Type::MasterOut;
					roomStateData.userId = disconnectedUser->id;
					roomStateData.userName = disconnectedUser->name;
					packet->SetData(roomStateData);

					room->opponentUser->joinedRoom = nullptr;

					_tcpServer->Send(room->opponentUser->sessionID, packet);
					std::cout << "Room destroyed because " << disconnectedUser->id << " is logout. so send packet to opponent " << room->opponentUser->id << '\n';
				}

				_resources.RemoveRoom(room->ownerUser->sessionID);
			}
			else {
				// disconnected user is opponent
				// room opponent is disconnected so recognize to owner
				std::shared_ptr<flaw::Packet> packet = std::make_shared<flaw::Packet>();
				packet->header.senderId = SERVER_ID;
				packet->header.packetId = PacketId::RoomState;

				RoomStateData roomStateData;
				roomStateData.type = RoomStateData::Type::OpponentOut;
				roomStateData.userId = disconnectedUser->id;
				roomStateData.userName = disconnectedUser->name;
				packet->SetData(roomStateData);

				room->opponentUser = nullptr;

				_tcpServer->Send(room->ownerUser->sessionID, packet);
				std::cout << "Opponent " << disconnectedUser->id << " is logout. so send packet to owner\n";
			}
		}

		_resources.RemoveUser(sessionID);
		_resources.GetEventBus().EmitEvent<UserLogoutEvent>(disconnectedUser);
	}
}

void LobyServer::HandlePacketQueue() {
	while (true) {
		std::function<void()> work;

		{
			std::lock_guard<std::mutex> lock(_packetQueueMutex);
			if (_packetQueue.empty()) {
				break;
			}

			work = _packetQueue.front();
			_packetQueue.pop();
		}

		work();
	}
}

void LobyServer::Shutdown() {
	_tcpServer->Shutdown();
}

void LobyServer::OnTcpServerSessionStart(int sessionID) {
	// 
}

void LobyServer::OnTcpServerSessionEnd(int sessionID) {
	std::lock_guard<std::mutex> lock(_disconnSessionsMutex);
	_disconnSessions.push(sessionID);
}

void LobyServer::OnTcpServerPacketReceived(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	if (packet->ComparePacketId(PacketId::Login)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleLoginPacket(sessionID, packet); });
	}
	else if (packet->ComparePacketId(PacketId::Message)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleMessagePacket(sessionID, packet); });
	}
	else if (packet->ComparePacketId(PacketId::SignUp)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleSignUpPacket(sessionID, packet); });
	}
	else if (packet->ComparePacketId(PacketId::GetLoby)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleGetLobyPacket(sessionID, packet); });
	}
	else if (packet->ComparePacketId(PacketId::CreateRoom)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleCreateRoomPacket(sessionID, packet); });
	}
	else if (packet->ComparePacketId(PacketId::DestroyRoom)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleDestroyRoomPacket(sessionID, packet); });
	}
	else if (packet->ComparePacketId(PacketId::JoinRoom)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleJoinRoomPacket(sessionID, packet); });
	}
	else if (packet->ComparePacketId(PacketId::LeaveRoom)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleLeaveRoomPacket(sessionID, packet); });
	}
	else if (packet->ComparePacketId(PacketId::GetChatServer)) {
		AddToPacketQueue([this, sessionID, packet]() { HandleGetGetChatServerPacket(sessionID, packet); });
	}	
	else {
		std::cout << "Unknown packet received" << std::endl;
	}
}

void LobyServer::AddToPacketQueue(std::function<void()>&& work) {
	std::lock_guard<std::mutex> lock(_packetQueueMutex);
	_packetQueue.push(std::move(work));
}

void LobyServer::HandleLoginPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	LoginData loginData;
	packet->GetData(loginData);

	_dbServer->GetUser(loginData.id, [this, sessionID, packet, password = std::move(loginData.password)](bool success, UserData& userData) {
		LoginResultData loginResultData;
		packet->header.packetId = PacketId::LoginResult;

		if (success) {
			if (userData.password != password) {
				loginResultData.result = LoginResultData::Type::PasswordMismatch;
			}
			else {
				std::shared_ptr<User> exist;
				if (_resources.TryGetUser(sessionID, exist)) {
					loginResultData.result = LoginResultData::Type::AlreadyLoggedIn;
				}
				else {
					loginResultData.result = LoginResultData::Type::Success;
					_resources.AddUser(sessionID, userData);
					std::cout << "User " << userData.id << " logged in" << std::endl;
				}
			}
		}
		else {
			loginResultData.result = LoginResultData::Type::NotFound;
		}

		packet->SetData(loginResultData);

		_tcpServer->Send(sessionID, packet);
	});
}

void LobyServer::HandleMessagePacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!_resources.TryGetUser(sessionID, user)) {
		return;
	}

	MessageData messageData;
	packet->GetData(messageData);

	std::cout << "Message received: " << messageData.message << std::endl;
}

void LobyServer::HandleSignUpPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	SignUpData signUpData;
	packet->GetData(signUpData);

	_dbServer->SignUp(signUpData.id, signUpData.password, signUpData.username, [this, sessionID, packet](bool result) {
		SignUpResultData signUpResultData;
		packet->header.packetId = PacketId::SignUpResult;

		if (result) {
			signUpResultData.result = true;
		}
		else {
			signUpResultData.result = false;
		}

		packet->SetData(signUpResultData);

		_tcpServer->Send(sessionID, packet);
	});
}

void LobyServer::HandleGetLobyPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!_resources.TryGetUser(sessionID, user)) {
		return;
	}

	EmptyData emptyData;
	packet->GetData(emptyData);

	GetLobyResultData getLobyResultData;
	packet->header.packetId = PacketId::GetLobyResult;

	RoomData roomData;
	for(auto it = _resources.RoomBegin(); it != _resources.RoomEnd(); it++) {
		roomData.title = it->second->title;
		roomData.ownerId = it->second->ownerUser->id;
		roomData.ownerName = it->second->ownerUser->name;
		getLobyResultData.rooms.push_back(std::move(roomData));
	}

	packet->SetData(getLobyResultData);

	_tcpServer->Send(sessionID, packet);
}

void LobyServer::HandleCreateRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!_resources.TryGetUser(sessionID, user)) {
		return;
	}

	CreateRoomData createRoomData;
	packet->GetData(createRoomData);

	CreateRoomResultData createRoomResultData;

	std::shared_ptr<Room> room = user->joinedRoom;
	if (room) {
		createRoomResultData.result = CreateRoomResultData::Type::AlreadyExist;
	}
	else {
		RoomData roomData;
		roomData.title = std::move(createRoomData.title);
		roomData.ownerId = user->id;
		roomData.ownerName = user->name;
		
		user->joinedRoom = _resources.AddRoom(sessionID, user, roomData);

		createRoomResultData.result = CreateRoomResultData::Type::Success;
	}

	packet->header.packetId = PacketId::CreateRoomResult;
	packet->SetData(createRoomResultData);

	_tcpServer->Send(sessionID, packet);
}

void LobyServer::HandleDestroyRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (_resources.TryGetUser(sessionID, user) == false) {
		return;
	}

	DestroyRoomResultData destroyRoomResultData;
	packet->header.packetId = PacketId::DestroyRoomResult;

	std::shared_ptr<Room> room = user->joinedRoom;
	if (!room) {
		destroyRoomResultData.result = DestroyRoomResultData::Type::NotExist;
	}
	else if(room->ownerUser != user) {
		destroyRoomResultData.result = DestroyRoomResultData::Type::NotOwner;
	}
	else if (room->opponentUser) {
		destroyRoomResultData.result = DestroyRoomResultData::Type::OpponentExist;
	}
	else {
		user->joinedRoom = nullptr;

		_resources.RemoveRoom(sessionID);
		destroyRoomResultData.result = DestroyRoomResultData::Type::Success;
	}

	packet->SetData(destroyRoomResultData);

	_tcpServer->Send(sessionID, packet);
}

void LobyServer::HandleJoinRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!_resources.TryGetUser(sessionID, user)) {
		return;
	}

	JoinRoomData joinRoomData;
	packet->GetData(joinRoomData);

	JoinRoomResultData joinRoomResultData;
	packet->header.packetId = PacketId::JoinRoomResult;

	std::shared_ptr<User> roomOwner;
	std::shared_ptr<Room> room;

	if (!_resources.TryGetUser(joinRoomData.ownerId, roomOwner)) {
		joinRoomResultData.result = JoinRoomResultData::Type::NotValidOwner;
	}
	else if (!_resources.TryGetRoom(roomOwner->sessionID, room)) {
		joinRoomResultData.result = JoinRoomResultData::Type::NotExisRoom;
	}
	else if (room->opponentUser) {
		if (room->opponentUser == user) {
			joinRoomResultData.result = JoinRoomResultData::Type::AlreadyJoined;
		}
		else {
			joinRoomResultData.result = JoinRoomResultData::Type::Full;
		}
	}
	else {
		user->joinedRoom = room;
		room->opponentUser = user;
		joinRoomResultData.result = JoinRoomResultData::Type::Success;
	}

	packet->SetData(joinRoomResultData);

	_tcpServer->Send(sessionID, packet);

	if (joinRoomResultData.result == JoinRoomResultData::Type::Success) {
		packet->header.senderId = SERVER_ID;
		packet->header.packetId = PacketId::RoomState;

		RoomStateData roomStateData;
		roomStateData.type = RoomStateData::Type::OpponentJoined;
		roomStateData.userId = user->id;
		roomStateData.userName = user->name;
		packet->SetData(roomStateData);

		_tcpServer->Send(roomOwner->sessionID, packet);
	}
}

void LobyServer::HandleLeaveRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!_resources.TryGetUser(sessionID, user)) {
		return;
	}

	LeaveRoomData leaveRoomData;
	packet->GetData(leaveRoomData);

	LeaveRoomResultData leaveRoomResultData;
	packet->header.packetId = PacketId::LeaveRoomResult;

	std::shared_ptr<Room> room = user->joinedRoom;

	if (!room) {
		leaveRoomResultData.result = LeaveRoomResultData::Type::NotExistRoom;
	}
	else if (room->ownerUser->id != leaveRoomData.ownerId) {
		leaveRoomResultData.result = LeaveRoomResultData::Type::NotValidOwner;
	}
	else if (room->opponentUser != user) {
		leaveRoomResultData.result = LeaveRoomResultData::Type::NotJoined;
	}
	else if (room->ownerUser == user) {
		if (room->opponentUser) {
			leaveRoomResultData.result = LeaveRoomResultData::Type::OpponentExist;
		}
		else {
			room->ownerUser = nullptr;
			_resources.RemoveRoom(user->sessionID);
			user->joinedRoom = nullptr;
			leaveRoomResultData.result = LeaveRoomResultData::Type::Success;
		}
	}
	else {
		room->opponentUser = nullptr;
		user->joinedRoom = nullptr;
		leaveRoomResultData.result = LeaveRoomResultData::Type::Success;
	}

	packet->SetData(leaveRoomResultData);
	_tcpServer->Send(sessionID, packet);

	if (leaveRoomResultData.result == LeaveRoomResultData::Type::Success && room->ownerUser) {
		packet->header.senderId = SERVER_ID;
		packet->header.packetId = PacketId::RoomState;

		RoomStateData roomStateData;
		roomStateData.type = RoomStateData::Type::OpponentOut;
		roomStateData.userId = user->id;
		roomStateData.userName = user->name;

		packet->SetData(roomStateData);

		_tcpServer->Send(room->ownerUser->sessionID, packet);
	}
}

void LobyServer::HandleGetGetChatServerPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!_resources.TryGetUser(sessionID, user)) {
		return;
	}

	GetChatServerResultData getChatServerResultData;
	packet->header.packetId = PacketId::GetChatServerResult;

	// may be make multiple chat servers and random select in future

	auto& config = _resources.GetConfig();
	getChatServerResultData.ip = config.dns;
	getChatServerResultData.port = config.chatServerPort;

	packet->SetData(getChatServerResultData);

	_tcpServer->Send(sessionID, packet);

	std::cout << "User " << user->id << " requested chat server" << std::endl;
}

