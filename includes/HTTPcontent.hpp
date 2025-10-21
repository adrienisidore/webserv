#ifndef HTTPCONTENT_HPP
# define HTTPCONTENT_HPP

# include "./webserv.hpp"

//Common elements between a Request and a Response sent through a TCPConnection.
//All the data that goes from Request to Response
class HTTPcontent {

	protected:

		int					_code;
		LocationConfig		_config;//A FINIR

	// start-line
		std::string			_method;
		std::string			_URI;
		const std::string	_protocol;

	// Headers
		std::map<std::string, std::string>	_headers;
	
	//Response :
		std::string			_path;

		HTTPcontent();
		~HTTPcontent();

		public:

		void								setCode(const int & code);
		void 								reset();

		virtual void						copyFrom(const HTTPcontent& other) = 0;//Doit etre code dans Request et Response
		int									getCode() const;
		std::string							getMethod() const;
		std::string							getURI() const;
		std::string							getProtocol() const;
		std::map<std::string, std::string>	getHeaders() const;

};

#endif