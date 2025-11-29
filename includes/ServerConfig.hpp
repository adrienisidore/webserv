#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

#include "./webserv.hpp"

class GlobalConfig;
class LocationConfig;

//1 instance ServerConfig == 1 serveur
class ServerConfig {
private:
	std::map<std::string, std::string>		directives;
	std::map<std::string, LocationConfig>	locations;

public:
	ServerConfig();
	~ServerConfig();

	void	setDirective(const std::string &key, const std::string &value);

	void	inheritFromGlobal(const GlobalConfig &global);

	void	addLocation(const std::string &path, LocationConfig &loc);

	std::string									getDirective(const std::string &key) const;
	const std::map<std::string, std::string> &	getDirectives() const;


	const std::map<std::string, LocationConfig> &getLocations() const;
	std::map<std::string, LocationConfig>		&accessLocations();
};

#endif
