#ifndef AHTTPTRANSFER_HPP
# define AHTTPTRANSFER_HPP

# include "./webserv.hpp"

//Create abstract class HTTPDataTransfer : has a code, methods ...
//_code, _headers, _method, _request_target, _protocol tous les get

class AHTTPtransfer {

	protected:

		int		_code;

	// start-line
		std::string	_method;
		std::string	_target;//_request_target
		std::string	_protocol;

	// Headers
		std::map<std::string, std::string>	_headers;//ne se transfert pas avec <<

	// Body
		std::string	_body;//ne se transfert pas avec <<

		AHTTPtransfer();
		AHTTPtransfer(int & code);
		~AHTTPtransfer();

		public:

		virtual void						copyFrom(const AHTTPtransfer& other) = 0;
		int									getCode() const;//getStatusCode
		std::string							getMethod() const;
		std::string							getTarget() const;//getRequestTarget
		std::string							getProtocol() const;
		std::map<std::string, std::string>	getHeaders() const;
		std::string							getBody() const;
	
};

AHTTPtransfer&	operator<<(AHTTPtransfer& response, const AHTTPtransfer &request);

#endif