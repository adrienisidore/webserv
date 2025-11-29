#include "webserv.hpp"

#include "LocationConfig.hpp"
#include "ServerConfig.hpp"

LocationConfig::LocationConfig() {}

// Définit une directive dans ce bloc
void		LocationConfig::setDirective(const std::string &key, const std::string &value) {
	if (key == "error_page" || key == "cgi_handler") {

		std::map<std::string, std::string>::iterator it = directives.find(key);
		if (it != directives.end())
		{
			it->second += "\n";
			it->second += value;
		}
		else
			directives[key] = value;
	}
	else
		directives[key] = value;
}

void LocationConfig::inheritFromServer(const ServerConfig &server) {

	bool hadErrorPage   = (directives.find("error_page")   != directives.end());
	bool hadCgiHandler  = (directives.find("cgi_handler")  != directives.end());

    const std::map<std::string, std::string> &serverDirectives = server.getDirectives();
    for (std::map<std::string, std::string>::const_iterator it = serverDirectives.begin();
         it != serverDirectives.end(); ++it) {
        if (directives.find(it->first) == directives.end())
            directives[it->first] = it->second;
    }

	// Si l'enfant a deja la directive "error_page" || "cgi_handler" et le parent aussi alors je dois les concatener + '\n'
	if (hadErrorPage) {
		std::map<std::string, std::string>::const_iterator git = serverDirectives.find("error_page");
		if (git != serverDirectives.end()) {
			std::map<std::string, std::string>::iterator sit = directives.find("error_page");
			if (sit != directives.end())
				sit->second = git->second + "\n" + sit->second;
		}
	}
	if (hadCgiHandler) {
		std::map<std::string, std::string>::const_iterator git = serverDirectives.find("cgi_handler");
		if (git != serverDirectives.end()) {
			std::map<std::string, std::string>::iterator sit = directives.find("cgi_handler");
			if (sit != directives.end())
				sit->second = git->second + "\n" + sit->second;
		}
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
