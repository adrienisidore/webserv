#ifndef GLOBALCONFIG_HPP
# define GLOBALCONFIG_HPP

//Code handled :
//100, 200, 201, 400, 403, 404, 405, 408, 411, 413, 414, 500, 501

#include "./webserv.hpp"

// Global → paramètres généraux pour tout le serveur (ex. root, error_page, etc.), le fichier de configuration
//  └── server → paramètres pour un “virtual host” (une IP:port, un site)
//        └── location → paramètres pour une route spécifique (ex. /images/)
			// ├── location / {}
			// ├── location /images {}
			// └── location /cgi-bin {}
			// }

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
	const std::map<std::string, std::string>	&getDirectives() const;


	//to get and modify the servers
	std::map<std::string, ServerConfig> &accessServers();
};


#endif
