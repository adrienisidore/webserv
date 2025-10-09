#include "webserv.hpp"

Client::Client(int fd): _fd(fd), _status_code(0) {

	_remainder = "";
}

Client::~Client() {
}

void	Client::start_new_message() {
	
	_current_message.clear();
	_request_start_time = time(NULL);
}

void	Client::read_data(char buff[BUFF_SIZE], int *bytes_received) {

	memset(buff, 0, BUFF_SIZE);
	*bytes_received = recv(_fd, buff, BUFF_SIZE, 0);
	_last_chunk_time = time(NULL);
	_current_message.append(buff);
}

std::string	Client::get_current_message() const {
	
	return _current_message;
}

int	Client::get_status_code() const {
	
	return _status_code;
}

bool	Client::header_complete(char buff[BUFF_SIZE], int bytes) {
	
	time_t	now = time(NULL);

	if (bytes == 0)
		return true;

	if (bytes < 0 && !buff[0])
		return true;

	size_t header_end = _current_message.find("\r\n\r\n");
	if (header_end != std::string::npos) {
		_remainder = _current_message.substr(header_end);
		return true;
	}
	if (now - _request_start_time > REQUEST_MAX_TIME || now - _last_chunk_time > CHUNK_MAX_TIME) {
		_status_code = 408 ;
		return true;
	}
	if (_current_message.size() > HEADER_MAX_SIZE) {
		_status_code = 431;
		return true;
	}
	return false;
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

