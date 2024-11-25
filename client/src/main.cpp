#include <boost/asio.hpp>
#include <iostream>

#include "TcpClient.h"
#include "UdpServer.h"
#include "Serialization.h"
#include "Packet.h"

enum PacketId {
	Login = 0,
	Message
};

struct LoginData {
	std::string username;
	std::string password;
	MSGPACK_DEFINE(username, password);
};

struct MessageData {
	std::string message;
	MSGPACK_DEFINE(message);
};

bool running = false;

void OnSessionStart() {
	std::cout << "Session start" << std::endl;
	running = true;
}

void OnSessionEnd() {
	std::cout << "Session end" << std::endl;
}

void OnPacketReceived(std::shared_ptr<flaw::Packet> packet) {
	if (packet->header.packetId == PacketId::Message) {
		MessageData messageData;
		packet->GetData(messageData);

		std::cout << "Message received: " << messageData.message << std::endl;
	}
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
		
		LoginData src;
		std::vector<std::shared_ptr<flaw::Packet>> packets;
		for (int i = 0; i < 100; i++) {
			src.username = "admin" + std::to_string(i);
			src.password = "1234" + std::to_string(i);

			auto packet = std::make_shared<flaw::Packet>(1, 0, src);
			packets.push_back(packet);
		}

		src.username = "admin";
		src.password = "1234";

		auto packet = std::make_shared<flaw::Packet>(1, 0, src);
		packets.push_back(packet);

		int count = 1;
		while (running)
		{
			// Do something
			client.Send(packets);
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
