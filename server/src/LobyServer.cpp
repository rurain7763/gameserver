#include "LobyServer.h"
#include "DatabaseServer.h"

#include <iostream>

LobyServer::LobyServer(Config& config, boost::asio::io_context& ioContext, std::shared_ptr<DatabaseServer>& dbServer) 
	: _config(config) 
{
	_tcpServer = std::make_unique<flaw::TcpServer>(ioContext);
	_tcpServer->SetOnSessionStart(std::bind(&LobyServer::OnTcpServerSessionStart, this, std::placeholders::_1));
	_tcpServer->SetOnSessionEnd(std::bind(&LobyServer::OnTcpServerSessionEnd, this, std::placeholders::_1));
	_tcpServer->SetOnPacketReceived(std::bind(&LobyServer::OnTcpServerPacketReceived, this, std::placeholders::_1, std::placeholders::_2));

	_dbServer = dbServer;
}

void LobyServer::Start() {
	_tcpServer->Bind(_config.lobyServerIp, _config.lobyServerPort);
	_tcpServer->StartListen();
	_tcpServer->StartAccept();
}

void LobyServer::Update() {
	while (true) {
		int sessionID;

		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (_disconnectedSessions.empty()) {
				break;
			}

			sessionID = _disconnectedSessions.front();
			_disconnectedSessions.pop();
		}

		std::shared_ptr<User> disconnectedUser;
		if (!TryGetUser(sessionID, disconnectedUser)) {
			continue;
		}

		std::shared_ptr<Room> room;
		if (TryGetRoom(disconnectedUser->joinedRoomSessionID, room)) {
			if (room->ownerSessionID == disconnectedUser->sessionID && room->opponentSessionID != INVALID_SESSION_ID) {
				// disconnected user is owner
				// room owner is disconnected so recognize to opponent
				std::shared_ptr<flaw::Packet> packet = std::make_shared<flaw::Packet>();
				packet->header.senderId = SERVER_ID;
				packet->header.packetId = PacketId::RoomState;

				RoomStateData roomStateData;
				roomStateData.type = RoomStateData::Type::MasterOut;
				roomStateData.userId = disconnectedUser->id;
				roomStateData.userName = disconnectedUser->name;
				packet->SetData(roomStateData);

				std::shared_ptr<User> opponent;
				if (TryGetUser(room->opponentSessionID, opponent)) {
					opponent->joinedRoomSessionID = INVALID_SESSION_ID;
				}

				_tcpServer->Send(room->opponentSessionID, packet);

				_rooms.erase(room->ownerSessionID);
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

				room->opponentSessionID = INVALID_SESSION_ID;

				_tcpServer->Send(room->ownerSessionID, packet);
			}
		}

		RemoveUser(sessionID);
	}

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

void LobyServer::Shutdown() {
	_tcpServer->Shutdown();
}

void LobyServer::OnTcpServerSessionStart(int sessionID) {
	std::cout << "Session start" << sessionID << std::endl;
}

void LobyServer::OnTcpServerSessionEnd(int sessionID) {
	std::cout << "Session end: " << sessionID << std::endl;

	std::lock_guard<std::mutex> lock(_mutex);
	_disconnectedSessions.push(sessionID);
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
	std::lock_guard<std::mutex> lock(_mutex);
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
				if (TryGetUser(sessionID, exist)) {
					loginResultData.result = LoginResultData::Type::AlreadyLoggedIn;
				}
				else {
					loginResultData.result = LoginResultData::Type::Success;
					AddUser(sessionID, userData);
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
	if (!TryGetUser(sessionID, user)) {
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
	if (!TryGetUser(sessionID, user)) {
		return;
	}

	EmptyData emptyData;
	packet->GetData(emptyData);

	GetLobyResultData getLobyResultData;
	packet->header.packetId = PacketId::GetLobyResult;

	std::shared_ptr<User> owner;
	RoomData roomData;
	for (auto& pair : _rooms) {
		if (TryGetUser(pair.second->ownerSessionID, owner)) {
			roomData.title = pair.second->title;
			roomData.ownerId = owner->id;
			roomData.ownerName = owner->name;
			getLobyResultData.rooms.push_back(std::move(roomData));
		}
	}

	packet->SetData(getLobyResultData);

	_tcpServer->Send(sessionID, packet);
}

void LobyServer::HandleCreateRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!TryGetUser(sessionID, user)) {
		return;
	}

	CreateRoomData createRoomData;
	packet->GetData(createRoomData);

	CreateRoomResultData createRoomResultData;

	if (_rooms.find(sessionID) != _rooms.end()) {
		createRoomResultData.result = CreateRoomResultData::Type::AlreadyExist;
	}
	else {
		std::shared_ptr<Room> room = std::make_shared<Room>();
		room->title = std::move(createRoomData.title);
		room->ownerSessionID = user->sessionID;
		room->opponentSessionID = INVALID_SESSION_ID;

		user->joinedRoomSessionID = sessionID;
		_rooms[sessionID] = std::move(room);
		createRoomResultData.result = CreateRoomResultData::Type::Success;
	}

	packet->header.packetId = PacketId::CreateRoomResult;
	packet->SetData(createRoomResultData);

	_tcpServer->Send(sessionID, packet);
}

void LobyServer::HandleDestroyRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (TryGetUser(sessionID, user) == false) {
		return;
	}

	DestroyRoomResultData destroyRoomResultData;
	packet->header.packetId = PacketId::DestroyRoomResult;

	std::shared_ptr<Room> room;
	if (!TryGetRoom(sessionID, room)) {
		destroyRoomResultData.result = DestroyRoomResultData::Type::NotExist;
	}
	else if (room->opponentSessionID != INVALID_SESSION_ID) {
		destroyRoomResultData.result = DestroyRoomResultData::Type::OpponentExist;
	}
	else {
		user->joinedRoomSessionID = INVALID_SESSION_ID;
		_rooms.erase(sessionID);
		destroyRoomResultData.result = DestroyRoomResultData::Type::Success;
	}

	packet->SetData(destroyRoomResultData);

	_tcpServer->Send(sessionID, packet);
}

