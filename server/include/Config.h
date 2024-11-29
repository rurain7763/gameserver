#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
	std::string dns;

	std::string lobyServerIp;
	short lobyServerPort;

	std::string chatServerIp;
	short chatServerPort;
	

	std::string mysqlIp;
	short mysqlPort;
	std::string mysqlUser;
	std::string mysqlPassword;
	std::string mysqlDatabase;
};

#endif