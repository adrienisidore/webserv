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

		Request(const std::string &	message);
		~Request();

		std::string							getInfo(const std::string & which_info);
		std::map<std::string, std::string>	getHeaders();

};