void LobyServer::HandleJoinRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!TryGetUser(sessionID, user)) {
		return;
	}

	JoinRoomData joinRoomData;
	packet->GetData(joinRoomData);

	JoinRoomResultData joinRoomResultData;
	packet->header.packetId = PacketId::JoinRoomResult;

	std::shared_ptr<User> roomOwner;
	std::shared_ptr<Room> room;

	if (!TryGetUser(joinRoomData.ownerId, roomOwner)) {
		joinRoomResultData.result = JoinRoomResultData::Type::NotValidOwner;
	}
	else if (!TryGetRoom(roomOwner->sessionID, room)) {
		joinRoomResultData.result = JoinRoomResultData::Type::NotExisRoom;
	}
	else if (room->opponentSessionID != INVALID_SESSION_ID) {
		if (room->opponentSessionID == sessionID) {
			joinRoomResultData.result = JoinRoomResultData::Type::AlreadyJoined;
		}
		else {
			joinRoomResultData.result = JoinRoomResultData::Type::Full;
		}
	}
	else {
		user->joinedRoomSessionID = room->ownerSessionID;
		room->opponentSessionID = sessionID;
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
	if (!TryGetUser(sessionID, user)) {
		return;
	}

	LeaveRoomData leaveRoomData;
	packet->GetData(leaveRoomData);

	LeaveRoomResultData leaveRoomResultData;
	packet->header.packetId = PacketId::LeaveRoomResult;

	bool isThisUserOwner = false;
	std::shared_ptr<User> roomOwner;
	std::shared_ptr<Room> room;

	if (!TryGetUser(leaveRoomData.ownerId, roomOwner)) {
		leaveRoomResultData.result = LeaveRoomResultData::Type::NotValidOwner;
	}
	else if (!TryGetRoom(roomOwner->sessionID, room)) {
		leaveRoomResultData.result = LeaveRoomResultData::Type::NotExistRoom;
	}
	else if (room->ownerSessionID == sessionID) {
		if (room->opponentSessionID != INVALID_SESSION_ID) {
			leaveRoomResultData.result = LeaveRoomResultData::Type::OpponentExist;
		}
		else {
			_rooms.erase(roomOwner->sessionID);
			leaveRoomResultData.result = LeaveRoomResultData::Type::Success;
			isThisUserOwner = true;
		}
	}
	else if (room->opponentSessionID != sessionID) {
		leaveRoomResultData.result = LeaveRoomResultData::Type::NotJoined;
	}
	else {
		user->joinedRoomSessionID = INVALID_SESSION_ID;
		room->opponentSessionID = INVALID_SESSION_ID;
		leaveRoomResultData.result = LeaveRoomResultData::Type::Success;
		isThisUserOwner = false;
	}

	packet->SetData(leaveRoomResultData);

	_tcpServer->Send(sessionID, packet);

	if (leaveRoomResultData.result == LeaveRoomResultData::Type::Success && !isThisUserOwner) {
		packet->header.senderId = SERVER_ID;
		packet->header.packetId = PacketId::RoomState;

		RoomStateData roomStateData;
		roomStateData.type = RoomStateData::Type::OpponentOut;
		roomStateData.userId = user->id;
		roomStateData.userName = user->name;

		packet->SetData(roomStateData);

		_tcpServer->Send(roomOwner->sessionID, packet);
	}
}

void LobyServer::HandleGetGetChatServerPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	std::shared_ptr<User> user;
	if (!TryGetUser(sessionID, user)) {
		return;
	}

	GetChatServerResultData getChatServerResultData;
	packet->header.packetId = PacketId::GetChatServerResult;

	// may be make multiple chat servers and random select in future
	getChatServerResultData.ip = _config.chatServerIp;
	getChatServerResultData.port = _config.chatServerPort;

	packet->SetData(getChatServerResultData);

	_tcpServer->Send(sessionID, packet);

	std::cout << "User " << user->id << " requested chat server" << std::endl;
}

bool LobyServer::TryGetUser(const int sessionID, std::shared_ptr<User>& user) {
	auto it = _sessionToLoginUser.find(sessionID);
	if (it == _sessionToLoginUser.end()) {
		return false;
	}
	user = it->second;
	return true;
}

bool LobyServer::TryGetUser(const std::string& id, std::shared_ptr<User>& user) {
	auto it = _idToLoginUser.find(id);
	if (it == _idToLoginUser.end()) {
		return false;
	}
	user = it->second;
	return true;
}

bool LobyServer::TryGetRoom(const int sessionID, std::shared_ptr<Room>& room) {
	auto it = _rooms.find(sessionID);
	if (it == _rooms.end()) {
		return false;
	}
	room = it->second;
	return true;
}

void LobyServer::AddUser(const int sessionID, const UserData& userData) {
	std::shared_ptr<User> user = std::make_shared<User>();
	user->sessionID = sessionID;
	user->id = userData.id;
	user->name = userData.username;
	user->joinedRoomSessionID = INVALID_SESSION_ID;

	_sessionToLoginUser[sessionID] = user;
	_idToLoginUser[userData.id] = user;
}

void LobyServer::RemoveUser(const int sessionID) {
	std::shared_ptr<User> user;
	if (!TryGetUser(sessionID, user)) {
		return;
	}

	_sessionToLoginUser.erase(sessionID);
	_idToLoginUser.erase(user->id);
}