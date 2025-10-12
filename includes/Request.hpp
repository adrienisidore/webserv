#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

//Create abstract class HTTPDataTransfer : has a code, methods ...

//what a client sent, using TCP
class	Request {

	private:
	//Status_code
		int		_code;
		
	// raw header
		std::string	_current_header; // track the header each TCP chunk, each time recv is called

	// start-line
		std::string	_method;
		std::string	_request_target;
		std::string	_protocol;
		void		setStartLine(const std::string & message);

	// Headers
		std::map<std::string, std::string>	_headers;
		void								setHeaders(const std::string & message);

	// Body
		std::string	_remainder; // first bytes of the body, if they've been send with last headers' bytes
		std::string	_body;//useless for now
		void		addBody(const std::string & message);//useless for now

		std::string		getParentDirectory(const std::string &path) const;
		void			checkAccessAndPermissions();

	public:
		Request();
		Request(const std::string &	message, const int & s_c);
		~Request();

		void								reset();
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
