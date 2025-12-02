#ifndef AUTOCONFIG_HPP
# define AUTOCONFIG_HPP

# include "webserv.hpp"

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

// pour global, server et location : root, listen, index, error_page, (redir?)
// static void		basic_tests(T)

GlobalConfig AutoConfig(const std::string & filename);

#endif
