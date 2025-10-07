#include "./webserv.hpp"

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

		Request();

	public:

		Request(std::string	message);
		~Request();

		std::string	getMethod();
		std::string	getRequestTarget();
		std::string	getProtocol();
		std::map<std::string, std::string> getHeaders();
		std::string	getBody();
};
