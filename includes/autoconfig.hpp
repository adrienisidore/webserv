#ifndef AUTOCONFIG_HPP
# define AUTOCONFIG_HPP

# include "webserv.hpp"
# include "Exceptions.hpp"

bool isSafePath(const std::string &path);
bool isValidPort(const std::string& port_str);
bool isValidIP(const std::string& ip_str);
void validateListenFormat(const std::string& listen_value, const std::string& context);
void validateCgiDirective(const std::string &handlers, const std::string &context);
void validateAllowedMethods(const std::string &value, const std::string &context);

template< typename T >
void	AutoConfig_setDirective(T & config_element, const std::string & line) {

	size_t			pos = line.find(' '); //distinguer le nom de la valeur
	size_t			pos_colon = line.find(';');
	std::string		name;
	std::string		value;

	if (pos == std::string::npos || pos_colon == std::string::npos)
        return;
	name = line.substr(0, pos);
	value = line.substr(pos + 1, pos_colon - pos - 1);
	config_element.setDirective(name, value);
}

template <typename T>
void autoconfig_basic_tests(const T &glob_serv_loc, const std::string &context) {

	// tester valeur de root
	std::string root = glob_serv_loc.getDirective("root");
	if (!root.empty() && !isSafePath(root))
		throw ParsingException(context + " root path is unsafe");
	
	std::string autoindex = glob_serv_loc.getDirective("autoindex");
	if (!autoindex.empty() && autoindex != "on" && autoindex != "off") {
			throw ParsingException(context + ": autoindex value must be 'on' or 'off'. Found '" + autoindex + "'");
	}

	std::string listen = glob_serv_loc.getDirective("listen");
	if (!listen.empty()) {
			validateListenFormat(listen, context);
	}

	std::string cgi = glob_serv_loc.getDirective("cgi_handler");
	if (!cgi.empty()) {
		validateCgiDirective(cgi, context);
	}

	std::string allowed = glob_serv_loc.getDirective("allowed_methods");
	if (!allowed.empty()) {
		validateAllowedMethods(allowed, context);
	}

	// return ne peut etre present que dans les locations
	if ((context == "Global" || context == "Server") && !glob_serv_loc.getDirective("return").empty())
		throw ParsingException("Directive return only authorized in location(s)");
}



GlobalConfig AutoConfig(const std::string & filename);

#endif
