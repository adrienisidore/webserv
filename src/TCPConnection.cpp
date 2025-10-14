#include "webserv.hpp"

TCPConnection::TCPConnection(int tcp_socket): _tcp_socket(tcp_socket) {
	end_request();
}

void	TCPConnection::end_request() {
	_status = END;
	_header_start_time = 0;
	_last_tcp_chunk_time = 0;
	_body_start_time = 0;
	_end_of_request_time = time(NULL);
}

TCPConnection::~TCPConnection() {
}

Request	*TCPConnection::start_new_request() {
	
	_header_start_time = time(NULL);
	_end_of_request_time = 0;
	_body_start_time = 0;
	_status = READING_HEADER;
	_request.reset();
	return &_request;
}

void	TCPConnection::read_header() {

	// READ FROM RECV
	memset(_buff, 0, BUFF_SIZE);
	_bytes_received = recv(_tcp_socket, _buff, BUFF_SIZE, 0);
	_last_tcp_chunk_time = time(NULL);

	if (_bytes_received == 0) {
		_status = CLIENT_DISCONNECTED;	// there is one more case right ?
		return;
	}
	else if (_bytes_received < 0) {
		_status = READ_ERROR; // what is this error ?
		return;
	}

	// APPEND TO REQUEST CURRENT HEADER
	_request.append_to_header(_buff, _bytes_received);

	std::cout << "Header: " << _request.getCurrentHeader() << std::endl;
	if (_request.getCurrentHeader().size() > HEADER_MAX_SIZE) {
		_status = HEADER_TOO_LARGE; //_code = 431;
		return;
	}

	// CHECK IF END OF HEADER
	size_t header_end = _request.getCurrentHeader().find("\r\n\r\n");

	if (header_end != std::string::npos) {

		_request.parse_header();
		
		if (_request.getMethod() == "POST") { // OU PUT
			_body_start_time = time(NULL);
			_header_start_time = 0;
			_body_buff = _request.getCurrentHeader().substr(header_end);
			_status = READING_BODY;
		}
		else
			_status = READ_COMPLETE;
	}
}

std::string	get_time_stamp() {

	time_t	t = time(NULL);
	tm *now = localtime(&t);
	std::ostringstream oss;

	oss << (now->tm_year + 1900)
		<< (now->tm_mon + 1)
		<< now->tm_mday << "_"
		<< now->tm_hour
		<< now->tm_min
		<< now->tm_sec;

	return oss.str();
}

bool is_valid_length(const std::string& content_length) {

	if (content_length.empty())
        return false;
    
    for (size_t i = 0; i < content_length.length(); i++) {
        if (!std::isdigit(static_cast<unsigned char>(content_length[i])))
            return false;
    }
    
    if (content_length.length() > 1 && content_length[0] == '0')
        return false;
    
    char* endptr;
    errno = 0;
    unsigned long length = std::strtoul(content_length.c_str(), &endptr, 10);
    
    if (errno == ERANGE || *endptr != '\0')
        return false;
    
    if (length > BODY_MAX_SIZE)
        return false;
    
    return true;
}

void	TCPConnection::read_body() {

	std::string	filename;
	int			fd_write;

	// THE ACTUAL READING
	
	memset(_buff, 0, BUFF_SIZE);
	_bytes_received = recv(_tcp_socket, _buff, BUFF_SIZE, 0);
	_last_tcp_chunk_time = time(NULL);

	if (_bytes_received == 0) {
		_status = CLIENT_DISCONNECTED;	// there is one more case right ?
		return;
	}
	else if (_bytes_received < 0) {
		_status = READ_ERROR; // what is this error ?
		return;
	}

	// INITIALISE WRITING PART

	if (_request.getHeaders().find("NAME") != _request.getHeaders().end())
		filename = _request.getHeaders()["NAME"];
	else
		filename = get_time_stamp(); 

	fd_write = open(("./ressources/" + filename).c_str(), O_WRONLY, O_CREAT); // POST ou PUT ?
	
	if (fd_write == -1) {
		_status = FORBIDDEN_REQUEST;	
		return;
	}

	// WRITE TO FILE

	_body_buff.append(_buff);

	if (_request.getHeaders().find("TRANSFER-ENCODING") != _request.getHeaders().end() 
		&& _request.getHeaders()["TRANSFER-ENCODING"] == "chunked") {
		write_body_chunked();
	}

	else if (_request.getHeaders().find("CONTENT-LENGTH") != _request.getHeaders().end()
		&& is_valid_length(_request.getHeaders()["CONTENT-LENGTH"])) {
		write_body_length();
	}
	
	else 
		_status = LENGTH_REQUIRED;

	return;
}

void	TCPConnection::write_body_chunked() {

}

void	TCPConnection::write_body_length() {

}

int	TCPConnection::get_status() const {
	
	return _status;
}

void	TCPConnection::set_status(int status) {
	_status = status;
}

Request	TCPConnection::getRequest() const {
	return _request;
}

int		TCPConnection::getTCPSocket() const {
	return _tcp_socket;
}

time_t	TCPConnection::getHeaderTime() const {
	return _header_start_time;
}

time_t	TCPConnection::getBodyTime() const {
	return _body_start_time;
}

time_t	TCPConnection::getLastChunkTime() const {
	return _last_tcp_chunk_time;
}

time_t	TCPConnection::getEndOfRequestTime() const {
		return _end_of_request_time;
}

// besoin d'une nouvelle fonction ? pas sur
// http chunks are usefull for streaming -> sending data before you have it all
// ex: video streaming, infinite scrolling (I think)
// BUT in our case, our usecase for POST is that the client can upload large files, or CGIs
// -> In this case, the chunk-by-chunk approach is good, because the server can store the data step by step on a file rather than the RAM, avoid DOS attacks, and more...
//
// What are the differences between chunked and the rest ?
// -> For content-length, we put everything into a string

/*
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
*/
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

