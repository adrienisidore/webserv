#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

class	Request {

	private:

	std::string		_message;

	// start-line
		std::string	_method;
		std::string	_request_target;//ressource demandee
		std::string	_protocol;

	// Headers
		std::map<std::string, std::string>	_headers;
		void								setHeaders(const std::string & message);

	// Body
		std::string	_body;

		int	_status_code;

	public:

		Request(const std::string &	message);
		~Request();

		std::string							getInfo(const std::string & which_info);
		int									getStatusCode();
		std::map<std::string, std::string>	getHeaders();

};

#endif
