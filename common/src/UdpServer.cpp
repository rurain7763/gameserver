#include "UdpServer.h"

#include <iostream>

namespace flaw {
	UdpServer::UdpServer(boost::asio::io_context& ioContext)
		:	_socket(ioContext), 
			_packetPool(100), 
			_currentPacket(std::make_unique<Packet>()),
			_isReceiving(false), 
			_recvBuffer(1024),
			_checkSum(0)
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

			_checkSum = flaw::Utils::GetCheckSum(_checkSumData, sizeof(_checkSumData));

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

				int offset = 0;
				_currentPacket->header.packetSize = 0;

				// check checksum
				if (bytesTransferred < sizeof(_checkSumData)) {
					std::cout << "Not enough data for checksum\n";
					StartRecv();
					return;
				}
				
				int checksum = 0;
				for (; offset < sizeof(_checkSumData); offset++) {
					checksum += _recvBuffer[offset];
				}

				if(((checksum + _checkSum) & 0xff) != 0) {
					std::cout << "Invalid checksum\n";
					StartRecv();
					return;
				}

				if (bytesTransferred - offset < sizeof(PacketHeader)) {
					std::cout << "Not enough data for header\n";
					StartRecv();
					return;
				}

				_currentPacket->SetHeaderBE(_recvBuffer.data(), offset);

				if (!Packet::IsValidPacketId(_currentPacket->header.packetId)) {
					std::cout << "Invalid packet id\n";
					StartRecv();
					return;
				}

				if (bytesTransferred - offset < _currentPacket->GetSerializedSize()) {
					std::cout << "Not enough data for packet\n";
					StartRecv();
					return;
				}

				std::cout << "Received packet: packetid=" << _currentPacket->header.packetId << " size=" << _currentPacket->header.packetSize << "\n";

				_currentPacket->SetSerializedData(_recvBuffer.data(), offset);
				OnPacketReceived();

				_StartRecv();
			}
		);
	}

	void UdpServer::Send(const Peer& endpoint, std::shared_ptr<Packet> packet) {
		boost::asio::streambuf sendBuffer;
		std::ostream os(&sendBuffer);

		// append checksum bytes
		os.write(reinterpret_cast<const char*>(_checkSumData), sizeof(_checkSumData));

		packet->GetPacketRaw(os);

		_socket.async_send_to(
			sendBuffer.data(),
			endpoint.endpoint,
			[this](const boost::system::error_code& error, size_t bytesTransferred) {
				if (error) {
					HandleError(error);
					return;
				}

				// std::cout << "Sent " << bytesTransferred << " bytes\n";
			}
		);
	}

	void UdpServer::Shutdown() {
		_socket.close();
	}

	void UdpServer::OnPacketReceived() {
		Packet* poolPacket = nullptr;

		{
			std::lock_guard<std::mutex> lock(_packetPoolMutex);
			poolPacket = _packetPool.Get();
		}

		*poolPacket = std::move(*_currentPacket);

		std::shared_ptr<Packet> packet(poolPacket, [this](Packet* packet) {
			std::lock_guard<std::mutex> lock(_packetPoolMutex);
			_packetPool.Release(packet);
		});

		_cbOnPacketReceived(Peer{ _lastEndpoint }, packet);
	}

	void UdpServer::HandleError(const boost::system::error_code& error) {
		std::cerr << "Error: " << error.message() << std::endl;
	}
}
