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

	// Headers
		std::map<std::string, std::string>	_headers;

	// Body
		std::string	_body;//useless for now
		void		addBody(const std::string & message);//useless for now

		std::string		getParentDirectory(const std::string &path) const;
		void			checkAccessAndPermissions();

	public:
		Request();
		Request(const std::string &	message, const int & s_c);
		~Request();

		void		parse_header();
		void		setStartLine();
		void		setHeaders();
		void		append_to_header(char buff[BUFF_SIZE], int bytes);

		void								reset();
		void								setStatusCode(const int & st);
		int									getStatusCode() const;
		std::string							getMethod() const;
		std::string							getRequestTarget() const;
		std::string							getProtocol() const;
		std::string							getCurrentHeader() const;
		std::map<std::string, std::string>	getHeaders() const;
		std::string							getBody() const;
		std::string							getRemainder() const;

};

//display the info
std::ostream&	operator<<(std::ostream& os, const Request &request);

#endif
