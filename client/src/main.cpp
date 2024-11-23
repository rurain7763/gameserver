#include <boost/asio.hpp>
#include <iostream>

#include "TcpClient.h"
#include "UdpServer.h"
#include "Serialization.h"
#include "Packet.h"
#include "TestPacket.h"

#if true
bool running = false;

void OnSessionStart() {
	std::cout << "Session start" << std::endl;
	running = true;
}

void OnSessionEnd() {
	std::cout << "Session end" << std::endl;
}

void OnPacketReceived(std::shared_ptr<flaw::Packet> packet) {
	std::cout << "Packet received" << std::endl;
}

int main() {
	try {
		boost::asio::io_context ioContext;

		boost::asio::io_context::work idleWork(ioContext);

		std::thread contextThread([&ioContext]() { ioContext.run(); });

		flaw::TcpClient client(ioContext);
		client.SetOnSessionStart(OnSessionStart);
		client.SetOnSessionEnd(OnSessionEnd);
		client.SetOnPacketReceived(OnPacketReceived);

		std::thread inputThread([]() {
			std::string input;
			while (running) {
				std::getline(std::cin, input);
				if (input == "exit") {
					running = false;
				}
			}
		});	

		client.Connect("127.0.0.1", 8080);

		while (!running) {}

		A a;
		a.c_data = 'a';
		a.i_data = 1;
		a.f_data = 1.0f;

		auto packet = std::make_shared<flaw::Packet>(1, PacketType::PACKET_TYPE_A, a);

		int count = 1;
		while (running)
		{
			// Do something
			client.Send(packet);
			count++;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 20));
		}

		client.Disconnect();
		ioContext.stop();

		contextThread.join();
		inputThread.join();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
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

	server.Bind("127.0.0.1", 1235);
	server.SetOnPacketReceived(OnPacketReceived);
	server.StartRecv();

	flaw::Peer peer = flaw::Peer::Create("127.0.0.1", 1234);

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