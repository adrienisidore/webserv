#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

//formalize a proper request
class	Request : public HTTPcontent {

	private:
		
		void			setStartLine();
		void			setHeaders();

	public:
		Request();
		virtual ~Request();
		
		void			parse_header();
		void			unchunk_body();

		void			copyFrom(HTTPcontent& other);
;



};

//display the info
std::ostream&	operator<<(std::ostream& os, const Request &request);

#endif
