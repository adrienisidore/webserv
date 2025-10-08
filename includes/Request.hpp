#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "webserv.hpp"

class	Request {

	private:

	// start-line
		std::string	_method;
		std::string	_request_target;
		std::string	_protocol;

	// Headers
		std::map<std::string, std::string>	_headers;

	// Body
		std::string	_body;

	// Status
		int	_status_code;

	public:

		Request(const std::string &str, const int &s_c);
		~Request();
};

#endif
