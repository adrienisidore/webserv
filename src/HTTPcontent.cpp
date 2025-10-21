#include "webserv.hpp"

HTTPcontent::HTTPcontent() : _code(0), _protocol("HTTP/1.1") {}

//Peut etre recoder dans les heritants si besoin
// void	HTTPcontent::copyFrom(const HTTPcontent& other) {
//         _code = other.getCode();
//         _method = other.getMethod();
//         _URI = other.getURI();
// }

void	HTTPcontent::reset() {
	_code = 0;
	_method.clear();
	_URI.clear();
	_headers.clear();
}

void	HTTPcontent::setCode(const int & code) {
	_code = code;
}

std::string	HTTPcontent::getMethod() const {return (_method);}

std::string	HTTPcontent::getURI() const {return (_URI);}

std::string	HTTPcontent::getProtocol() const {return (_protocol);}

int	HTTPcontent::getCode() const {return (_code);}

std::map<std::string, std::string> HTTPcontent::getHeaders() const {return (_headers);}

HTTPcontent::~HTTPcontent() {}