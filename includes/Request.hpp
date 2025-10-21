#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

//what a client sent, using TCP
class	Request : public HTTPcontent {

	private:
		
		void			setStartLine();
		void			setHeaders();

	public:
		Request();
		~Request();
		
		void			parse_header();
		void			unchunk_body();

		void			copyFrom(const HTTPcontent& other);
;



};

//display the info
std::ostream&	operator<<(std::ostream& os, const Request &request);

#endif
