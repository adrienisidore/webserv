#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

class	Request {

	private:
	//Status_code
		int		_status_code;
		void	setStatusCode(const int & st);

	// start-line
		std::string	_method;
		std::string	_request_target;
		std::string	_protocol;
		void		setStartLine(const std::string & message);

	// Headers
		std::map<std::string, std::string>	_headers;
		void								setHeaders(const std::string & message);

	// Body
		std::string	_body;
		void		setBody(const std::string & message);

	public:
		Request(const std::string &	message, const int & s_c);
		~Request();

		int									getStatusCode();
		std::string							getMethod();
		std::string							getRequestTarget();
		std::string							getProtocol();
		std::map<std::string, std::string>	getHeaders();
		std::string							getBody();

};

#endif
