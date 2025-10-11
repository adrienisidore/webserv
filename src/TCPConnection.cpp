#include "webserv.hpp"

TCPConnection::TCPConnection(int tcp_socket): _tcp_socket(tcp_socket), _status_code(0) {

	_remainder = "";
}

TCPConnection::~TCPConnection() {
}

void	TCPConnection::start_new_message() {
	
	_current_header.clear();
	_header_start_time = time(NULL);
}

void	TCPConnection::start_new_body() {

	_current_header.clear();
	_body_start_time = time(NULL);
}

void	TCPConnection::append_to_header(char *buff) {
	_current_header.append(buff);
}

void	TCPConnection::read_data(char buff[BUFF_SIZE], int *bytes_received) {

	memset(buff, 0, BUFF_SIZE);
	*bytes_received = recv(_tcp_socket, buff, BUFF_SIZE, 0);
	_last_tcp_chunk_time = time(NULL);
}

void	TCPConnection::parse_http_chunk(char buff[BUFF_SIZE], int bytes) {

	if (bytes == 0 || (bytes < 0 && buff[0]))
		return ;
	
}

// besoin d'une nouvelle fonction ? pas sur
// http chunks are usefull for streaming -> sending data before you have it all
// ex: video streaming, infinite scrolling (I think)
// BUT in our case, our usecase for POST is that the client can upload large files, or CGIs
// -> In this case, the chunk-by-chunk approach is good, because the server can store the data step by step on a file rather than the RAM, avoid DOS attacks, and more...
//
// What are the differences between chunked and the rest ?
// -> For content-length, we put everything into a string

std::string	TCPConnection::get_current_header() const {
	
	return _current_header;
}

int	TCPConnection::get_status_code() const {
	
	return _status_code;
}

int	TCPConnection::header_complete(char buff[BUFF_SIZE], int bytes)
{
	
	time_t	now = time(NULL);

	// client souhaite partir ou data transfer failed : on deconnecte le client
	if (bytes == 0) {
		return 1;
	}

	if (bytes < 0 && buff[0]) {
		return 1;
	}

	//le client prend trop de temps, ou envoie trop de donnees : on renvoie une reponse
	if (now - _header_start_time > REQUEST_MAX_TIME || now - _last_tcp_chunk_time > CHUNK_MAX_TIME) {
		_status_code = 408 ;
		return 2;
	}

	if (_current_header.size() > HEADER_MAX_SIZE) {
		_status_code = 431;
		return 2;
	}

	size_t header_end = _current_header.find("\r\n\r\n");
	if (header_end != std::string::npos) {
		_remainder = _current_header.substr(header_end);
		return 2;//le header est complet, tout s'est bioen passe : on renvoie une reponse
	}

	return 0; // on continue a lire
}

int	TCPConnection::body_length_complete(char buff[BUFF_SIZE], int bytes, int bytes_written, unsigned long len, unsigned long max_len) {

	time_t	now = time(NULL);

	// client souhaite partir ou data transfer failed : on deconnecte le client
	if (bytes == 0 || bytes_written == 0) {
		return 1;
	}

	if ((bytes < 0 && buff[0]) || bytes_written < 0) {
		return 1;
	}

	//le client prend trop de temps, ou envoie trop de donnees : on renvoie une reponse
	if (now - _header_start_time > REQUEST_MAX_TIME || now - _last_tcp_chunk_time > CHUNK_MAX_TIME) {
		_status_code = 408 ;
		return 2;
	}

	if (len > max_len || len > BODY_MAX_SIZE) {
		_status_code = 431;
		return 2;
	}
	return 0;
}
	/*
	
	IF bytes < 0:
		-http error
		-buffer is empty but client is still conn work with POST and ected 

	 
	HOW TO DETECT END OF HEADER
	-> \r\n\	GET

	HOW TO DETECT END OF BODY ?
	if Transfer-Encoding: Chunked -> end of body by a chunk with a length of 0 (still /r/n/r/n ?)	
	if Content-Length -> end of body when length is read

	else-> status code 400 or 411 (length required)



	1. Read headers until \r\n\r\n
	2. Store the potential reminder (after the \r\n\r\n)
	3. If POST -> check Content-Length or Chunk-encoded
	4. Parse potential chunk 

	*/
	// 408 timeout
	// 431 headers too large
	// 413 body too large / total request too large
	// 0 request complete
	// what is the error code ?
	// overall timeout -> 30s
	// between chinks timeout -> 50s
	// 
	// \r\n\r\n -> end of header ?

