#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "./webserv.hpp"

//what a client sent, using TCP
class	Request : public HTTPcontent {

	private:
		
	// raw header
		std::string		_current_header; // track the header each TCP chunk, each time recv is called
		std::string		_current_body;
		int				_body_protocol; // is content_length precised, or is it a chunked body
		unsigned long	_content_length;

		void		setStartLine();
		void		setHeaders();

		std::string		parentDir(const std::string &path) const;
		void			checkPermissions();

	public:
		Request();
		~Request();
		
		void			parse_header();
		void			unchunk_body();

		void			copyFrom(const HTTPcontent& other);

		void			append_to_header(char buff[BUFF_SIZE], int bytes);
		std::string		getCurrentHeader() const;

		void			append_to_body(char buff[BUFF_SIZE], int bytes);
		void			setCurrentBody(std::string str);
		std::string		getCurrentBody() const;

		int				getBodyProtocol() const;
		unsigned long	getContentLength() const;

		void			setBodyProtocol(int);
		void			setContentLength(unsigned long);

};

//display the info
std::ostream&	operator<<(std::ostream& os, const Request &request);

#endif
