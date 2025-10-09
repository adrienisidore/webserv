#include "webserv.hpp"

TCPConnection::TCPConnection(int tcp_socket): _tcp_socket(tcp_socket), _status_code(0) {

	_remainder = "";
}

TCPConnection::~TCPConnection() {
}

void	TCPConnection::start_new_message() {
	
	_current_message.clear();
	_header_start_time = time(NULL);
}

void	TCPConnection::start_new_body() {

	_current_message.clear();
	_body_start_time = time(NULL);
}

void	TCPConnection::read_data(char buff[BUFF_SIZE], int *bytes_received) {

	memset(buff, 0, BUFF_SIZE);
	*bytes_received = recv(_tcp_socket, buff, BUFF_SIZE, 0);
	_last_tcp_chunk_time = time(NULL);
	_current_message.append(buff);
}

// besoin d'une nouvelle fonction ? pas sur
// http chunks are usefull for streaming -> sending data before you have it all
// ex: video streaming, infinite scrolling (I think)
// BUT in our case, our usecase for POST is that the client can upload large files, or CGIs
// -> In this case, the chunk-by-chunk approach is good, because the server can store the data step by step on a file rather than the RAM, avoid DOS attacks, and more...
//

void	TCPConnection::read_data_chunked(char buff[BUFF_SIZE], int *bytes_received) {
	memset(buff, 0, BUFF_SIZE);
	// ...
}

std::string	TCPConnection::get_current_message() const {
	
	return _current_message;
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
	if (now - _request_start_time > REQUEST_MAX_TIME || now - _last_tcp_chunk_time > CHUNK_MAX_TIME) {
		_status_code = 408 ;
		return 2;
	}

	if (_current_message.size() > HEADER_MAX_SIZE) {
		_status_code = 431;
		return 2;
	}

	size_t header_end = _current_message.find("\r\n\r\n");
	if (header_end != std::string::npos) {
		_remainder = _current_message.substr(header_end);
		return 2;//le header est complet, tout s'est bioen passe : on renvoie une reponse
	}

	return 0; // on continue a lire
}
	/*
	
	IF bytes < 0:
		-http error
		-buffer is empty but client is still connected 

	 
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

