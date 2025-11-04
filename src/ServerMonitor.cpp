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
	}
	else
		throw ParsingException("listn: invalid IP / Port"); // devrait etre inutile si parsing est bien fait
	//Verifier si on a besoin de convertir la c_str() avec htons ntohs
	int	status = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
	// /!\ HOST and LISTEN must exist and always have the same value format
	// ATTENTION il peut y avoir plus de serverConfigs que de listening sockets 
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
		_pollfds.insert(last_socket(), pollfd_wrapper(tcp_socket));

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
	_map_cgis.insert({socket, cgi});  

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

		monitor_listening_sockets();
		monitor_connections();
		monitor_cgis();

		std::cout << "size of map connections: " << _map_connections.size() << std::endl;
		
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
	for (size_t i = 0; i < _global_config.getServers().size(); ++i) {
		++it;
	}

	while (it != last_socket() && _is_running) {	// ONLY FOR CLIENTFDS, NOT CGIS

		if (it->revents & (POLLHUP | POLLERR | POLLNVAL)) {
			it = close_tcp_connection(it);
			// SEND ERROR RESPONSE HERE
			continue;
		}
		
		TCPConnection	*connection = _map_connections[it->fd];

        if (connection->get_status() == ERROR ||
            connection->get_status() == CLIENT_DISCONNECTED) {
            it = close_tcp_connection(it);
            continue;
        }
		if (it->revents & POLLIN) {
			
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
			if (connection->get_status() == READ_COMPLETE || connection->get_status() == ERROR) {
				connection->initialize_response();
				it->events = POLLOUT;
				// on ERROR -> keep in head that connection should be CLOSED
			}

			else if (connection->get_status() == CLIENT_DISCONNECTED)
				it = close_tcp_connection(it);
		}
		if (it->revents & POLLOUT) {
			if (connection->get_status() == READY_TO_SEND) {
				
				//connection->send_response();
				connection->end_transfer();
				it->events = POLLIN;
				connection->set_status(END);

				if (connection->getResponse().getCode())
					it = close_tcp_connection(it);
			}
			else
				++it;
		}
		else
			++it;
	}	
}

void	ServerMonitor::monitor_cgis() {

	char	buff[BUFF_SIZE];
	int		bytes_received;

	std::vector<pollfd>::iterator	it = last_socket();
	++it;
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
				cgi._connection->setStatus(ERROR);
				cgi._connection->set_error(500);
				// mettre a POLLIN, etc... bonne idee ?
				it = close_cgi_socket(it);
			}
		}
	}
}

std::vector<pollfd>::iterator	ServerMonitor::last_socket() {
	std::vector<pollfd>::iterator	it = _pollfds.begin();
	for (size_t i = 0; i < _map_connections.size() + _map_server_configs.size(); ++i) {
		++it;
	}
	return it;
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
