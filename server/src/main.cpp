#include <boost/asio.hpp>
#include <iostream>
#include <queue>
#include <mutex>

#include "TcpServer.h"
#include "UdpServer.h"
#include "Serialization.h"
#include "PacketDataTypes.h"
#include "MySqlClient.h"

#if true
class DatabaseServer {
public:
	DatabaseServer(boost::asio::io_context& ioContex) {
		_mySql = std::make_unique<flaw::MySqlClient>(ioContex);
		_mySql->SetOnConnect(std::bind(&DatabaseServer::OnConnect, this));
		_mySql->SetOnDisconnect(std::bind(&DatabaseServer::OnDisconnect, this));
	}

	void Start() {
		_mySql->Connect("192.168.50.18", "ldh", "Jjang_dong12", "a");
	}

	void Update() {
		// TODO: Implement
	}

	void Shutdown() {
		_mySql->Disconnect();
	}

	void OnConnect() {
		std::cout << "Database connected" << std::endl;
	}

	void OnDisconnect() {
		std::cout << "Database disconnected" << std::endl;
	}

	void GetUser(const std::string& id, std::function<void(bool, UserData&)> callback) {
		_mySql->Execute("SELECT id, nickname, password FROM users WHERE id = {}", [callback = std::move(callback)](flaw::MySqlResult result) {
			bool success = false;
			UserData userData;

			if (!result.Empty()) {
				success = true;
				userData.id = result.Get<std::string>(0, 0);
				userData.username = result.Get<std::string>(0, 1);
				userData.password = result.Get<std::string>(0, 2);
			}
			
			callback(success, userData);
		}, id);
	}

	void IsUserExist(const std::string& id, std::function<void(bool)> callback) {
		_mySql->Execute("SELECT id, password FROM users WHERE id = {}", [callback = std::move(callback)](flaw::MySqlResult result) {
			callback(!result.Empty());
		}, id);
	}

	void SignUp(
		const std::string& id, 
		const std::string& password, 
		const std::string& username, 
		std::function<void(bool)> callback) 
	{
		// TODO: validate check id and password

		_mySql->Execute("INSERT INTO users (id, password, nickname, type) VALUES ({}, {}, {}, {})", [callback = std::move(callback)](flaw::MySqlResult result) {
			callback(true);
		}, id, password, username, "USER");
	}

private:
	std::unique_ptr<flaw::MySqlClient> _mySql;
};

class LobyServer {
private:
	struct User {
		std::string id;
		std::string name;
	};

	struct Room {
		std::string ownerId;
		std::string title;
	};

public:
	LobyServer(boost::asio::io_context& ioContext, std::shared_ptr<DatabaseServer>& dbServer) {
		_tcpServer = std::make_unique<flaw::TcpServer>(ioContext);
		_tcpServer->SetOnSessionStart(std::bind(&LobyServer::OnTcpServerSessionStart, this, std::placeholders::_1));
		_tcpServer->SetOnSessionEnd(std::bind(&LobyServer::OnTcpServerSessionEnd, this, std::placeholders::_1));
		_tcpServer->SetOnPacketReceived(std::bind(&LobyServer::OnTcpServerPacketReceived, this, std::placeholders::_1, std::placeholders::_2));

		_dbServer = dbServer;
	}

	void Start() {
		_tcpServer->Bind("127.0.0.1", 8080);
		_tcpServer->StartListen();
		_tcpServer->StartAccept();
	}

	void Update() {
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

			auto it = _sessionToLoginUser.find(sessionID);

			if (it != _sessionToLoginUser.end()) {
				User& user = GetUser(sessionID);

				_rooms.erase(user.id);
				RemoveUser(sessionID);
			}
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

	void Shutdown() {
		_tcpServer->Shutdown();
	}

	void OnTcpServerSessionStart(int sessionID) {
		std::cout << "Session start" << sessionID << std::endl;
	}

	void OnTcpServerSessionEnd(int sessionID) {
		std::cout << "Session end: " << sessionID << std::endl;

		std::lock_guard<std::mutex> lock(_mutex);
		_disconnectedSessions.push(sessionID);
	}

	void OnTcpServerPacketReceived(int sessionID, std::shared_ptr<flaw::Packet> packet) {
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
		else {
			std::cout << "Unknown packet received" << std::endl;
		}
	}

private:
	void AddToPacketQueue(std::function<void()>&& work) {
		std::lock_guard<std::mutex> lock(_mutex);
		_packetQueue.push(std::move(work));
	}

	void HandleLoginPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
		if (!_tcpServer->IsSessionExist(sessionID)) {
			return;
		}

		LoginData loginData;
		packet->GetData(loginData);

		_dbServer->GetUser(loginData.id, [this, sessionID, packet, password = std::move(loginData.password)](bool success, UserData& userData) {
			LoginResultData loginResultData;

			if (success) {
				if(userData.password != password) {
					loginResultData.result = LoginResultData::Type::PasswordMismatch;
				}
				else {
					loginResultData.result = LoginResultData::Type::Success;
					AddUser(sessionID, userData);
				}
			}
			else {
				loginResultData.result = LoginResultData::Type::NotFound;
			}

			packet->header.packetId = PacketId::LoginResult;
			packet->SetData(loginResultData);

			_tcpServer->Send(sessionID, packet);
		});
	}

