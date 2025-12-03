#ifndef GLOBALCONFIG_HPP
# define GLOBALCONFIG_HPP	

#include "./webserv.hpp"

class ServerConfig;

class GlobalConfig {
private:
	//key = directive_name
	std::map<std::string, std::string> directives; // ex: root, error_page, ...
	//key = Host/IP:Port
	std::map<std::string, ServerConfig> servers;

public:
	GlobalConfig();
	~GlobalConfig();

	void setDirective(const std::string &key, const std::string &value);

	void addServer(const std::string &key, const ServerConfig &server);

	std::string									getDirective(const std::string &key) const;
	const std::map<std::string, std::string> &	getDirectives() const;
	const std::map<std::string, ServerConfig> &	getServers() const;

	//to get and modify the servers
	std::map<std::string, ServerConfig> &accessServers();
};


#endif
