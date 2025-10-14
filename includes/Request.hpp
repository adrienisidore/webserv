#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

//what a client sent, using TCP
class	Request : public HTTPcontent {

	private:
		
	// raw header
		std::string	_current_header; // track the header each TCP chunk, each time recv is called

		void		setStartLine();
		void		setHeaders();

		std::string		parentDir(const std::string &path) const;
		void			checkPermissions();

	public:
		Request();
		~Request();
		
		void		parse_header();

		void			copyFrom(const HTTPcontent& other);

		void			append_to_header(char buff[BUFF_SIZE], int bytes);
		std::string		getCurrentHeader() const;

};

//display the info
std::ostream&	operator<<(std::ostream& os, const Request &request);

#endif
