#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <boost/asio.hpp>

namespace flaw {
	struct Peer {
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

	class UdpServer {
	public:
		UdpServer(boost::asio::io_context& ioContext);

		void Bind(const std::string& ip, const short port);

		void StartRecv();
		void Send(const Peer& endpoint, const char* buffer, size_t size);

		inline void SetOnPacketReceived(std::function<void(const Peer&, const char*, size_t)> cb) { _cbOnPacketReceived = cb; }

	private:
		void _StartRecv();

		void OnPacketReceived(const Peer& endpoint, const char* data, size_t size);

		void HandleError(const boost::system::error_code& error);

	private:
		boost::asio::ip::udp::socket _socket;
		boost::asio::ip::udp::endpoint _lastEndpoint;

		bool _isReceiving;
		std::vector<char> _recvBuffer;

		std::function<void(const Peer&, const char*, size_t)> _cbOnPacketReceived;
	};
}

#endif
