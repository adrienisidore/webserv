#include "webserv.hpp"

HTTPcontent::HTTPcontent() : _code(0), _protocol("HTTP/1.1") {}

//Potentiellement useless, uniquement par securite
void	HTTPcontent::reset(TCPConnection *connection) {
	_code = 0;
	_method.clear();
	_URI.clear();
	_headers.clear();
	_current_header.clear();
	_current_body.clear();
	_connection = connection;
	//a priori Locationconfig n'a pas besoin d'etre nettoye (a checker).
}

void	HTTPcontent::setLocation(const ServerConfig & servconfig) {

	_config = servconfig.getLocations().at(_URI);

}

// HTTPcontent	&HTTPcontent::operator=(const CGI &src) {
// 	_code = src.getCode();
// 	_config = src.getConfig();
// 	_headers = src.getHeaders();
// 	_method = src.getMethod();
// 	_URI = src.getURI();
// 	_content_length = src._content_length;
// 	_current_header = src._current_header;
// 	_current_body = src._current_body;
// }

void	HTTPcontent::setCode(const int & code) {_code = code;}

std::string	HTTPcontent::getMethod() const {return (_method);}

std::string	HTTPcontent::getURI() const {return (_URI);}

int	HTTPcontent::getCode() const {return (_code);}

std::map<std::string, std::string>	HTTPcontent::getHeaders() const {return (_headers);}

LocationConfig						HTTPcontent::getConfig() const { return (_config);}

HTTPcontent::~HTTPcontent() {}

void	HTTPcontent::append_to_header(char buff[BUFF_SIZE], int bytes) {_current_header.append(buff, (size_t)bytes);}

void	HTTPcontent::setCurrentBody(std::string str) {_current_body = str;}

std::string	HTTPcontent::getCurrentBody() const {return _current_body;}


std::string		HTTPcontent::getCurrentHeader() const {return _current_header;}


unsigned long	HTTPcontent::getContentLength() const {return _content_length;}

void	HTTPcontent::setContentLength(unsigned long len) {_content_length = len;}

void	HTTPcontent::append_to_body(char buff[BUFF_SIZE], int bytes) {_current_body.append(buff, (size_t)bytes);}