	void HandleMessagePacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
		if (!_tcpServer->IsSessionExist(sessionID)) {
			return;
		}

		MessageData messageData;
		packet->GetData(messageData);

		std::cout << "Message received: " << messageData.message << std::endl;
	}

	void HandleSignUpPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
		if (!_tcpServer->IsSessionExist(sessionID)) {
			return;
		}

		SignUpData signUpData;
		packet->GetData(signUpData);

		_dbServer->SignUp(signUpData.id, signUpData.password, signUpData.username, [this, sessionID, packet](bool result) {
			SignUpResultData signUpResultData;

			if (result) {
				signUpResultData.result = true;
			}
			else {
				signUpResultData.result = false;
			}

			packet->header.packetId = PacketId::SignUpResult;
			packet->SetData(signUpResultData);

			_tcpServer->Send(sessionID, packet);
		});
	}

	void HandleGetLobyPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
		if (!_tcpServer->IsSessionExist(sessionID)) {
			return;
		}

		EmptyData emptyData;
		packet->GetData(emptyData);
		
		GetLobyResultData getLobyResultData;
		
		for (auto& pair : _rooms) {
			RoomData roomData;
			roomData.title = pair.second.title;

			User& owner = GetUser(pair.second.ownerId);

			roomData.ownerId = owner.id;
			roomData.ownerName = owner.name;
				
			getLobyResultData.rooms.push_back(std::move(roomData));
		}

		packet->header.packetId = PacketId::GetLobyResult;
		packet->SetData(getLobyResultData);

		_tcpServer->Send(sessionID, packet);
	}

	void HandleCreateRoomPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
		if (!_tcpServer->IsSessionExist(sessionID)) {
			return;
		}

		CreateRoomData createRoomData;
		packet->GetData(createRoomData);

		CreateRoomResultData createRoomResultData;

		User& owner = GetUser(sessionID);

		if (_rooms.find(owner.id) != _rooms.end()) {
			createRoomResultData.result = CreateRoomResultData::Type::AlreadyExist;
		}
		else {
			Room room;
			room.ownerId = owner.id;
			room.title = createRoomData.title;

			_rooms[owner.id] = std::move(room);
			createRoomResultData.result = CreateRoomResultData::Type::Success;
		}

		packet->header.packetId = PacketId::CreateRoomResult;
		packet->SetData(createRoomResultData);

		_tcpServer->Send(sessionID, packet);
	}

	void HandleGetChatListPacket(int sessionID, std::shared_ptr<flaw::Packet> packet) {
		if (!_tcpServer->IsSessionExist(sessionID)) {
			return;
		}

		GetChatListData getChatListData;
		packet->GetData(getChatListData);

		GetChatListResultData getChatListResultData;
	}

	User& GetUser(int sessionID) {
		return _loginUsers[_sessionToLoginUser[sessionID]];
	}

	User& GetUser(const std::string& id) {
		return _loginUsers[_idToLoginUser[id]];
	}

	void AddUser(const int sessionID, const UserData& userData) {
		_sessionToLoginUser[sessionID] = _loginUsers.size();
		_idToLoginUser[userData.id] = _loginUsers.size();
		_loginUsers.push_back(User{ userData.id, userData.username });
	}

	void RemoveUser(const int sessionID) {
		int index = _sessionToLoginUser[sessionID];
		_sessionToLoginUser.erase(sessionID);
		_idToLoginUser.erase(_loginUsers[index].id);
		_loginUsers.erase(_loginUsers.begin() + index);
	}

private:
	std::unique_ptr<flaw::TcpServer> _tcpServer;
	std::shared_ptr<DatabaseServer> _dbServer;

	std::mutex _mutex;
	std::queue<std::function<void()>> _packetQueue;
	std::queue<int> _disconnectedSessions;

	std::vector<User> _loginUsers;
	std::unordered_map<int, int> _sessionToLoginUser;
	std::unordered_map<std::string, int> _idToLoginUser;

	std::unordered_map<std::string, Room> _rooms;
};
#endif

int main() {
	try {
		boost::asio::io_context ioContext;
		boost::asio::io_context::work idleWork(ioContext);
		std::thread contextThread([&ioContext]() { ioContext.run(); });

		auto dbServer = std::make_shared<DatabaseServer>(ioContext);
		auto lobyServer = std::make_shared<LobyServer>(ioContext, dbServer);

		bool running = true;

		lobyServer->Start();
		dbServer->Start();

		std::thread inputThread([&running]() {
			std::string input;
			while (running) {
				std::getline(std::cin, input);
				if (input == "exit") {
					running = false;
				}
			}
		});

		while (running) {
			dbServer->Update();
			lobyServer->Update();
		}

		lobyServer->Shutdown();
		dbServer->Shutdown();

		ioContext.stop();

		contextThread.join();
		inputThread.join();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
