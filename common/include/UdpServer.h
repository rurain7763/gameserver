#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <boost/asio.hpp>

#include "Global.h"
#include "ObjectPool.h"
#include "Packet.h"

namespace flaw {
	struct FLAW_API Peer {
		boost::asio::ip::udp::endpoint endpoint;

		static Peer Create(const std::string& ip, const short port) {
			Peer peer;
			if (ip.find(":") != std::string::npos) {
				auto address = boost::asio::ip::make_address_v6(ip);
				peer.endpoint = boost::asio::ip::udp::endpoint(address, port);
			}
			else if (ip.find(".") != std::string::npos) {
				auto address = boost::asio::ip::make_address_v4(ip);
				peer.endpoint = boost::asio::ip::udp::endpoint(address, port);
			}
			else {
				throw std::invalid_argument("Invalid IP address");
			}

			return peer;
		}
	};

	class FLAW_API UdpServer {
	public:
		UdpServer(boost::asio::io_context& ioContext);

		void Bind(const std::string& ip, const short port);

		void StartRecv();
		void Send(const Peer& endpoint, std::shared_ptr<Packet> packet);

		void Shutdown();

		inline void SetOnPacketReceived(std::function<void(Peer&, std::shared_ptr<Packet>)> cb) { _cbOnPacketReceived = cb; }

	private:
		void _StartRecv();

		void OnPacketReceived();

		void HandleError(const boost::system::error_code& error);

	private:

		boost::asio::ip::udp::socket _socket;
		boost::asio::ip::udp::endpoint _lastEndpoint;

		std::mutex _packetPoolMutex;
		ObjectPool<Packet> _packetPool;

		bool _isReceiving;
		std::unique_ptr<Packet> _currentPacket;
		std::vector<char> _recvBuffer;

		unsigned char _checkSumData[4] = { 0x01, 0x02, 0x03, 0x04 };
		int _checkSum;

		std::function<void(Peer&, std::shared_ptr<Packet>)> _cbOnPacketReceived;
	};
}

#endif
