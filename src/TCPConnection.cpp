#include "webserv.hpp"

TCPConnection::TCPConnection(int tcp_socket, ServerConfig config): _tcp_socket(tcp_socket), _config(config) {
	end_transfer();
}

TCPConnection::~TCPConnection() {}

//When transfer is done, or before waiting the data
void	TCPConnection::end_transfer() {
	_status = END;
	_header_start_time = 0;
	_last_tcp_chunk_time = 0;
	_body_start_time = 0;
	_end_of_request_time = time(NULL);
}

//When a new request arrives
void	TCPConnection::initialize_transfer() {
	
	_header_start_time = time(NULL);
	_end_of_request_time = 0;
	_body_start_time = 0;
	_status = READING_HEADER;
	_request.reset();
}

void	TCPConnection::check_body_headers() {

	if (_request.getHeaders().find("CONTENT-LENGTH") != _request.getHeaders().end()) {
		if (!is_valid_length(_request.getHeaders()["CONTENT-LENGTH"]))
			return set_error(413);
		else
			_request.setBodyProtocol(CONTENT_LENGTH);
	}

	else if (_request.getHeaders().find("TRANSFER-ENCODING") != _request.getHeaders().end() 
		&& _request.getHeaders()["TRANSFER-ENCODING"] == "chunked") {
		_request.setBodyProtocol(CHUNKED);
	}

	else
		return set_error(411);

	_status = READING_BODY;
}

void TCPConnection::use_recv() {

	memset(_buff, 0, BUFF_SIZE);
	_bytes_received = recv(_tcp_socket, _buff, BUFF_SIZE, 0);
	_last_tcp_chunk_time = time(NULL);

	if (_bytes_received == 0) {
		_status = CLIENT_DISCONNECTED;	// there is one more case right ?
		return;
	}
	else if (_bytes_received < 0)
		return set_error(500);
}

void	TCPConnection::read_header() {

	use_recv();

	// APPEND TO REQUEST CURRENT HEADER
	_request.append_to_header(_buff, _bytes_received);

	// std::cout << "Header: " << _request.getCurrentHeader() << std::endl;
	if (_request.getCurrentHeader().size() > HEADER_MAX_SIZE)
		return set_error(431);

	// CHECK IF END OF HEADER
	size_t header_end = _request.getCurrentHeader().find("\r\n\r\n");

	if (header_end != std::string::npos) {

		_request.parse_header();
		if (_request.getCode()) {
			_status = ERROR;
			return;
		}
		else if (_request.getMethod() == "POST") {
			_body_start_time = time(NULL);
			_header_start_time = 0;
			_request.setCurrentBody(_request.getCurrentHeader().substr(header_end));
			_status = WAIT_FOR_BODY;
			return;
		}
		else {
			_status = READ_COMPLETE;
			return;
		}
	}
}

void	TCPConnection::read_body() {
	
	use_recv();
	
	// APPEND TO REQUEST CURRENT BODY

	_request.append_to_body(_buff, BUFF_SIZE);		// THE READING

	// std::cout << "Body: " << _request.getCurrentBody() << std::endl;
	if (_request.getCurrentBody().size() > BODY_MAX_SIZE)
			return set_error(413);
	
	// CHECK IF END OF BODY
	if (_request.getBodyProtocol() == CHUNKED) {

		size_t header_end = _request.getCurrentBody().find("0\r\n\r\n");
		if (header_end != std::string::npos) {

			_request.unchunk_body();

			if (_request.getCode()) {
				_status = ERROR;
				return;
			}
			else {
				_status = READ_COMPLETE;
				return;
			}
		}
	}
	else if (_request.getBodyProtocol() == CONTENT_LENGTH) {

		int diff = _request.getCurrentBody().size() - _request.getContentLength();
		if (diff == 0) {
			_status = READ_COMPLETE;
			return;
		}
		else if (diff > 0)
			return set_error(400);
	}
	return;
}

void	TCPConnection::set_error(int error_code) {

	_status = ERROR;
	_request.setCode(error_code);
	return;
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

bool TCPConnection::is_valid_length(const std::string& content_length) {

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

	_request.setContentLength(length);
    return true;
}

int	TCPConnection::get_status() const {return _status;}

Request	TCPConnection::getRequest() const {return _request;}

int		TCPConnection::getTCPSocket() const {return _tcp_socket;}

time_t	TCPConnection::getHeaderTime() const {return _header_start_time;}

time_t	TCPConnection::getBodyTime() const {return _body_start_time;}

time_t	TCPConnection::getLastChunkTime() const {return _last_tcp_chunk_time;}

time_t	TCPConnection::getEndOfRequestTime() const {return _end_of_request_time;}
