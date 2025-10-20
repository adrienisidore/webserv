#include "webserv.hpp"

// ------------------------------------------------INIT-------------------------------------------------------

ServerMonitor	*ServerMonitor::_instance = NULL;
GlobalConfig	ServerMonitor::_global_config;

//ATTENTION : penser au cas ou filename == "" (pas de  fichier conf fourni en argument)
ServerMonitor::ServerMonitor(const std::string & filename) {
	set_signals_default();
	_global_config = AutoConfig(filename);
	create_all_listening_sockets();
	// now need to create all the sockets: listening and clients
}

ServerMonitor::~ServerMonitor() {
	stop();
}


void	ServerMonitor::handle_sigint(int sig) {

	(void)sig;
	ServerMonitor::_instance->stop(); // pas sur que suffisant
}

void	ServerMonitor::stop() {
	_is_running = false;

	for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
		close(it->fd);
	}
	for (std::map<int, TCPConnection *>::iterator it = _map_connections.begin(); it != _map_connections.end(); ++it) {
		delete it->second;
	}
}

//Gestion Ctrl + C
void	ServerMonitor::set_signals_default() {

	ServerMonitor::_instance = this;
	signal(SIGINT, handle_sigint);
}

void	ServerMonitor::create_all_listening_sockets() {

	for (std::map<std::string, ServerConfig>::const_iterator config_it = _global_config.getServers().begin(); 
				config_it != _global_config.getServers().end(); ++config_it) {

		// CREATE LISTENING SOCKET

		int listening = socket(AF_INET, SOCK_STREAM, 0);
		if (listening == -1)
			throw SocketException(strerror(errno));
		int opt = 1;
		setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // bypass the TIME_WAIT socket security
			
		// ADD IT TO POLLFDS AND MAP

		_pollfds.push_back(pollfd_wrapper(listening));
		_map_server_configs[listening] = config_it->second;
		
		// BIND IT TO THE CORRECT IP / PORT

		bind_listening_socket(listening);	
		
		}
}

void	ServerMonitor::bind_listening_socket(int listening) {
	
	ServerConfig	config = _map_server_configs[listening];

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

	int	status = getaddrinfo(config.getDirective("host").c_str(), config.getDirective("listen").c_str(), &hints, &res);
	// /!\ HOST and LISTEN must exist and always have the same value format
	if (status) {
		freeaddrinfo(res);
		throw SocketException(gai_strerror(status));
	}
	
	if (bind(listening, res->ai_addr, res->ai_addrlen) == -1) {
		freeaddrinfo(res);
		throw SocketException(strerror(errno));
	}

	freeaddrinfo(res);
}

void	ServerMonitor::add_new_client_socket(int listening) {

	ServerConfig	config = _map_server_configs[listening];

	sockaddr_in	param;
	memset(&param, 0, sizeof(param));//FONCTION INTERDITE
	socklen_t	tcp_size = sizeof(param);
	

	//Creation du socket connecte pour le tcp
	int	tcp_socket = accept(listening, (sockaddr *)&param, &tcp_size);

	if (tcp_socket == -1)
		throw SocketException(strerror(errno));

	if (_pollfds.size() - _global_config.getServers().size() < MAX_CONNECTIONS) { // ! CAN BE SPECIFIED IN CONFIG FILE ?

		// add the pollfd derived from the socket to the fds list
		_pollfds.push_back(pollfd_wrapper(tcp_socket));

		// create a new tcp with the socket and add it to the map
		TCPConnection	*connection = new TCPConnection(tcp_socket, config);
		_map_connections[tcp_socket] = connection;

		std::cout << "A new TCP connection arrived !" << std::endl;
	}
	else {
		std::cout << "Too many TCP connections, impossible to connect" << std::endl;
		close(tcp_socket);
	}

}

std::vector<pollfd>::iterator	ServerMonitor::close_tcp_connection(std::vector<pollfd>::iterator it) {
	std::cout << "Closing connection" << std::endl;
	close(it->fd);
	delete _map_connections[it->fd];	// Warning: it creates the element if doesn't exist yet
	_map_connections.erase(it->fd);
	return _pollfds.erase(it);
}

// configure socket (non-blocking) and wrap it in a pollfd for the tracking_tab
pollfd	ServerMonitor::pollfd_wrapper(int fd) {

	pollfd	new_socket;
	int flags = fcntl(fd, F_GETFL, 0);

	// make the socket non-blocking
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	new_socket.fd = fd;
	new_socket.events = POLLIN;
	new_socket.revents = 0;

	return new_socket;
}

// ------------------------------------------------RUNNING-------------------------------------------------------

void	ServerMonitor::run() {

	//Les clients peuvent se connecter
	for (std::map<int, ServerConfig>::iterator it = _map_server_configs.begin(); it != _map_server_configs.end(); ++it)
	{
		if (listen(it->first, MAX_QUEUE))
			throw SocketException(strerror(errno));
		// if (listen(it->first, atoi(it->second.getDirective("max_queue").c_str())))
		// 	throw SocketException(strerror(errno));

	}
	_is_running = true;
	while (_is_running) {

		if (poll(&_pollfds[0], _pollfds.size(), calculate_next_timeout()) == -1) {
			int	saved_er = errno;
			if (saved_er == EINTR)
				return;
			else
				throw SocketException(strerror(errno));
		}
		for (size_t i = 0; i < _global_config.getServers().size(); ++i) {

			if (_pollfds[i].revents & POLLIN)
				add_new_client_socket(_pollfds[i].fd);

		}
		monitor_connections();
		check_timeouts();
	}
}

void	ServerMonitor::monitor_connections() {

	std::vector<pollfd>::iterator	it = _pollfds.begin();
	for (size_t i = 0; i < _global_config.getServers().size(); ++i) {
		++it;
	}

	while (it != _pollfds.end()) {
		if (it->revents & POLLIN) {	// what if chunk size > 4096? --> wait for next turn

			TCPConnection	*connection = _map_connections[it->fd];
			
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

int		ServerMonitor::calculate_next_timeout() {

	int	now = time(NULL);

	if (_map_connections.empty())
		return (100);

	int	next_timeout = NO_REQUEST_MAX_TIME;

	for (std::map<int, TCPConnection *>::iterator it = _map_connections.begin(); it != _map_connections.end(); ++it) {
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

void	ServerMonitor::check_timeouts() {
	
	time_t	now = time(NULL);
	int		timeout = 0;

	std::vector<pollfd>::iterator	it = _pollfds.begin();
	++it;
	while (it != _pollfds.end()) {
		TCPConnection	*conn = _map_connections[it->fd];

		timeout = 0;

		if (conn->getEndOfRequestTime() && (now - conn->getEndOfRequestTime() > HEADER_MAX_TIME)) // CONFIG 
			timeout = 1;

		else if (conn->getHeaderTime() && (now - conn->getHeaderTime() > HEADER_MAX_TIME)) // CONFIG
			timeout = 1;

		else if (conn->getBodyTime() && (now - conn->getBodyTime() > BODY_MAX_TIME)) // CONFIG
			timeout = 1;

		else if (conn->getLastChunkTime() && (now - conn->getLastChunkTime() > BETWEEN_CHUNK_MAX_TIME)) // CONFIG
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
