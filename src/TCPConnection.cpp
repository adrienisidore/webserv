#include "webserv.hpp"


TCPConnection::TCPConnection(int fd, const ServerConfig &config, const struct sockaddr_storage &addr, socklen_t addr_len)
	: _tcp_socket(fd), _config(config), _client_addr(addr), _client_addr_len(addr_len)
{
	_header_max_time = ret_time_directive("request_header_timeout", HEADER_MAX_TIME);
	_body_max_time = ret_time_directive("request_body_timeout", BODY_MAX_TIME);
	_between_chunks_max_time = ret_time_directive("send_timeout", BETWEEN_CHUNK_MAX_TIME);
	_no_request_max_time = ret_time_directive("keepalive_timeout", NO_REQUEST_MAX_TIME);
	_cgi_max_time = ret_time_directive("cgi_timeout", CGI_TIMEOUT);

	end_transfer();
}

TCPConnection::~TCPConnection() {}

time_t TCPConnection::ret_time_directive(std::string directive_name, int defaul) const {

	std::string	directive_str = _config.getDirective(directive_name);
	if (!directive_str.empty()) {
		time_t	time = std::atoi(directive_str.c_str());
		if (time > 0 && time <= defaul)
			return time;
	}
	return defaul; 
}

//When transfer is done, or before waiting the data
void	TCPConnection::end_transfer() {
	_status = END;
	_header_start_time = 0;
	_last_tcp_chunk_time = 0;
	_body_start_time = 0;
	_cgi_start_time = 0;
	_end_of_request_time = time(NULL);
}

//When a new request arrives
void	TCPConnection::initialize_transfer() {
	
	_header_start_time = time(NULL);
	_end_of_request_time = 0;
	_body_start_time = 0;
	_cgi_start_time = 0;
	_status = READING_HEADER;
	_request.reset(this);
	_response.reset(this);
	_response._cgi.reset(this);

}

void	TCPConnection::check_body_headers() {

	std::map<std::string, std::string>	headers = _request.getHeaders();

	if (headers.find("CONTENT-LENGTH") != headers.end()) {
		if (!is_valid_length(headers["CONTENT-LENGTH"]))
			return set_error(413);
		else {
			_request.setContentLength(std::atol(headers["CONTENT-LENGTH"].c_str()));
			setBodyProtocol(CONTENT_LENGTH);
		}
	}

	else if (headers.find("TRANSFER-ENCODING") != headers.end() 
		&& headers["TRANSFER-ENCODING"] == "chunked") {
		setBodyProtocol(CHUNKED);
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
		_status = CLIENT_DISCONNECTED;
		return;
	}
	else if (_bytes_received < 0) {
		_status = CLIENT_DISCONNECTED;
		return set_error(500);
	}
}

void	TCPConnection::read_header() {

	unsigned long	max_size;

	use_recv();

	if (_status == CLIENT_DISCONNECTED || _status == READ_COMPLETE)
        return;
	_request.append_to_header(_buff, _bytes_received);

	std::string max_size_str = _config.getDirective("client_header_buffer_size");
	if (max_size_str.empty())
		max_size = HEADER_MAX_SIZE;
	else {
		max_size = std::atol(max_size_str.c_str());
		if (max_size <= 0)
			max_size = HEADER_MAX_SIZE;
	}

	if (max_size < _request.getCurrentHeader().size())
		return set_error(431);

	size_t header_end = _request.getCurrentHeader().find("\r\n\r\n");

	if (header_end != std::string::npos) {

		_request.parse_header();
		
		if (_request.getCode()) {
			_status = READ_COMPLETE;
			return;
		}
		_request.setLocation(_config);
		
		if (_request.getMethod() == "POST" && !_request.getCode()) {
			_body_start_time = time(NULL);
			_header_start_time = 0;
			_request.setCurrentBody(_request.getCurrentHeader().substr(header_end + 4));
			_status = WAIT_FOR_BODY;
			return;
		}
		else {
			_status = READ_COMPLETE;
			return;
		}
	}
}

