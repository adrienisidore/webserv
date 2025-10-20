#include "webserv.hpp"

Server	*Server::_instance = NULL;

Server::Server(std::string port)
: _listening(-1), _port(port), _is_running(false), _ressources_path("./ressources")
{
	create_server_socket();
	bind_server_socket();
}

Server::~Server() {
	stop();
}

void	Server::stop() {

	_is_running = false;

	close(_listening);
	// close and delete all TCP connection
	for (std::map<int, TCPConnection *>::iterator it = _map.begin(); it != _map.end(); ++it) {
		close(it->first);
		delete it->second;
	}
}

void	Server::run() {

	_pollfds.push_back(pollfd_wrapper(_listening));
	//Envoye listening socket dans la liste des sockets a surveiller
	
	//Les clients peuvent se connecter
	if (listen(_listening, MAX_QUEUE))
		throw SocketException(strerror(errno));

	_is_running = true;

	while (_is_running) {

		if (poll(&_pollfds[0], _pollfds.size(), calculate_next_timeout()) == -1) {
			int	saved_er = errno;
			if (saved_er == EINTR)
				return;
			else
				throw SocketException(strerror(errno));
		}
		//Surveillance de listening
		if (_pollfds[0].revents & POLLIN)
			create_tcp_socket();

		monitor_connections();
		check_timeouts();
	}
}

int		Server::calculate_next_timeout() {

	int	now = time(NULL);

	if (_map.empty())
		return (100);

	int	next_timeout = NO_REQUEST_MAX_TIME;

	for (std::map<int, TCPConnection *>::iterator it = _map.begin(); it != _map.end(); ++it) {
		TCPConnection	*conn = it->second;

		int	remaining;

		if (conn->getEndOfRequestTime()) {
			remaining = NO_REQUEST_MAX_TIME - (now - conn->getEndOfRequestTime());
			if (remaining > 0 && remaining < next_timeout)
				next_timeout = remaining;
		}

		if (conn->getHeaderTime()) {
			remaining = HEADER_MAX_TIME - (now - conn->getHeaderTime());
			if (remaining > 0 && remaining < next_timeout)
				next_timeout = remaining;
		}

		if (conn->getBodyTime()) {
			remaining = BODY_MAX_TIME - (now - conn->getBodyTime());
			if (remaining > 0 && remaining < next_timeout)
				next_timeout = remaining;
		}

		if (conn->getLastChunkTime()) {
			remaining = BETWEEN_CHUNK_MAX_TIME- (now - conn->getLastChunkTime());
			if (remaining > 0 && remaining < next_timeout)
				next_timeout = remaining;
		}
	}

	if (next_timeout <= 0)
		return 1;

	return next_timeout * 1000;
}

void	Server::check_timeouts() {
	
	time_t	now = time(NULL);
	int		timeout = 0;

	std::vector<pollfd>::iterator	it = _pollfds.begin();
	++it;
	while (it != _pollfds.end()) {
		TCPConnection	*conn = _map[it->fd];

		timeout = 0;

		if (conn->getEndOfRequestTime() && (now - conn->getEndOfRequestTime() > HEADER_MAX_TIME))
			timeout = 1;

		else if (conn->getHeaderTime() && (now - conn->getHeaderTime() > HEADER_MAX_TIME))
			timeout = 1;

		else if (conn->getBodyTime() && (now - conn->getBodyTime() > BODY_MAX_TIME))
			timeout = 1;

		else if (conn->getLastChunkTime() && (now - conn->getLastChunkTime() > BETWEEN_CHUNK_MAX_TIME))
			timeout = 1;

		if (timeout) {
			std::cout << "TIMEOUT !" << std::endl;
			it = close_tcp_connection(it);
		}

		else {
			++it;
		}
	}
}

std::vector<pollfd>::iterator	Server::close_tcp_connection(std::vector<pollfd>::iterator it) {

	std::cout << "Closing connection" << std::endl;
	close(it->fd);
	delete _map[it->fd];	// Warning: it creates the element if doesn't exist yet
	_map.erase(it->fd);
	return _pollfds.erase(it);
}

