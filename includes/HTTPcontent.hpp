#ifndef HTTPCONTENT_HPP
# define HTTPCONTENT_HPP

# include "./webserv.hpp"

//Common elements between a Request and a Response sent throug a TCPConnection.
//This abstract class only contains getters
class HTTPcontent {

	protected:

		int		_code;

	// start-line
		std::string	_method;
		std::string	_target;//_request_target
		const std::string	_protocol;

	// Headers
		std::map<std::string, std::string>	_headers;//ne se transfert pas avec <<

		HTTPcontent();
		HTTPcontent(const int & code);
		~HTTPcontent();

		public:

		void								setCode(const int & code);
		void 								reset();


		virtual void						copyFrom(const HTTPcontent& other) = 0;
		int									getCode() const;//getStatusCode
		std::string							getMethod() const;
		std::string							getTarget() const;//getRequestTarget
		std::string							getProtocol() const;
		std::map<std::string, std::string>	getHeaders() const;

};

#endif