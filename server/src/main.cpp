#include "Resources.h"
#include "LobyServer.h"
#include "ChatServer.h"
#include "DatabaseServer.h"

#include <iostream>
#include <fstream>

int main() {
	try {
		boost::asio::io_context ioContext;
		boost::asio::io_context::work idleWork(ioContext);
		std::thread contextThread([&ioContext]() { ioContext.run(); });

		Resources resources;
		resources.LoadConfig("./config.txt");
		
		auto dbServer = std::make_shared<DatabaseServer>(resources, ioContext);
		auto lobyServer = std::make_shared<LobyServer>(resources, ioContext, dbServer);
		auto chatServer = std::make_shared<ChatServer>(resources, ioContext);

		bool running = true;

		lobyServer->Start();
		dbServer->Start();
		chatServer->Start();

		std::thread inputThread([&running]() {
			std::string input;
			while (running) {
				std::getline(std::cin, input);
				if (input == "exit") {
					running = false;
				}
			}
		});

		while (running) {
			chatServer->Update();
			lobyServer->Update();
			dbServer->Update();
		}

		lobyServer->Shutdown();
		dbServer->Shutdown();

		ioContext.stop();

		contextThread.join();
		inputThread.join();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
