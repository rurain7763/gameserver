#include "TcpServer.h"

#include <iostream>

namespace flaw {
	TcpServer::TcpServer(boost::asio::io_context& ioContext) 
		: _acceptor(ioContext), _sessionIDCounter(0), _packetPool(MAX_PACKET_POOL_SIZE)
	{
	}

	void TcpServer::Bind(const std::string ip, const short port) {
		boost::asio::ip::tcp::endpoint endpoint;

		if (ip.find(":") != std::string::npos) {
			auto address = boost::asio::ip::make_address_v6(ip);
		 	endpoint = boost::asio::ip::tcp::endpoint(address, port);
		}
		else if (ip.find(".") != std::string::npos) {
			auto address = boost::asio::ip::make_address_v4(ip);
			endpoint = boost::asio::ip::tcp::endpoint(address, port);
		}
		else {
			throw std::invalid_argument("Invalid IP address");
		}

		_acceptor.open(endpoint.protocol());
		_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		_acceptor.set_option(boost::asio::ip::tcp::no_delay(true));
		_acceptor.set_option(boost::asio::socket_base::keep_alive(true));
		_acceptor.bind(endpoint);
	}

	void TcpServer::StartListen() {
		_acceptor.listen();
	}

	void TcpServer::StartAccept() {
		_acceptor.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
			if (!ec) {
				int sessionID = -1;
				
				{
					std::lock_guard<std::mutex> lock(_sessionMutex);
					sessionID = GetAvailableSessionID();
				}

				auto session = Session::Create(sessionID, std::move(socket));
				session->SetOnSessionEnd(std::bind(&TcpServer::OnSessionEnd, this, std::placeholders::_1));
				session->SetOnPacketReceived(std::bind(&TcpServer::OnPacketReceived, this, std::placeholders::_1, std::placeholders::_2));

				{
					std::lock_guard<std::mutex> lock(_sessionMutex);
					_sessions[sessionID] = session;
				}

				session->StartRecv();

				_cbOnSessionStart(sessionID);
			}

			StartAccept();
		});
	}

	void TcpServer::Shutdown() {
		_acceptor.cancel();

		{
			std::lock_guard<std::mutex> lock(_sessionMutex);
			for (auto& pair : _sessions) {
				_cbOnSessionEnd(pair.first);
				pair.second->Close();
			}
			_sessions.clear();
		}

		_acceptor.close();
	}

	void TcpServer::Send(int sessionID, std::shared_ptr<Packet> packet) {
		std::shared_ptr<Session> session = nullptr;

		{
			std::lock_guard<std::mutex> lock(_sessionMutex);
			auto it = _sessions.find(sessionID);
			if (it == _sessions.end()) {
				return;
			}

			session = it->second;
		}

		session->StartSend(packet);
	}

	void TcpServer::Send(int sessionID, const std::vector<std::shared_ptr<Packet>>& packets) {
		std::shared_ptr<Session> session = nullptr;

		{
			std::lock_guard<std::mutex> lock(_sessionMutex);
			auto it = _sessions.find(sessionID);
			if (it == _sessions.end()) {
				return;
			}

			session = it->second;
		}

		session->StartSend(packets);
	}

	bool TcpServer::IsSessionExist(int sessionID) {
		std::lock_guard<std::mutex> lock(_sessionMutex);
		return _sessions.find(sessionID) != _sessions.end();
	}

	int TcpServer::GetAvailableSessionID() {
		if (_sessionIDPool.empty()) {
			return _sessionIDCounter++;
		}

		int sessionID = _sessionIDPool.back();
		_sessionIDPool.pop_back();
		return sessionID;
	}

	void TcpServer::ReleaseSessionID(int sessionID) {
		_sessionIDPool.push_back(sessionID);
	}

	void TcpServer::OnSessionEnd(std::shared_ptr<Session> session) {
		const int sessionID = session->GetSessionID();
		
		_cbOnSessionEnd(sessionID);

		{
			std::lock_guard<std::mutex> lock(_sessionMutex);
			_sessions.erase(session->GetSessionID());
			ReleaseSessionID(sessionID);
		}
	}

	void TcpServer::OnPacketReceived(std::shared_ptr<Session> session, const std::unique_ptr<Packet>& packet) {
		Packet* poolPacket = nullptr;

		{
			std::lock_guard<std::mutex> lock(_packetPoolMutex);
			poolPacket = _packetPool.Get();
		}

		*poolPacket = std::move(*packet);

		std::shared_ptr<Packet> sharedPacket(poolPacket, [this](Packet* packet) {
			std::lock_guard<std::mutex> lock(_packetPoolMutex);
			_packetPool.Release(packet);
		});

		_cbOnPacketReceived(session->GetSessionID(), sharedPacket);
	}
}