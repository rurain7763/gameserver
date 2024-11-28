#include "Session.h"

#include <iostream>
#include <sstream>

namespace flaw {
	Session::Session(int sessionID, boost::asio::ip::tcp::socket&& socket)
		:	_sessionID(sessionID), 
			_socket(std::move(socket)), 
			_currentPacket(std::make_unique<Packet>()),
			_recvBuffer(RECV_BUFFER_SIZE),
			_recvBufferStream(RECV_BUFFER_STREAM_SIZE),
			_isSending(false),
			_sendBufferStream(SEND_BUFFER_STREAM_SIZE)
	{	
	}

	void Session::StartRecv() {
		_socket.async_read_some(
			boost::asio::buffer(_recvBuffer), 
			[this, self = shared_from_this()](const boost::system::error_code& error, size_t bytesTransferred) {
				if (error) {
					HandleError(error);
					_cbOnSessionEnd(self);
					return;
				}

				// std::cout << "Received " << bytesTransferred << " bytes\n";
				
				std::ostream os(&_recvBufferStream);
				os.write(&_recvBuffer[0], bytesTransferred);

				while (true) {
					size_t streamSize = _recvBufferStream.size();
					const char* data = boost::asio::buffer_cast<const char*>(_recvBufferStream.data());

					int offset = 0;

					// handle packet header
					if (_currentPacket->header.packetSize == 0) {
						if (streamSize < sizeof(PacketHeader)) {
							// std::cout << "Not enough data for header\n";
							break;
						}

						_currentPacket->SetHeaderBE(data, offset);

						if (streamSize - offset < _currentPacket->GetSerializedSize()) {
							// std::cout << "Not enough data for packet\n";
							_recvBufferStream.consume(offset);
							break;
						}
					}

					// handle packet data
					if (streamSize - offset < _currentPacket->GetSerializedSize()) {
						// std::cout << "Not enough data for packet\n";
						break;
					}

					_currentPacket->SetSerializedData(data, offset);
					_cbOnPacketReceived(self, _currentPacket);

					// consume data
					_recvBufferStream.consume(offset);
					_currentPacket->header.packetSize = 0;
				}

				StartRecv();
			}
		);
	}

	void Session::StartSend(std::shared_ptr<Packet> packet) {
		{
			std::lock_guard<std::mutex> lock(_sendMutex);

			std::ostream os(&_sendBufferStream);
			packet->GetPacketRaw(os);
			
			if (_isSending) {
				return;
			}
			_isSending = true;
		}

		HandleSend();
	}

	void Session::StartSend(const std::vector<std::shared_ptr<Packet>>& packets) {
		{
			std::lock_guard<std::mutex> lock(_sendMutex);

			for (const auto& packet : packets) {
				std::ostream os(&_sendBufferStream);
				packet->GetPacketRaw(os);
			}

			if (_isSending) {
				return;
			}
			_isSending = true;
		}

		HandleSend();
	}

	void Session::HandleSend() {
		if (!_sendBufferStream.size()) {
			std::lock_guard<std::mutex> lock(_sendMutex);
			_isSending = false;
			return;
		}

		boost::asio::async_write(
			_socket, 
			_sendBufferStream.data(),
			[this, self = shared_from_this()](const boost::system::error_code& error, size_t bytesTransferred) {
				if (error) {
					HandleError(error);
					_cbOnSessionEnd(self);
					return;
				}

				// std::cout << "Sent " << bytesTransferred << " bytes" << std::endl;

				{
					std::lock_guard<std::mutex> lock(_sendMutex);
					_sendBufferStream.consume(bytesTransferred);
				}

				if (!_sendBufferStream.size()) {
					std::lock_guard<std::mutex> lock(_sendMutex);
					_isSending = false;
					return;
				}

				HandleSend();
			}
		);
	}

	void Session::Close() {
		if (_socket.is_open()) {
			boost::system::error_code ec;
			_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			_socket.close(ec);
		}
	}

	void Session::HandleError(const boost::system::error_code& error) {
		if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
			std::cout << "Connection closed by peer" << std::endl;
		} else {
			std::cerr << "Error: " << error.message() << std::endl;
		}
	}

	std::shared_ptr<Session> Session::Create(int sessionID, boost::asio::ip::tcp::socket&& socket) {
		return std::make_shared<Session>(sessionID, std::move(socket));
	}
}
