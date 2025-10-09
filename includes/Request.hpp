#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

//what a client sent, using TCP
class	Request {

	private:
	//Status_code
		int		_status_code;
		

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
		void		addBody(const std::string & message);

	public:
		Request(const std::string &	message, const int & s_c);
		~Request();

		void								setStatusCode(const int & st);
		int									getStatusCode() const;
		std::string							getMethod() const;
		std::string							getRequestTarget() const;
		std::string							getProtocol() const;
		std::map<std::string, std::string>	getHeaders() const;
		std::string							getBody() const;

};

//display the info
std::ostream&	operator<<(std::ostream& os, const Request &request);

#endif
