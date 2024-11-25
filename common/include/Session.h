#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <functional>
#include <vector>
#include <deque>
#include <chrono>

#include "Packet.h"

namespace flaw {
	class Session : public std::enable_shared_from_this<Session> {
	public:
		Session(int sessionID, boost::asio::ip::tcp::socket&& socket);

		void StartRecv();
		void StartSend(std::shared_ptr<Packet> packet);
		void StartSend(const std::vector<std::shared_ptr<Packet>>& packets);

		void Close();

		inline void SetOnSessionEnd(std::function<void(std::shared_ptr<Session>)> cb) { _cbOnSessionEnd = cb; }
		inline void SetOnPacketReceived(std::function<void(std::shared_ptr<Session>, const std::unique_ptr<Packet>&)> cb) { _cbOnPacketReceived = cb; }

		inline int GetSessionID() const { return _sessionID; }

		static std::shared_ptr<Session> Create(int sessionID, boost::asio::ip::tcp::socket&& socket);

	private:
		void HandleSend();

		void HandleError(const boost::system::error_code& error);

	private:
		static const int RECV_BUFFER_SIZE = 1024;
		static const int RECV_BUFFER_STREAM_SIZE = 4096;
		static const int SEND_BUFFER_STREAM_SIZE = 4096;

		int _sessionID;
		boost::asio::ip::tcp::socket _socket;

		std::unique_ptr<Packet> _currentPacket;
		std::vector<char> _recvBuffer;
		boost::asio::streambuf _recvBufferStream;

		std::mutex _sendMutex;
		bool _isSending;
		boost::asio::streambuf _sendBufferStream;

		std::function<void(std::shared_ptr<Session>)> _cbOnSessionEnd;
		std::function<void(std::shared_ptr<Session>, const std::unique_ptr<Packet>&)> _cbOnPacketReceived;
	};
}

#endif