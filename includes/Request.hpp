#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

//what a client sent, using TCP
class	Request : public HTTPcontent {

	private:
		
	// raw header
		std::string	_current_header; // track the header each TCP chunk, each time recv is called

		void		setStartLine(const std::string & message);

		void		setHeaders(const std::string & message);

	// Body
		std::string	_remainder; // first bytes of the body, if they've been send with last headers' bytes
		std::string	_body;//useless for now
		// void		addBody(const std::string & message);//useless for now

		std::string		parentDir(const std::string &path) const;
		void			checkPermissions();

	public:
		Request();
		Request(const std::string &	message, const int & code);
		~Request();

		void								reset();
		void								setCode(const int & code);

};

//display the info
std::ostream&	operator<<(std::ostream& os, const Request &request);

#endif