void	TCPConnection::read_body(bool state_changed) {
	
	// state_changed is necessary to prevent from reading twice on the same call:
	// 1 time the body and 1 time the body when the request hasn't sent the body yet 
	// -> only read from the network once per poll cycle
	if (!state_changed) {

		use_recv();

		if (_status == CLIENT_DISCONNECTED || _status == READ_COMPLETE)
			return;

		_request.append_to_body(_buff, _bytes_received);
		}

	double	max_size;
	
	std::string max_size_str = _config.getDirective("client_max_body_size");
	if (max_size_str.empty())
		max_size = BODY_MAX_SIZE;
	else {
		max_size = std::atol(max_size_str.c_str());
		if (max_size <= 0)
			max_size = BODY_MAX_SIZE;
	}

	if (max_size < _request.getCurrentBody().size())
		return set_error(413);
	
	// CHECK IF END OF BODY
	if (getBodyProtocol() == CHUNKED) {

		size_t body_end = _request.getCurrentBody().find("0\r\n\r\n");
		if (body_end != std::string::npos) {

			_request.unchunk_body();
			_status = READ_COMPLETE;
			return;
		}
	}

	else if (getBodyProtocol() == CONTENT_LENGTH) {

		int diff = _request.getCurrentBody().size() - _request.getContentLength();
		if (diff >= 0) {
			_status = READ_COMPLETE;
			_request.setCurrentBody(_request.getCurrentBody().substr(0, _request.getContentLength()));
			return;
		}
	}
	return;
}

void	TCPConnection::execute_method() {

	int	poll_cgi;

	_response.copyFrom(_request);

	_header_start_time = 0;
	_body_start_time = 0;
	_last_tcp_chunk_time = 0;

	poll_cgi = _response.fetch();
	// check compatibilite entre location config et request
	if (poll_cgi) {

		_status = NOT_READY_TO_SEND;

		try {
			_response._cgi.execute_cgi();
			_map_cgi_fds_to_add.insert(std::pair<int, CGI>(poll_cgi, _response._cgi));  
			_cgi_start_time = time(NULL);
			return;
		} 
		catch (std::exception &er) {
			close(poll_cgi);
			_map_cgi_fds_to_add.erase(poll_cgi);
			_response.setCode(500);
			_response._error_();
			_status = READY_TO_SEND;
		}
	}
	
	// Not a cgi -> execute right now 
	_response.execute();
	_status = READY_TO_SEND;
	return;
}

void	TCPConnection::set_error(int error_code) {

	_status = READ_COMPLETE;
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
	
	double	max_size;

	std::string max_size_str = _config.getDirective("client_max_body_size");
	if (max_size_str.empty())
		max_size = BODY_MAX_SIZE;
	else {
		max_size = std::atol(max_size_str.c_str());
		if (max_size <= 0)
			max_size = BODY_MAX_SIZE;
	}

	if (length > max_size)
		return false;

    return true;
}

void	TCPConnection::setStatus(int status) {_status = status;}

int	TCPConnection::get_status() const {return _status;}

Request	TCPConnection::getRequest() const {return _request;}

Response TCPConnection::getResponse() const {return _response;}

int		TCPConnection::getTCPSocket() const {return _tcp_socket;}

time_t	TCPConnection::getHeaderTime() const {return _header_start_time;}

time_t	TCPConnection::getBodyTime() const {return _body_start_time;}

time_t	TCPConnection::getLastChunkTime() const {return _last_tcp_chunk_time;}

time_t	TCPConnection::getEndOfRequestTime() const {return _end_of_request_time;}

time_t	TCPConnection::getCGITime() const {return _cgi_start_time;}

int		TCPConnection::getBodyProtocol() const {return _body_protocol;}

void	TCPConnection::setBodyProtocol(int protocol) {_body_protocol = protocol;}

time_t	TCPConnection::getHeaderMaxTime() const {return _header_max_time;}

time_t	TCPConnection::getBodyMaxTime() const {return _body_max_time;}

time_t	TCPConnection::getBetweenChunksMaxTime() const {return _between_chunks_max_time;}

time_t	TCPConnection::getNoRequestMaxTime() const {return _no_request_max_time;}

time_t	TCPConnection::getCGIMaxTime() const {return _cgi_max_time;}
