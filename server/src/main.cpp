#include <boost/asio.hpp>
#include <iostream>

#include "Global.h"
#include "TcpServer.h"
#include "UdpServer.h"
#include "Serialization.h"

#if true
std::shared_ptr<flaw::TcpServer> server;

void OnSessionStart(int sessionID) {
	std::cout << "Session start: " << sessionID << std::endl;
}

void OnSessionEnd(int sessionID) {
	std::cout << "Session end: " << sessionID << std::endl;
}

void OnPacketReceived(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	if(packet->header.packetId == PacketType::PACKET_TYPE_A) {
		A a = packet->GetData<A>();
		std::cout << "Packet received: " << a.c_data << " " << a.i_data << " " << a.f_data << std::endl;
	}

	server->Send(sessionID, packet);
}

void main() {
	try {
		boost::asio::io_context ioContext;

		boost::asio::io_context::work idleWork(ioContext);

		std::thread contextThread([&ioContext]() { ioContext.run(); });

		server = std::make_shared<flaw::TcpServer>(ioContext);
		server->SetOnSessionStart(OnSessionStart);
		server->SetOnSessionEnd(OnSessionEnd);
		server->SetOnPacketReceived(OnPacketReceived);

		bool running = true;

		std::thread inputThread([&running]() {
			std::string input;
			while (running) {
				std::getline(std::cin, input);
				if (input == "exit") {
					running = false;
				}
			}
		});

		server->StartListen("127.0.0.1", 8080);
		server->StartAccept();

		while (running)
		{
			// Do something
		}

		server->Shutdown();
		ioContext.stop();

		contextThread.join();
		inputThread.join();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return;
}
#else
void OnPacketReceived(const flaw::Peer& peer, const char* data, size_t size) {
	std::cout << "Packet received: " << size << std::endl;
}

int main() {
	boost::asio::io_context ioContext;

	boost::asio::io_context::work idleWork(ioContext);

	std::thread contextThread([&ioContext]() { ioContext.run(); });

	flaw::UdpServer server(ioContext);

	server.Bind("127.0.0.1", 1234);
	server.SetOnPacketReceived(OnPacketReceived);
	server.StartRecv();

	flaw::Peer peer = flaw::Peer::Create("127.0.0.1", 1235);

	while (true) {
		std::string input;
		std::getline(std::cin, input);

		if (input == "exit") {
			break;
		}

		server.Send(peer, input.c_str(), input.size() + 1);
	}

	return 0;
}
#endif