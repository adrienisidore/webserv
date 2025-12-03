#include "webserv.hpp"

#include "GlobalConfig.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

ServerConfig::ServerConfig() {}

void		ServerConfig::setDirective(const std::string &key, const std::string &value) {
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

void		ServerConfig::inheritFromGlobal(const GlobalConfig &global) {

	bool hadErrorPage   = (directives.find("error_page")   != directives.end());
	bool hadCgiHandler  = (directives.find("cgi_handler")  != directives.end());

	const std::map<std::string, std::string> &globalDirectives = global.getDirectives();
	for (std::map<std::string, std::string>::const_iterator it = globalDirectives.begin();
		it != globalDirectives.end(); ++it) {
		if (directives.find(it->first) == directives.end())
			// l'enfant ne possede pas la directive "it->first" du parent, donc il en herite
			directives[it->first] = it->second;
	}

	// Si l'enfant a deja la directive "error_page" || "cgi_handler" et le parent aussi alors je dois les concatener + '\n'
	if (hadErrorPage) {
		std::map<std::string, std::string>::const_iterator git = globalDirectives.find("error_page");
		if (git != globalDirectives.end()) {
			std::map<std::string, std::string>::iterator sit = directives.find("error_page");
			if (sit != directives.end())
				sit->second = git->second + "\n" + sit->second;
		}
	}
	if (hadCgiHandler) {
		std::map<std::string, std::string>::const_iterator git = globalDirectives.find("cgi_handler");
		if (git != globalDirectives.end()) {
			std::map<std::string, std::string>::iterator sit = directives.find("cgi_handler");
			if (sit != directives.end())
				sit->second = git->second + "\n" + sit->second;
		}
	}
}

void ServerConfig::addLocation(const std::string &path, LocationConfig &location) {
	location.setDirective("location_uri", path);
    locations[path] = location;
}

std::string	ServerConfig::getDirective(const std::string &key) const {
	std::map<std::string, std::string>::const_iterator it = directives.find(key);
	if (it != directives.end())
		return it->second;
	return "";
}

const std::map<std::string, std::string> & ServerConfig::getDirectives() const { return directives; }

const std::map<std::string, LocationConfig> & ServerConfig::getLocations() const { return locations; }

std::map<std::string, LocationConfig>		&ServerConfig::accessLocations() { return locations; }

ServerConfig::~ServerConfig() {}
