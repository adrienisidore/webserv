#include "webserv.hpp"

TCPConnection::TCPConnection(int tcp_socket, ServerConfig config): _tcp_socket(tcp_socket), _config(config) {
	end_transfer();//A quoi ca sert ?
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
	_request.reset(this);
	_response.reset(this);
	_response._cgi.reset(this);

}

void	TCPConnection::check_body_headers() {

	if (_request.getHeaders().find("CONTENT-LENGTH") != _request.getHeaders().end()) {
		if (!is_valid_length(_request.getHeaders()["CONTENT-LENGTH"]))
			return set_error(413);
		else
			setBodyProtocol(CONTENT_LENGTH);
	}

	else if (_request.getHeaders().find("TRANSFER-ENCODING") != _request.getHeaders().end() 
		&& _request.getHeaders()["TRANSFER-ENCODING"] == "chunked") {
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
		_status = CLIENT_DISCONNECTED;	// there is one more case right ?
		return;
	}
	else if (_bytes_received < 0)
		return set_error(500);
}

//On reutilise les attributs _buff, _bytes_received ...
// void TCPConnection::use_recv2() {

// 	memset(_buff, 0, BUFF_SIZE);
// 	_bytes_received = read(outpipe, _buff, BUFF_SIZE);//outpipe : d'ou vient la data du CGI
// 	_last_tcp_chunk_time = time(NULL);

// 	// Veut dire que le CGI a finit d'ecrire (checker si c'est ca ou si y'a EOF)
// 	if (_bytes_received == 0) {
// 		_status = ;	
// 		return;
// 	}
// 	else if (_bytes_received < 0)
// 		return set_error(500);
// }

void	TCPConnection::read_header() {

	use_recv();

	if (_status == CLIENT_DISCONNECTED || _status == ERROR)
        return;
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
		_request.setLocation(_config);
		if (_request.getMethod() == "POST") {
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
	if (getBodyProtocol() == CHUNKED) {

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
	else if (getBodyProtocol() == CONTENT_LENGTH) {

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


// execute la method puis construit la reponse -> plus qu'a l'envoyer
void	TCPConnection::execute_method() {

	int	poll_cgi;

	_response.copyFrom(_request);

	// Probleme : quelque soit la methode on execute le CGI de la meme maniere (mammouth)

	poll_cgi = _response.fetch();// check compatibilite entre location config et request
	if (poll_cgi) {

		_status = NOT_READY_TO_SEND;
		ServerMonitor::_instance->add_new_cgi_socket(poll_cgi, _response._cgi);

		try {
			_response._cgi.launchExecve();
			return;
		} 
		catch (std::exception &er) {

			ServerMonitor::_instance->close_cgi_fd(poll_cgi);
			_response.setCode(500);
		}
	}

	// POST ou DELETE : pour POST et DELETE on effectue une action
	if (_response.getMethod() == "POST" && !_response.getCode())
		_response._post_();
	else if (_response.getMethod() == "DELETE" && !_response.getCode())
		_response._delete_();
	// ICI : toutes les actions ont ete executes, POST et DELETE on preremplie le _body
	// il reste plus qu'a GET ou ERROR et ajouter la startLine (buildResponse : HTTP/1.1 200 ok)
	else if (_response.getMethod() == "GET" && !_response.getCode())
		_response._get_();
	if (_response.getCode())
		_response._error_();
	
	_status = READY_TO_SEND;
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

//  send_response();
void TCPConnection::send_response(void)
{
    struct stat fileStat;
    if (stat("../ressources/ServerInterface.html", &fileStat) == -1) return;//Envoyer code d'erreur serveur
    int fileSize = fileStat.st_size;

    int fd = open("../ressources/ServerInterface.html", O_RDONLY);
    if (fd == -1) return;//Il faudra envoyer le bon code d'erreur 4xx/5xx si problème

    // Charger fichier
    char *fileContent = new char[fileSize];
    read(fd, fileContent, fileSize);
    close(fd);

    std::ostringstream string_fileSize;
    string_fileSize << fileSize;
    std::string header = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + string_fileSize.str() + "\r\n"
        "Connection: keep-alive\r\n\r\n";

    int responseSize = strlen(header.c_str()) + fileSize;//Fonction interdite
    char *response = new char[responseSize];
    memcpy(response, header.c_str(), strlen(header.c_str()));//Fonction interdite
    memcpy(response + strlen(header.c_str()), fileContent, fileSize);//Fonction interdite

    std::cout << "Server's response: " << response << std::endl;

    send(_tcp_socket, response, responseSize, 0);

    delete[] fileContent;
    delete[] response;
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

int		TCPConnection::getBodyProtocol() const {return _body_protocol;}

void	TCPConnection::setBodyProtocol(int protocol) {_body_protocol = protocol;}
