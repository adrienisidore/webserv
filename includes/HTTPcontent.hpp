#ifndef HTTPCONTENT_HPP
# define HTTPCONTENT_HPP

# include "./webserv.hpp"

class TCPConnection;
//Request/Response :
//All the data that goes from Request to Response
class HTTPcontent {

	protected:

		int					_code;
		LocationConfig		_config;

	// lecture
		std::string			_current_header; // track the header each TCP chunk, each time recv is called
		std::string			_current_body;

		unsigned long		_content_length; // obligatoire pour response mais facultative pout request

	// start-line
		std::string			_method;
		std::string			_URI;
		const std::string	_protocol;

	// Headers
		std::map<std::string, std::string>	_headers;
	
	//Response :
		std::string			_path;

		HTTPcontent();
		virtual ~HTTPcontent();

		public:

		//AJOUTER un pointeur vers sa TCPConnection : permet a partir du CGI (ou Response ou Request) de savoir a quelle TCPConnection il appartient

		TCPConnection						*_connection;

		//HTTPcontent							&operator=(const CGI &);
		void								setLocation(const ServerConfig & servconfig);

		void								setCode(const int & code);
		void 								reset(TCPConnection *connection);

		virtual void						copyFrom(HTTPcontent& other) = 0;
		int									getCode() const;
		std::string							getMethod() const;
		std::string							getURI() const;
		std::map<std::string, std::string>	getHeaders() const;
		LocationConfig						getConfig() const;

		void								setContentLength(unsigned long);
		unsigned long						getContentLength() const;
		void								setCurrentBody(std::string str);
		std::string							getCurrentBody() const;
		std::string							getCurrentHeader() const;
		void								append_to_header(char buff[BUFF_SIZE], int bytes);
		void								append_to_body(char buff[BUFF_SIZE], int bytes);


};

#endif
