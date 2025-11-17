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

ServerMonitor::~ServerMonitor() {}


void	ServerMonitor::handle_sigint(int sig) {

	(void)sig;
	ServerMonitor::_instance->stop(); // pas sur que suffisant
}

void	ServerMonitor::stop() {
	_is_running = false;

	for (std::map<int, TCPConnection *>::iterator it = _map_connections.begin(); it != _map_connections.end(); ++it) {
		if (it->second != NULL)
			delete it->second;
	}
	for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
		if (it->fd >= 0)
			close(it->fd);
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
		
		std::cout << "listening socket created" << std::endl;
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
	//	and some other

	std::string listen_value = config.getDirective("listen");
	size_t colon_pos = listen_value.find(':');
	std::string host;
	std::string port;

	if (colon_pos != std::string::npos)
	{
		// Format: "127.0.0.1:8080"
		host = listen_value.substr(0, colon_pos);
		port = listen_value.substr(colon_pos + 1);
		std::cout << "host -> '" << host << "'" << std::endl;
		std::cout << "port -> '" << port << "'" << std::endl;
	}
	else
		throw ParsingException("listn: invalid IP / Port"); // devrait etre inutile si parsing est bien fait
	//Verifier si on a besoin de convertir la c_str() avec htons ntohs
	int	status = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
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

	if (_map_connections.size() < MAX_CONNECTIONS) { // specified in config file
		// add the pollfd derived from the socket to the fds list
		_pollfds.insert(connected_socket_end(), pollfd_wrapper(tcp_socket));

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

void	ServerMonitor::add_new_cgi_socket(int socket, CGI cgi) {


	sockaddr_in	param;
	memset(&param, 0, sizeof(param));//FONCTION INTERDITE
	socklen_t	tcp_size = sizeof(param);

	int	tcp_socket = accept(socket, (sockaddr *)&param, &tcp_size);

	if (tcp_socket == -1)
		throw SocketException(strerror(errno));

	_pollfds.push_back(pollfd_wrapper(socket));
	_map_cgis.insert(std::pair<int, CGI>(socket, cgi));  

	std::cout << "A new CGI has been launched !" << std::endl;
}

//std::vector<pollfd>::iterator	ServerMonitor::close_cg
std::vector<pollfd>::iterator	ServerMonitor::close_tcp_connection(std::vector<pollfd>::iterator it) {
	std::cout << "Closing connection" << std::endl;
	close(it->fd);
	delete _map_connections[it->fd];	// Warning: it creates the element if doesn't exist yet
	_map_connections.erase(it->fd);
	return _pollfds.erase(it);
}

std::vector<pollfd>::iterator	ServerMonitor::close_cgi_socket(std::vector<pollfd>::iterator it) {

	close(it->fd);
	_map_cgis.erase(it->fd);
	return _pollfds.erase(it);
}

void	ServerMonitor::close_cgi_fd(int fd) {

	std::vector<pollfd>::iterator it = _pollfds.begin();
	while (it != _pollfds.end()) {
		if (it->fd == fd) {
			close_cgi_socket(it);
			break;
		}
		++it;
	}
}

// configure socket (non-blocking) and wrap it in a pollfd for the tracking_tab
pollfd	ServerMonitor::pollfd_wrapper(int fd) {

	pollfd	new_socket;
	int flags = fcntl(fd, F_GETFL, 0);

	// make the socket non-blocking
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	new_socket.fd = fd;
	new_socket.events = POLLIN; // | POLLOUT
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
		std:: cout << "socket is listening..." << std::endl;
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
		
		for (size_t i = 0; i < _pollfds.size(); ++i) {
			if (_pollfds[i].revents != 0) {
				std::cout << "fd " << _pollfds[i].fd << " revents: " << _pollfds[i].revents << std::endl;
			}
		}

		// std::cout << "before monitoring listening" << std::endl;
		monitor_listening_sockets();
		// std::cout << "before monitoring connections" << std::endl;
		monitor_connections();
		// std::cout << "before monitoring cgis" << std::endl;
		// std::cout << "size of pollfd: " << _pollfds.size() << std::endl;
		// std::cout << "size of map connections: " << _map_connections.size() << std::endl;
		// std::cout << "size of map server configs: " << _map_server_configs.size() << std::endl;
		monitor_cgis();
		// std::cout << "after monitoring cgis" << std::endl;
		check_timeouts();
	}
}

void	ServerMonitor::monitor_listening_sockets() {
	// ecouter les listening sockets
	for (size_t i = 0; i < _global_config.getServers().size(); ++i) {

		if (_pollfds[i].revents & POLLIN)
			add_new_client_socket(_pollfds[i].fd);
	}
}

void	ServerMonitor::monitor_connections() {
	// ecouter les sockets clients

	std::vector<pollfd>::iterator	it = _pollfds.begin();
	// std::cout << "=== MONITOR CONNECTIONS START ===" << std::endl;
	//    std::cout << "Total pollfds: " << _pollfds.size() << std::endl;
	//    std::cout << "Server configs: " << _map_server_configs.size() << std::endl;
	//    std::cout << "TCP connections: " << _map_connections.size() << std::endl;
	for (size_t i = 0; i < _global_config.getServers().size(); ++i) {
		++it;
	}

	while (it != connected_socket_end() && _is_running) {	// IT can be last_socket
		std::cout << "connection check - checking fd: " << it->fd 
              << " revents: " << it->revents << std::endl;  // ← ADD THIS

		bool	should_close = false;

		if (it->revents & (POLLHUP | POLLERR | POLLNVAL)) {
			std::cout << "DETECTED HANGUP/ERROR" << std::endl;
			it = close_tcp_connection(it);
			continue;
		}
		
		TCPConnection	*connection = _map_connections[it->fd];

		if (it->revents & POLLIN) {
			std::cout << "DETECTED POLLIN" << std::endl;
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

			if (connection->get_status() == CLIENT_DISCONNECTED)
				should_close = true;

			else if (connection->get_status() == READ_COMPLETE) {
				connection->execute_method();
				it->events = POLLOUT;
				// on ERROR -> keep in head that connection should be CLOSED
			}

		}
		else if (it->revents & POLLOUT) {
			std::cout << "DETECTED POLLOUT" << std::endl;
			if (connection->get_status() == READY_TO_SEND) {
				
				ssize_t sent;
				sent = send(it->fd, connection->getResponse().getCurrentBody().c_str(), connection->getResponse().getCurrentBody().size(), 0);
                if (sent < 0) {
                    std::cout << "Send failed: " << strerror(errno) << std::endl;
                    should_close = true;
                } else {

					connection->end_transfer();
					it->events = POLLIN;

					if (!connection->getResponse().keep_alive())
						should_close = true;
					}
			}
			else {
				std::cout << "WARNING: POLLOUT but not READY_TO_SEND, resetting to POLLIN" << std::endl;
				it->events = POLLIN;
			}
		}
		if (should_close)
			it = close_tcp_connection(it);
		else
			++it;
	}	
}

void	ServerMonitor::monitor_cgis() {

	char	buff[BUFF_SIZE];
	int		bytes_received;

	std::vector<pollfd>::iterator	it = connected_socket_end();

	while (it != _pollfds.end() && _is_running) {

		CGI	cgi = _map_cgis[it->fd];
		// check for POLLERR
		if (it->revents & POLLIN) {

			memset(buff, 0, BUFF_SIZE);
			bytes_received = recv(it->fd, buff, BUFF_SIZE, 0);
			cgi.append_to_body(buff, BUFF_SIZE);

			if (bytes_received == 0) {	// EOF
				cgi._connection->setStatus(READY_TO_SEND);
				cgi._connection->_response.copyFrom(cgi);
				it = close_cgi_socket(it);
			}

			else if (bytes_received < 0) {
				cgi._connection->set_error(500);
				// mettre a POLLIN, etc... bonne idee ?
				it = close_cgi_socket(it);
			}
		}
	}
}

std::vector<pollfd>::iterator ServerMonitor::connected_socket_end() {
    std::vector<pollfd>::iterator it = _pollfds.begin();
    
    // Skip listening sockets
    for (size_t i = 0; i < _map_server_configs.size(); ++i) {
        ++it;
    }
    
    // Skip TCP connection sockets
    for (size_t i = 0; i < _map_connections.size(); ++i) {
        ++it;
    }
    
    return it;  // ← Now points to first CGI pipe (or end if no CGIs)
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
	//std::cout << "next timeout: " << next_timeout << std::endl;

	if (next_timeout <= 0)
		return 1;

	return next_timeout * 1000;
}

void	ServerMonitor::check_timeouts() {
	
	time_t	now = time(NULL);
	int		timeout = 0;

    std::vector<pollfd>::iterator it = _pollfds.begin();
    for (size_t i = 0; i < _global_config.getServers().size(); ++i) {
        ++it;
    }
	while (it != connected_socket_end()) {
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
