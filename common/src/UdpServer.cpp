#include "UdpServer.h"

#include <iostream>

namespace flaw {
	UdpServer::UdpServer(boost::asio::io_context& ioContext)
		: _socket(ioContext), _isReceiving(false), _recvBuffer(1024)
	{
	}

	void UdpServer::Bind(const std::string& ip, const short port) {
		boost::asio::ip::udp::endpoint endpoint;

		if (ip.find(":") != std::string::npos) {
			auto address = boost::asio::ip::make_address_v6(ip);
			endpoint = boost::asio::ip::udp::endpoint(address, port);
		}
		else if (ip.find(".") != std::string::npos) {
			auto address = boost::asio::ip::make_address_v4(ip);
			endpoint = boost::asio::ip::udp::endpoint(address, port);
		}
		else {
			throw std::invalid_argument("Invalid IP address");
		}

		_socket.open(endpoint.protocol());
		_socket.bind(endpoint);
	}

	void UdpServer::StartRecv() {
		if (!_isReceiving) {
			_isReceiving = true;
			_StartRecv();
		}
	}

	void UdpServer::_StartRecv() {
		_socket.async_receive_from(
			boost::asio::buffer(_recvBuffer),
			_lastEndpoint,
			[this](const boost::system::error_code& error, size_t bytesTransferred) {
				if (error) {
					HandleError(error);
					return;
				}

				OnPacketReceived({ _lastEndpoint }, _recvBuffer.data(), bytesTransferred);
				_StartRecv();
			}
		);
	}

	void UdpServer::Send(const Peer& endpoint, const char* buffer, size_t size) {
		_socket.async_send_to(
			boost::asio::buffer(buffer, size),
			endpoint.endpoint,
			[this](const boost::system::error_code& error, size_t bytesTransferred) {
				if (error) {
					HandleError(error);
					return;
				}
			}
		);
	}

	void UdpServer::OnPacketReceived(const Peer& endpoint, const char* data, size_t size) {
		_cbOnPacketReceived(endpoint, data, size);
	}

	void UdpServer::HandleError(const boost::system::error_code& error) {
		std::cerr << "Error: " << error.message() << std::endl;
	}
}