void	Server::create_server_socket() {
	
	// Create a socket by specifying:
	//	- domain / adress family (Ipv4, AppleTalk ...), 
	//	- semantics / type / unit (Streams, datagrams ...) 
	//	- protocol: dependent of domain + type

	_listening = socket(AF_INET, SOCK_STREAM, 0);
	if (_listening == -1)
		throw SocketException(strerror(errno));

	// bypass the TIME_WAIT socket security
	
    int opt = 1;
    setsockopt(_listening, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void	Server::bind_server_socket() {
	
	// Bind the socket to a local IP / port
	
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// allocate res with a linked list containing all interfaces (ip adresses):
	//	- 127.0.0.1 / localhost
	//	- 192.168.1.100 / eth0
	//	- 127.17.0.1 / docker
	//	and some other...

	int	status = getaddrinfo(NULL, _port.c_str(), &hints, &res);
	if (status) {
		freeaddrinfo(res);
		throw SocketException(gai_strerror(status));
	}
	
	if (bind(_listening, res->ai_addr, res->ai_addrlen) == -1) {
		freeaddrinfo(res);
		throw SocketException(strerror(errno));
	}

	freeaddrinfo(res);
}

// configure socket (non-blocking) and wrap it in a pollfd for the tracking_tab
pollfd	Server::pollfd_wrapper(int fd) {

	pollfd	new_socket;
	int flags = fcntl(fd, F_GETFL, 0);

	// make the socket non-blocking
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	new_socket.fd = fd;
	new_socket.events = POLLIN;
	new_socket.revents = 0;

	return new_socket;
}

// fd that represents a client connected using TCP
void	Server::create_tcp_socket() {

	sockaddr_in	param;
	memset(&param, 0, sizeof(param));//FONCTION INTERDITE
	socklen_t	tcp_size = sizeof(param);
	

	//Creation du socket connecte pour le tcp
	int	tcp_socket = accept(_listening, (sockaddr *)&param, &tcp_size);

	if (tcp_socket == -1)
		throw SocketException(strerror(errno));

	if (_pollfds.size() < MAX_CONNECTIONS) {

		// add the pollfd derived from the socket to the fds list
		_pollfds.push_back(pollfd_wrapper(tcp_socket));

		// create a new tcp with the socket and add it to the map
		TCPConnection	*connection = new TCPConnection(tcp_socket);
		_map[tcp_socket] = connection;

		std::cout << "A new TCP connection arrived !" << std::endl;
	}
	else {
		std::cout << "Too many TCP connections, impossible to connect" << std::endl;
		close(tcp_socket);
	}
}

void	Server::monitor_connections() {

	std::vector<pollfd>::iterator	it = _pollfds.begin();
	++it;
	
	while (it != _pollfds.end()) {
		if (it->revents & POLLIN) {	// what if chunk size > 4096? --> wait for next turn

			TCPConnection	*connection = _map[it->fd];

			//La requete precedente a etre geree
			if (connection->get_status() == END)
				connection->initialize_transfer();

			//Header en cours de transfert
			if (connection->get_status() == READING_HEADER)
				connection->read_header();

			if (connection->get_status() == WAIT_FOR_BODY)
				connection->check_body_headers();

			if (connection->get_status() == READING_BODY)
				connection->read_body();

			// La requet est syntaxiquement complete
			if (connection->get_status() == READ_COMPLETE) {

				connection->end_transfer();

				// conn->_response(request);
				// OU

				// if (condition pour keep-alive)
				// it = close_tcp_connection(it);
				
				++it;
			}
			//Doit-on fermer la connexion si une erreur arrive ?
			if (connection->get_status() == ERROR ||
					connection->get_status() == CLIENT_DISCONNECTED) {

				// conn->_respponse(request)

				// DELETE the TCP connecion

				// if (condition pour keep-alive)
				it = close_tcp_connection(it);
			}
			else
				++it;
		}
		else
			++it;
	}	
}

//ATTENTION : envoyer petit a petit (ajouter le header specifiant la taille des chunk envoyes)
void Server::simple_reply(int clientSocket, const char *filename)
{
    struct stat fileStat;
    if (stat(filename, &fileStat) == -1) return;//Envoyer code d'erreur serveur
    int fileSize = fileStat.st_size;

    int fd = open(filename, O_RDONLY);
    if (fd == -1)  {
		return;//Il faudra envoyer le bon code d'erreur 4xxFIN/5xx si problème
	}

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
	response[responseSize] = '\0';

    std::cout << "Server's response: " << response << std::endl;

    send(clientSocket, response, responseSize, 0);

    delete[] fileContent;
    delete[] response;
}

	//Request	request(message);	// 

		/*
	What are the steps ?

	1. Accumulation: is the request complete ? if no, wait for the remaining chunks (!timeout)
	2. Syntactic parsing: is the request correctly formatted ?
	3. Semantic validation: is the request correct according to server's state?
	
	4. Generate response
	5. Send response
	6. Following connection: should the server keep communication or end it, or else ?
	
	If error occured at any point, send corresponding error response and return
	*/


	//send(client_fd.fd, buff, bytes_received + 1, 0); // (or write())

