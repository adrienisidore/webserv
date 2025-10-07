#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

class	Request {

	private:

	// start-line
		std::string	_method;
		std::string	_request_target;//ressource demandee
		std::string	_protocol;

	// Headers
		std::map<std::string, std::string>	_headers;

	// Body
		std::string	_body;

	// Status : une variable qui devrait etre propre a Request et response ? On peut
	//On peut faire deriver Request et Response d'une Interface Message.
	//Dans Message il faut aussi forcement un body meme s'il reste vide (requetes sans body).
	//Idem pour header ?
	//Idem pour protocol
		int	_status_code;

		Request();

	public:

		Request(const std::string &	message);
		~Request();

		std::string							getInfo(const std::string & which_info);
		std::map<std::string, std::string>	getHeaders();

};

std::ostream&	operator<<(std::ostream& os, const Request &request);

#endif
