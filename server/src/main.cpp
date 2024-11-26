#include <boost/asio.hpp>
#include <iostream>
#include <queue>
#include <mutex>

#include "TcpServer.h"
#include "UdpServer.h"
#include "Serialization.h"
#include "PacketDataTypes.h"
#include "MySql.h"

class PacketQueue {
public:
	class Entry {
	public:
		Entry(std::shared_ptr<flaw::Packet> packet, std::function<void(std::shared_ptr<flaw::Packet>)> resolver)
			: _packet(packet), resolver(resolver)
		{
		}

		void operator()(std::shared_ptr<flaw::Packet> packet) {
			resolver(packet);
		}

		std::shared_ptr<flaw::Packet> GetPacket() const {
			return _packet;
		}

	private:
		std::shared_ptr<flaw::Packet> _packet;
		std::function<void(std::shared_ptr<flaw::Packet>)> resolver;
	};

	void Push(Entry&& entry) {
		std::lock_guard<std::mutex> lock(_packetQueueMutex);
		_packetQueue.push(std::move(entry));
	}

	Entry Pop() {
		std::lock_guard<std::mutex> lock(_packetQueueMutex);
		auto entry = _packetQueue.front();
		_packetQueue.pop();
		return entry;
	}

	bool Empty() {
		std::lock_guard<std::mutex> lock(_packetQueueMutex);
		return _packetQueue.empty();
	}

private:
	std::mutex _packetQueueMutex;
	std::queue<Entry> _packetQueue;
};

PacketQueue packetQueue;
std::shared_ptr<flaw::TcpServer> tcpServer;
std::shared_ptr<flaw::UdpServer> udpServer;

void OnTcpServerSessionStart(int sessionID) {
	std::cout << "Session start" << sessionID << std::endl;
}

void OnTcpServerSessionEnd(int sessionID) {
	std::cout << "Session end: " << sessionID << std::endl;
}

void OnTcpServerPacketReceived(int sessionID, std::shared_ptr<flaw::Packet> packet) {
	packetQueue.Push(PacketQueue::Entry(packet, [sessionID](auto packet) { tcpServer->Send(sessionID, packet); }));
}

void OnUdpServerPacketReceived(flaw::Peer& peer, std::shared_ptr<flaw::Packet> packet) {
	packetQueue.Push(PacketQueue::Entry(packet, [peer](auto packet) { udpServer->Send(peer, packet); }));
}

int main() {
	try {
		boost::asio::io_context ioContext;
		boost::asio::io_context::work idleWork(ioContext);
		std::thread contextThread([&ioContext]() { ioContext.run(); });

		auto mySql = std::make_shared<flaw::MySql>();
		mySql->Connect("192.168.50.18", "ldh", "Jjang_dong12");

		tcpServer = std::make_shared<flaw::TcpServer>(ioContext);
		tcpServer->SetOnSessionStart(OnTcpServerSessionStart);
		tcpServer->SetOnSessionEnd(OnTcpServerSessionEnd);
		tcpServer->SetOnPacketReceived(OnTcpServerPacketReceived);

		udpServer = std::make_shared<flaw::UdpServer>(ioContext);
		udpServer->SetOnPacketReceived(OnUdpServerPacketReceived);

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

		mySql->SetSchema("a");

		mySql->Select<int, std::string>(
			"id, test",
			"user",
			[](const int id, const std::string& password) {
				std::cout << "id: " << id << ", password: " << password << std::endl;
			}
		);

		tcpServer->Bind("127.0.0.1", 8080);
		tcpServer->StartListen();
		tcpServer->StartAccept();

		udpServer->Bind("127.0.0.1", 8080);
		udpServer->StartRecv();

		while (running) {
			if (!packetQueue.Empty()) {
				auto entry = packetQueue.Pop();
				auto& packet = entry.GetPacket();

				if (packet->header.packetId == PacketId::Login) {
					LoginData loginData;
					packet->GetData(loginData);

					MessageData messageData;
					if (loginData.username == "admin" && loginData.password == "1234") {
						messageData.message = "Login successful, hello admin";

						auto responsePacket = std::make_shared<flaw::Packet>(0, PacketId::Message, messageData);
						entry(responsePacket);
					}
					else {
						messageData.message = "Login failed, who are you?";

						auto responsePacket = std::make_shared<flaw::Packet>(0, PacketId::Message, messageData);
						entry(responsePacket);
					}
				}
				else if (packet->header.packetId == PacketId::Message) {
					MessageData messageData;
					packet->GetData(messageData);

					std::cout << "Message received: " << messageData.message << std::endl;
				}
			}
		}

		mySql->Disconnect();
		tcpServer->Shutdown();
		ioContext.stop();

		contextThread.join();
		inputThread.join();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
