#include "webserv.hpp"

#include "LocationConfig.hpp"
#include "ServerConfig.hpp"

LocationConfig::LocationConfig() {}

// Définit une directive dans ce bloc
void		LocationConfig::setDirective(const std::string &key, const std::string &value) {
	directives[key] = value;
}

void LocationConfig::inheritFromServer(const ServerConfig &server) {
    const std::map<std::string, std::string> &serverDirectives = server.getDirectives();
    for (std::map<std::string, std::string>::const_iterator it = serverDirectives.begin();
         it != serverDirectives.end(); ++it) {
        if (directives.find(it->first) == directives.end())
            directives[it->first] = it->second;
    }
}

// Accès à une directive
std::string	LocationConfig::getDirective(const std::string &key) const {
	std::map<std::string, std::string>::const_iterator it = directives.find(key);
	if (it != directives.end())
		return it->second;
	return "";
}

const std::map<std::string, std::string> & LocationConfig::getDirectives() const { return directives; }

LocationConfig::~LocationConfig() {}
