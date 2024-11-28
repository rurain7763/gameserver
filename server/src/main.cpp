#include "Config.h"
#include "LobyServer.h"
#include "ChatServer.h"
#include "DatabaseServer.h"

#include <iostream>
#include <fstream>

Config ReadConfigFile()
{
	std::ifstream configFile("./config.txt", std::ios::in);
	if (!configFile.is_open()) {
		throw std::runtime_error("Failed to open config file");
	}

	Config config;
	configFile >> config.lobyServerIp;
	configFile >> config.lobyServerPort;
	configFile >> config.chatServerIp;
	configFile >> config.chatServerPort;
	configFile >> config.mysqlIp;
	configFile >> config.mysqlUser;
	configFile >> config.mysqlPassword;
	configFile >> config.mysqlDatabase;

	return config;
}

int main() {
	try {
		boost::asio::io_context ioContext;
		boost::asio::io_context::work idleWork(ioContext);
		std::thread contextThread([&ioContext]() { ioContext.run(); });

		// this is must be read from config file
		Config config = ReadConfigFile();
		
		auto dbServer = std::make_shared<DatabaseServer>(config, ioContext);
		auto lobyServer = std::make_shared<LobyServer>(config, ioContext, dbServer);
		auto chatServer = std::make_shared<ChatServer>(config, ioContext);

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
			dbServer->Update();
			lobyServer->Update();
			chatServer->Update();
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
