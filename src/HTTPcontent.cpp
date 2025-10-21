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

void	HTTPcontent::append_to_header(char buff[BUFF_SIZE], int bytes) {_current_header.append(buff, (size_t)bytes);}

void	HTTPcontent::setCurrentBody(std::string str) {_current_body = str;}

std::string	HTTPcontent::getCurrentBody() const {return _current_body;}


std::string		HTTPcontent::getCurrentHeader() const {return _current_header;}


unsigned long	HTTPcontent::getContentLength() const {return _content_length;}

void	HTTPcontent::setContentLength(unsigned long len) {_content_length = len;}

void	HTTPcontent::append_to_body(char buff[BUFF_SIZE], int bytes) {_current_body.append(buff, (size_t)bytes);}
