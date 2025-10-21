#include "webserv.hpp"

#include "GlobalConfig.hpp"
#include "ServerConfig.hpp"

GlobalConfig::GlobalConfig() {}

void		GlobalConfig::setDirective(const std::string &key, const std::string &value) {
	directives[key] = value;
}


void		GlobalConfig::addServer(const std::string &key, const ServerConfig &server) {
	servers[key] = server;
	servers[key].inheritFromGlobal(*this);

}

std::string	GlobalConfig::getDirective(const std::string &key) const {
	std::map<std::string, std::string>::const_iterator it = directives.find(key);
	if (it != directives.end())
		return it->second;
	return "";
}

const std::map<std::string, std::string> & GlobalConfig::getDirectives() const { return directives; }


std::map<std::string, ServerConfig> & GlobalConfig::accessServers() { return servers; }

const std::map<std::string, ServerConfig> &	GlobalConfig::getServers() const {return servers;}

GlobalConfig::~GlobalConfig() {}
