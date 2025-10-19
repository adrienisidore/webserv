#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

#include "./webserv.hpp"

class ServerConfig;

//1 instance LocationConfig == 1 location
class LocationConfig {
private:
	std::map<std::string, std::string> directives;

public:
	LocationConfig();
	~LocationConfig();

	// Définit une directive dans ce bloc
	void setDirective(const std::string &key, const std::string &value);

	void inheritFromServer(const ServerConfig &server);

	std::string									getDirective(const std::string &key) const;

	const std::map<std::string, std::string>	&getDirectives() const;
};

#endif


