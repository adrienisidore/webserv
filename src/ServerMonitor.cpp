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
	ServerMonitor::_instance->_is_running = false;
}

void	ServerMonitor::stop() {

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

	for (std::map<std::string, ServerConfig>::const_iterator config_it = _global_config.getServers().begin(); config_it != _global_config.getServers().end(); ++config_it) {

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


void ServerMonitor::add_new_client_socket(int listening) {

	ServerConfig config = _map_server_configs[listening];

	// structure générique IPv4/IPv6
	struct sockaddr_storage client_addr;
	socklen_t addr_len = sizeof(client_addr);

	// acceptation du client → remplit client_addr
	int tcp_socket = accept(listening, (struct sockaddr *)&client_addr, &addr_len);

	if (tcp_socket == -1)
		throw SocketException(strerror(errno));

	if (_map_connections.size() < MAX_CONNECTIONS) {

		_pollfds.insert(connected_socket_end(), pollfd_wrapper(tcp_socket));

		// PASSAGE DE L'ADRESSE AU TCPConnection
		TCPConnection *connection = new TCPConnection(tcp_socket, config, client_addr, addr_len);

		_map_connections[tcp_socket] = connection;

		std::cout << "A new TCP connection arrived !" << std::endl;
	}
	else {
		std::cout << "Too many TCP connections, impossible to connect" << std::endl;
		close(tcp_socket);
	}
}

// ServerMonitor.cpp

void	ServerMonitor::add_new_cgi_socket(CGI cgi) {

    // 1. SETUP READER (Output Pipe from Child)
    _pollfds.push_back(pollfd_wrapper(cgi._outpipe[0]));
    _pollfds.back().events = POLLIN;

    // Insert the CGI object into the map
    _map_cgis.insert(std::pair<int, CGI>(cgi._outpipe[0], cgi));

    // [CRITICAL FIX] Access the map entry and CLEAR the body
    // We must remove "LOLILOL" from the reader, otherwise it sticks to the response.
    _map_cgis[cgi._outpipe[0]].setCurrentBody(""); 


    // 2. SETUP WRITER (Input Pipe to Child)
    if (cgi.getMethod() == "POST" && !cgi.getCurrentBody().empty()) {
        
        _pollfds.push_back(pollfd_wrapper(cgi._inpipe[1]));
        _pollfds.back().events = POLLOUT;
        
        // We insert the ORIGINAL 'cgi' here (which still has the body "LOLILOL")
        // because the writer needs it.
        _map_cgis.insert(std::pair<int, CGI>(cgi._inpipe[1], cgi));
    }
    else {
        // If no body or GET, close the write end immediately so child gets EOF
        close(cgi._inpipe[1]);
    }
}

std::vector<pollfd>::iterator ServerMonitor::close_tcp_connection(std::vector<pollfd>::iterator it) {
    
    int fd = it->fd;

	std::cout << "CLOSE TCP CONNECTION: " << fd << std::endl;

    std::map<int, TCPConnection*>::iterator conn_it = _map_connections.find(fd);
    if (conn_it == _map_connections.end()) {
        close(fd);
        return _pollfds.erase(it);
    }
    
    TCPConnection* conn = conn_it->second;
    
    // Close CGI (invalidates iterators!)
    if (conn->get_status() == NOT_READY_TO_SEND) {
        close_associated_cgi(fd);  // Don't return iterator
    }
    
    //  Re-find TCP connection by fd
    it = find_pollfd_iterator(fd);
    
    if (it == _pollfds.end()) {
        // Connection disappeared somehow
        std::cerr << "Error: fd " << fd << " not found after CGI close!" << std::endl;
        delete conn;
        _map_connections.erase(conn_it);
        return _pollfds.begin();  // Safe: return valid iterator
    }
    
    // Now safe to close
    close(fd);
    delete conn;
    _map_connections.erase(conn_it);
    return _pollfds.erase(it);
}

// Helper
std::vector<pollfd>::iterator ServerMonitor::find_pollfd_iterator(int fd) {
    for (std::vector<pollfd>::iterator it = _pollfds.begin();
         it != _pollfds.end(); ++it) {
        if (it->fd == fd) {
            return it;
        }
    }
    return _pollfds.end();
}


std::vector<pollfd>::iterator	ServerMonitor::close_associated_cgi(int fd) {

	std::cout << "CLOSE ASSOCIATED CGI" << std::endl;

	std::map<int, TCPConnection*>::iterator conn_it = _map_connections.find(fd);
    
    if (conn_it == _map_connections.end()) {
        return _pollfds.end();
    }


	CGI cgi = conn_it->second->_response._cgi;

	std::cout << "cgi socket : " << cgi._outpipe[0] << std::endl;

	for (std::vector<pollfd>::iterator it = connected_socket_end(); it != _pollfds.end(); ++it) {
		std::cout << "it->fd : " << it->fd << std::endl;
		if (it->fd == cgi._outpipe[0]) {
			return (close_cgi_socket(it));
		}
	}
	return (_pollfds.end());
}

std::vector<pollfd>::iterator ServerMonitor::close_cgi_socket(std::vector<pollfd>::iterator it) {

	std::cout << "CLOSE CGI SOCKET" << std::endl;

    int fd = it->fd;
    
    std::map<int, CGI>::iterator cgi_it = _map_cgis.find(fd);
    
    if (cgi_it == _map_cgis.end()) {
        close(fd);
        _pollfds.erase(it);
        return _pollfds.end();
    }

    pid_t pid = cgi_it->second._pid;
    
    if (pid > 0) {
        std::cout << "Killing CGI process: " << pid << std::endl;
        kill(pid, SIGKILL);
        
        // int status;
        // waitpid(pid, &status, WNOHANG);  // Returns immediately if not ready
        
        // If it returns 0, zombie still exists but will be reaped by init eventually
    }
    
    close(fd);
    _map_cgis.erase(cgi_it);
    return _pollfds.erase(it);
}

// std::vector<pollfd>::iterator	ServerMonitor::close_cgi_socket(std::vector<pollfd>::iterator it) {
//
// 	int fd = it->fd;
//
// 	std::map<int, CGI>::iterator cgi_it = _map_cgis.find(fd);
//
//     if (cgi_it == _map_cgis.end()) {
//         return _pollfds.end();
//     }
//
// 	kill(cgi_it->second._pid, SIGKILL);
// 	close(fd);
// 	_map_cgis.erase(fd);
// 	return _pollfds.erase(it);
// }

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
	time_t	last_timeout_check = 0;
	while (_is_running) {

		int	timeout_ms = calculate_next_timeout();

		if (poll(&_pollfds[0], _pollfds.size(), timeout_ms) == -1) {
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

		time_t now = time(NULL);
        if (now != last_timeout_check) {
            std::cout << "Checking timeouts at " << now << std::endl;
            check_timeouts();
            last_timeout_check = now;
        }
        else {
            std::cout << "Skipping timeout check (same second)" << std::endl;
        }
	}
	stop();
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

	while (it != _pollfds.end() && it != connected_socket_end() && _is_running) {	// IT can be last_socket
		std::cout << "connection check - checking fd: " << it->fd 
              << " revents: " << it->revents << std::endl;  // ← ADD THIS

		bool	should_close = false;
		bool	state_changed_header = false;

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
			if (connection->get_status() == READING_HEADER) {
				connection->read_header();
				if (connection->get_status() == WAIT_FOR_BODY)
					state_changed_header = true;
			}

			if (connection->get_status() == WAIT_FOR_BODY)
				connection->check_body_headers();

			if (connection->get_status() == READING_BODY)
				connection->read_body(state_changed_header);
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
			//std::cout << "DETECTED POLLOUT" << std::endl;
			if (connection->get_status() == READY_TO_SEND) {
				
				ssize_t sent;
				sent = send(it->fd, connection->getResponse().getCurrentBody().c_str(), connection->getResponse().getCurrentBody().size(), 0);
                if (sent < 0) {
                    std::cout << "Send failed: " << strerror(errno) << std::endl;
                    should_close = true;
                } else {
                    std::cout << "Send succeded: " << connection->getResponse().getCurrentBody() << std::endl;
					connection->end_transfer();
					it->events = POLLIN;

					if (!connection->getResponse().keep_alive())
						should_close = true;
					}
			}
			else {
				//std::cout << "WARNING: POLLOUT but not READY_TO_SEND" << std::endl;
				//it->events = POLLIN;
			}
		}
		if (should_close)
			it = close_tcp_connection(it);
		else {
			std::cout << "++it" << std::endl;
			++it;
		}
	}	
	for (std::map<int, TCPConnection *>::iterator connection = _map_connections.begin(); connection != _map_connections.end(); ++connection) {
		for (std::map<int, CGI>::iterator it = connection->second->_map_cgi_fds_to_add.begin(); it != connection->second->_map_cgi_fds_to_add.end(); ++it) {
			std::cout << "ADD NEW CGI SOCKET" << std::endl;
			add_new_cgi_socket(it->second);

		}
		connection->second->_map_cgi_fds_to_add.clear();
	}

}

void	ServerMonitor::monitor_cgis() {

	char	buff[BUFF_SIZE];
	int		bytes_received;

	std::vector<pollfd>::iterator	it = connected_socket_end();

	while (it != _pollfds.end() && _is_running) {

		std::cout << "CGI check" << std::endl;

        if (_map_cgis.find(it->fd) == _map_cgis.end()) {
			std::cout << "CGI skip" << std::endl;
            ++it;
            continue;
        }

		CGI	&cgi = _map_cgis[it->fd];

		if (it->revents & (POLLIN | POLLHUP)) {

			std::cout << "=== CGI READ ATTEMPT ===" << std::endl;
			std::cout << "fd: " << it->fd << std::endl;
			std::cout << "revents: " << it->revents << " (POLLIN=" << (it->revents & POLLIN) 
					<< ", POLLHUP=" << (it->revents & POLLHUP) << ")" << std::endl;
			std::cout << "Current body size: " << cgi.getCurrentBody().size() << std::endl;

			memset(buff, 0, BUFF_SIZE);
			bytes_received = read(it->fd, buff, BUFF_SIZE);

			if (bytes_received > 0) {
				std::cout << "Data received: [" << std::string(buff, bytes_received) << "]" << std::endl;
				cgi.append_to_body(buff, bytes_received);
				++it;
				continue;
			}

			else if (bytes_received == 0) {	// EOF
				std::cout << "EOF" << std::endl;
				int	status;
				waitpid(cgi._pid, &status, 0); // or NOHANG ?
				
				if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
						cgi.setCode(500);
						cgi._connection->_response._error_();
						cgi._connection->setStatus(READY_TO_SEND);
						it = close_cgi_socket(it);
						continue;
				}
					//close(cgi._pid);

					cgi._connection->setStatus(READY_TO_SEND);
					cgi._connection->_response.createFromCgi(cgi);
					std::cout << "CURRENT BODY" << cgi._connection->_response.getCurrentBody() << std::endl;
					std::cout << "CGI CURRENT BODY" << cgi.getCurrentBody() << std::endl;
					it = close_cgi_socket(it);
					continue;
			}

			else if (bytes_received < 0) {
				std::cout << "errno: " << errno << " (" << strerror(errno) << ")" << std::endl;	// FORBIDDEN
				cgi.setCode(500);
				cgi._connection->_response._error_();
				cgi._connection->setStatus(READY_TO_SEND);
				// mettre a POLLIN, etc... bonne idee ?
				it = close_cgi_socket(it);
				continue;
			}
		}
		else if (it->revents & POLLOUT) {
            std::string body = cgi.getCurrentBody(); // The data to send
            
            ssize_t written = write(it->fd, body.c_str(), body.size());

            if (written > 0) {
                // Remove the part we just sent from the buffer
                cgi.setCurrentBody(body.erase(0, written));
                
                // If body is empty, we are done writing!
                if (body.empty()) {
                    std::cout << "Finished writing body to CGI.\n";
					close(it->fd);          // Send EOF to CGI stdin
                    _map_cgis.erase(it->fd); // Remove this specific FD from map
                    it = _pollfds.erase(it); // Remove from poll
                    continue;
                }
            }
			else if (written < 0) {
				std::cerr << "Write Error\n";
				close(it->fd);          // Send EOF to CGI stdin
				_map_cgis.erase(it->fd); // Remove this specific FD from map
				it = _pollfds.erase(it); // Remove from poll
				continue;
			}
		}
		else
			++it;
	}
}

