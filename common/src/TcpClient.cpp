#include "TcpClient.h"

#include <iostream>

namespace flaw {
	TcpClient::TcpClient(boost::asio::io_context& ioContext) 
		: _ioContext(ioContext), _session(nullptr), _packetPool(MAX_PACKET_POOL_SIZE)
	{
	}

	void TcpClient::Connect(const std::string ip, const short port) {
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

		auto socket = std::make_shared<boost::asio::ip::tcp::socket>(_ioContext);
		socket->open(endpoint.protocol());
		socket->set_option(boost::asio::ip::tcp::no_delay(true));
		socket->set_option(boost::asio::socket_base::keep_alive(true));
		socket->set_option(boost::asio::socket_base::send_buffer_size(8192));
		socket->set_option(boost::asio::socket_base::receive_buffer_size(8192));

		socket->async_connect(endpoint, [this, socket](boost::system::error_code ec) {
			if (!ec) {
				_session = Session::Create(0, std::move(*socket));
				_session->SetOnSessionEnd(std::bind(&TcpClient::OnSessionEnd, this, std::placeholders::_1));
				_session->SetOnPacketReceived(std::bind(&TcpClient::OnPacketReceived, this, std::placeholders::_1, std::placeholders::_2));

				_session->StartRecv();

				_cbOnSessionStart();
			}
		});
	}

	void TcpClient::Disconnect() {
		_session->Close();
		_session.reset();
	}

	void TcpClient::Send(std::shared_ptr<Packet> packet) {
		_session->StartSend(packet);
	}

	void TcpClient::Send(const std::vector<std::shared_ptr<Packet>>& packets) {
		_session->StartSend(packets);
	}

	void TcpClient::OnSessionEnd(std::shared_ptr<Session> session) {
		_cbOnSessionEnd();
	}

	void TcpClient::OnPacketReceived(std::shared_ptr<Session> session, const std::unique_ptr<Packet>& packet) {
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

		_cbOnPacketReceived(sharedPacket);
	}

}