#include "webserv.hpp"

#include "GlobalConfig.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

ServerConfig::ServerConfig() {}

void		ServerConfig::setDirective(const std::string &key, const std::string &value) {
	directives[key] = value;
}

void		ServerConfig::inheritFromGlobal(const GlobalConfig &global) {
    const std::map<std::string, std::string> &globalDirectives = global.getDirectives();
    for (std::map<std::string, std::string>::const_iterator it = globalDirectives.begin();
         it != globalDirectives.end(); ++it) {
        if (directives.find(it->first) == directives.end())
            directives[it->first] = it->second;
    }
}

void ServerConfig::addLocation(const std::string &path, LocationConfig &location) {
	location.setDirective("uri", path);
    locations[path] = location;
    locations[path].inheritFromServer(*this);

}

std::string	ServerConfig::getDirective(const std::string &key) const {
	std::map<std::string, std::string>::const_iterator it = directives.find(key);
	if (it != directives.end())
		return it->second;
	return "";
}

const std::map<std::string, std::string> & ServerConfig::getDirectives() const { return directives; }

const std::map<std::string, LocationConfig> & ServerConfig::getLocations() const { return locations; }

ServerConfig::~ServerConfig() {}