std::vector<pollfd>::iterator ServerMonitor::connected_socket_end() {
    std::vector<pollfd>::iterator it = _pollfds.begin();
    
    // Skip listening sockets
    for (size_t i = 0; i < _map_server_configs.size() && it != _pollfds.end(); ++i) {
        ++it;
    }
    
    // Skip TCP connection sockets
    for (size_t i = 0; i < _map_connections.size() && it != _pollfds.end(); ++i) {
        ++it;
    }
	std::cout << "map server configf size : " << _map_server_configs.size() << std::endl;
	std::cout << "map connections    size : " << _map_connections.size() << std::endl;
	std::cout << "map cgis           size : " << _map_cgis.size() << std::endl;
	// sleep(1);
    return it;  // ← Now points to first CGI pipe (or end if no CGIs)
}

int		ServerMonitor::calculate_next_timeout() {

	int	now = time(NULL);

	if (_map_connections.empty())
		return (100);

	int	next_timeout = -1;
	for (std::map<int, TCPConnection *>::iterator it = _map_connections.begin(); it != _map_connections.end(); ++it) {
		TCPConnection	*conn = it->second;

		int	remaining;

		if (conn->getEndOfRequestTime()) {
			remaining = conn->getNoRequestMaxTime() - (now - conn->getEndOfRequestTime());
			if (next_timeout < 0 || (remaining > 0 && remaining < next_timeout))
				next_timeout = remaining;
		}

		if (conn->getHeaderTime()) {
			remaining = conn->getHeaderMaxTime() - (now - conn->getHeaderTime());
			if (next_timeout < 0 || (remaining > 0 && remaining < next_timeout))
				next_timeout = remaining;
		}

		if (conn->getBodyTime()) {
			remaining = conn->getBodyMaxTime() - (now - conn->getBodyTime());
			if (next_timeout < 0 || (remaining > 0 && remaining < next_timeout))
				next_timeout = remaining;
		}

		if (conn->getLastChunkTime()) {
			remaining = conn->getBetweenChunksMaxTime() - (now - conn->getLastChunkTime());
			if (next_timeout < 0 || (remaining > 0 && remaining < next_timeout))
				next_timeout = remaining;
		}

		if (conn->getCGITime()) {
			remaining = conn->getCGIMaxTime() - (now - conn->getCGITime());
			if (next_timeout < 0 || (remaining > 0 && remaining < next_timeout))
				next_timeout = remaining;
		}
	}
	std::cout << "next timeout: " << next_timeout << std::endl;

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

		if (conn->getEndOfRequestTime() && (now - conn->getEndOfRequestTime() > conn->getNoRequestMaxTime())) // CONFIG 
			timeout = 1;

		else if (conn->getHeaderTime() && (now - conn->getHeaderTime() > conn->getHeaderMaxTime())) // CONFIG
			timeout = 1;

		else if (conn->getBodyTime() && (now - conn->getBodyTime() > conn->getBodyMaxTime())) // CONFIG
			timeout = 1;

		else if (conn->getLastChunkTime() && (now - conn->getLastChunkTime() > conn->getBetweenChunksMaxTime())) // CONFIG
			timeout = 1;

		else if (conn->getCGITime() && (now - conn->getCGITime() > conn->getCGIMaxTime())) // CONFIG
			timeout = 1;

		if (timeout) {
			std::cout << "TIMEOUT !" << std::endl;
			conn->_response.setCode(504);
			if (conn->getCGITime()) {
				close_associated_cgi(conn->getTCPSocket());
			}
			conn->_response._error_();
			conn->setStatus(READY_TO_SEND);
			std::vector<pollfd>::iterator poll_it = find_pollfd_iterator(conn->getTCPSocket());
            if (poll_it != _pollfds.end()) {
                poll_it->events = POLLOUT;
            }
		}
		++it;
	}
}
