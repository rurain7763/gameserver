#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <boost/asio.hpp>
#include <functional>

#include "Global.h"
#include "Packet.h"
#include "Session.h"
#include "ObjectPool.h"

namespace flaw {
	class FLAW_API TcpClient {
	public:
		TcpClient(boost::asio::io_context& ioContext);

		void Connect(const std::string ip, const short port);
		void Disconnect();

		void Send(std::shared_ptr<Packet> packet);
		void Send(const std::vector<std::shared_ptr<Packet>>& packets);

		inline void SetOnSessionStart(std::function<void()> cb) { _cbOnSessionStart = cb; }
		inline void SetOnSessionEnd(std::function<void()> cb) { _cbOnSessionEnd = cb; }
		inline void SetOnPacketReceived(std::function<void(std::shared_ptr<Packet> packet)> cb) { _cbOnPacketReceived = cb; }

	private:
		void OnSessionEnd(std::shared_ptr<Session> session);
		void OnPacketReceived(std::shared_ptr<Session> session, const std::unique_ptr<Packet>& packet);

	private:
		static const int MAX_PACKET_POOL_SIZE = 1024;

		boost::asio::io_context& _ioContext;
		std::shared_ptr<Session> _session;

		std::mutex _packetPoolMutex;
		ObjectPool<Packet> _packetPool;

		std::function<void()> _cbOnSessionStart;
		std::function<void()> _cbOnSessionEnd;
		std::function<void(std::shared_ptr<Packet> packet)> _cbOnPacketReceived;
	};
}

#endif
