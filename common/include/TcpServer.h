#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <boost/asio.hpp>
#include <functional>
#include <unordered_map>
#include <vector>

#include "Packet.h"
#include "Session.h"
#include "ObjectPool.h"

namespace flaw {
	class TcpServer {
	public:
		TcpServer(boost::asio::io_context& ioContext);

		void StartListen(const std::string ip, const short port);
		void StartAccept();

		void Shutdown();

		void Send(int sessionID, std::shared_ptr<Packet> packet);

		inline void SetOnSessionStart(std::function<void(int)> cb) { _cbOnSessionStart = cb; }
		inline void SetOnSessionEnd(std::function<void(int)> cb) { _cbOnSessionEnd = cb; }
		inline void SetOnPacketReceived(std::function<void(int, std::shared_ptr<Packet> packet)> cb) { _cbOnPacketReceived = cb; }

	private:
		int GetAvailableSessionID();
		void ReleaseSessionID(int sessionID);

		void OnSessionEnd(std::shared_ptr<Session> session);
		void OnPacketReceived(std::shared_ptr<Session> session, const std::unique_ptr<Packet>& packet);

	private:
		static const int MAX_PACKET_POOL_SIZE = 1024;

		boost::asio::ip::tcp::acceptor _acceptor;

		std::mutex _sessionMutex;
		std::unordered_map<int, std::shared_ptr<Session>> _sessions;

		int _sessionIDCounter;
		std::vector<int> _sessionIDPool;

		std::mutex _packetPoolMutex;
		ObjectPool<Packet> _packetPool;

		std::function<void(int)> _cbOnSessionStart;
		std::function<void(int)> _cbOnSessionEnd;
		std::function<void(int, std::shared_ptr<Packet>)> _cbOnPacketReceived;
	};
}

#endif
