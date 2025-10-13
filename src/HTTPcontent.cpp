#include "webserv.hpp"

HTTPcontent::HTTPcontent() : _code(0), _protocol("HTTP/1.1") {}

HTTPcontent::HTTPcontent(const int & code) : _code(code), _protocol("HTTP/1.1") {}

//response.copyFrom(request);
//Peut etre recoder dans les heritants si besoin
void	HTTPcontent::copyFrom(const HTTPcontent& other) {
        _code = other._code;
        _method = other._method;
        _target = other._target;
}

std::string	HTTPcontent::getMethod() const {return (_method);}

std::string	HTTPcontent::getTarget() const {return (_target);}

std::string	HTTPcontent::getProtocol() const {return (_protocol);}

int	HTTPcontent::getCode() const {return (_code);}

std::map<std::string, std::string> HTTPcontent::getHeaders() const {return (_headers);}

std::string	HTTPcontent::getBody() const {return (_protocol);}//useless ?

HTTPcontent::~HTTPcontent() {}