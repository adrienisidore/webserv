#include "webserv.hpp"

// ------------------------------------------------INIT-------------------------------------------------------

ServerMonitor	*ServerMonitor::_instance = NULL;
GlobalConfig	ServerMonitor::_global_config;

// set up the server: set signals, second parsing of config file and create listening sockets
ServerMonitor::ServerMonitor(const std::string & filename) {
	set_signals_default();
	_global_config = AutoConfig(filename);
	create_all_listening_sockets();
}

ServerMonitor::~ServerMonitor() {}


void	ServerMonitor::handle_sigint(int sig) {

	(void)sig;
	ServerMonitor::_instance->_is_running = false;
}

// delete the TCPConnection pointers and close all sockets
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

// handle Ctrl + C
void	ServerMonitor::set_signals_default() {

	ServerMonitor::_instance = this;
	signal(SIGINT, handle_sigint);
}

void	ServerMonitor::create_all_listening_sockets() {

	for (std::map<std::string, ServerConfig>::const_iterator config_it = _global_config.getServers().begin(); config_it != _global_config.getServers().end(); ++config_it) {

		// create one listening socket
		int listening = socket(AF_INET, SOCK_STREAM, 0);
		if (listening == -1)
			throw SocketException(strerror(errno));

		// bypass the TIME_WAIT socket security
		int opt = 1;
		setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			
		// add it to pollfds and map_server_config
		_pollfds.push_back(pollfd_wrapper(listening));
		_map_server_configs[listening] = config_it->second;
		
		bind_listening_socket(listening);	
		}
}

// Bind the socket to a local IP / port
void	ServerMonitor::bind_listening_socket(int listening) {
	
	ServerConfig	config = _map_server_configs[listening];

	
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

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
		throw ParsingException("listn: invalid IP / Port");

	// allocate res with a linked list containing all available interfaces (ip adresses):
	int	status = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
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

	// generic structure IPv4/IPv6
	struct sockaddr_storage client_addr;
	socklen_t addr_len = sizeof(client_addr);

	// accept client → fill client_addr
	int tcp_socket = accept(listening, (struct sockaddr *)&client_addr, &addr_len);

	if (tcp_socket == -1)
		throw SocketException(strerror(errno));

	if (_map_connections.size() < MAX_CONNECTIONS) {

		_pollfds.insert(connected_socket_end(), pollfd_wrapper(tcp_socket));
		TCPConnection *connection = new TCPConnection(tcp_socket, config, client_addr, addr_len);
		_map_connections[tcp_socket] = connection;
	}
	else {
		// too many connections
		close(tcp_socket);
	}
}

// add potential cgi sockets (should be called at the end of loop) 
void	ServerMonitor::add_all_cgi_sockets() {

	for (std::map<int, TCPConnection *>::iterator connection = _map_connections.begin(); connection != _map_connections.end(); ++connection) {
		for (std::map<int, CGI>::iterator it = connection->second->_map_cgi_fds_to_add.begin(); it != connection->second->_map_cgi_fds_to_add.end(); ++it) {
			add_new_cgi_socket(it->second);
		}
		connection->second->_map_cgi_fds_to_add.clear();
	}
}

void	ServerMonitor::add_new_cgi_socket(CGI cgi) {

    // setup reader (Output Pipe)
    _pollfds.push_back(pollfd_wrapper(cgi._outpipe[0]));
    _pollfds.back().events = POLLIN;
    _map_cgis.insert(std::pair<int, CGI>(cgi._outpipe[0], cgi));
    
	_map_cgis[cgi._outpipe[0]].setCurrentBody(""); 

    // setup writer (input pipe)
    if (cgi.getMethod() == "POST" && !cgi.getCurrentBody().empty()) {
        
        _pollfds.push_back(pollfd_wrapper(cgi._inpipe[1]));
        _pollfds.back().events = POLLOUT;        
        _map_cgis.insert(std::pair<int, CGI>(cgi._inpipe[1], cgi));
    }
    else {
        // If no body or GET, close the write end immediately so child gets EOF
        close(cgi._inpipe[1]);
    }
}

std::vector<pollfd>::iterator ServerMonitor::close_tcp_connection(std::vector<pollfd>::iterator it) {
    
    int fd = it->fd;

    std::map<int, TCPConnection*>::iterator conn_it = _map_connections.find(fd);
    if (conn_it == _map_connections.end()) {
        close(fd);
        return _pollfds.erase(it);
    }
    TCPConnection* conn = conn_it->second;
    
    if (conn->get_status() == NOT_READY_TO_SEND) {
        close_associated_cgi(fd);
    }
    //  Re-find TCP connection by fd (iterator may not be valid anymore)
    it = find_pollfd_iterator(fd);
    
    if (it == _pollfds.end()) {
        // Connection disappeared somehow
        std::cerr << "Error: fd " << fd << " not found after CGI close!" << std::endl;
        delete conn;
        _map_connections.erase(conn_it);
        return _pollfds.begin();
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


// close the CGI associated to the connection identified by the socket 'fd'
void	ServerMonitor::close_associated_cgi(int fd) {

	std::map<int, TCPConnection*>::iterator conn_it = _map_connections.find(fd);
    if (conn_it == _map_connections.end()) {
        return;
    }

	CGI cgi = conn_it->second->_response._cgi;
	for (std::vector<pollfd>::iterator it = connected_socket_end(); it != _pollfds.end(); ++it) {
		if (it->fd == cgi._outpipe[0]) {
			close_cgi_socket(it);
			return;
		}
	}
}

std::vector<pollfd>::iterator ServerMonitor::close_cgi_socket(std::vector<pollfd>::iterator it) {

    int fd = it->fd;
    
    std::map<int, CGI>::iterator cgi_it = _map_cgis.find(fd);
    
    if (cgi_it == _map_cgis.end()) {
        close(fd);
        _pollfds.erase(it);
        return _pollfds.end();
    }

    pid_t pid = cgi_it->second._pid;
    
	if (pid > 0)
        kill(pid, SIGKILL);
    close(fd);
    _map_cgis.erase(cgi_it);
    return _pollfds.erase(it);
}

// configure socket (non-blocking) and wrap it in a pollfd for the tracking
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

// Run the server
void	ServerMonitor::run() {

	// listen for all listening sockets
	for (std::map<int, ServerConfig>::iterator it = _map_server_configs.begin(); it != _map_server_configs.end(); ++it)
	{
		if (listen(it->first, MAX_QUEUE))
			throw SocketException(strerror(errno));
	}

	_is_running = true;
	time_t	last_timeout_check = 0;
	
	// main loop
	while (_is_running) {

		// poll() -> wait for events and triggers each timeout_ms if no events yet
		int	timeout_ms = calculate_next_timeout();
		if (timeout_ms < 0 || timeout_ms > UI_REFRESH_RATE_MS)
			timeout_ms = UI_REFRESH_RATE_MS;

		if (poll(&_pollfds[0], _pollfds.size(), timeout_ms) == -1) {
			int	saved_er = errno;
			if (saved_er == EINTR)
				continue;
			else
				throw SocketException(strerror(errno));
		}

		monitor_listening_sockets();
		monitor_connections();
		monitor_cgis();

		this->visualize();

		time_t now = time(NULL);
        if (now != last_timeout_check) {
            check_timeouts();
            last_timeout_check = now;
        }
	}
	stop();
}

// if POLLIN on a listening socket -> create a new client socket
void	ServerMonitor::monitor_listening_sockets() {
	for (size_t i = 0; i < _global_config.getServers().size(); ++i) {

		if (_pollfds[i].revents & POLLIN)
			add_new_client_socket(_pollfds[i].fd);
	}
}

// listen all connected sockets:
// - POLLIN -> read request to generate response
// - POLLOUT -> send response
void	ServerMonitor::monitor_connections() {

	// skip the listening sockets
	std::vector<pollfd>::iterator	it = _pollfds.begin();
	for (size_t i = 0; i < _global_config.getServers().size(); ++i) {
		++it;
	}

	// loop through connected sockets
	while (it != _pollfds.end() && it != connected_socket_end() && _is_running) {

		bool	should_close = false;
		bool	state_changed_header = false;

		if (it->revents & (POLLHUP | POLLERR | POLLNVAL)) {
			it = close_tcp_connection(it);
			continue;
		 }
		
		TCPConnection	*connection = _map_connections[it->fd];

		if (it->revents & POLLIN) {

			// set / reset connection
			if (connection->get_status() == END)
				connection->initialize_transfer();

			if (connection->get_status() == READING_HEADER) {
				connection->read_header();
				if (connection->get_status() == WAIT_FOR_BODY)
					state_changed_header = true;
			}

			if (connection->get_status() == WAIT_FOR_BODY)
				connection->check_body_headers();

			if (connection->get_status() == READING_BODY)
				connection->read_body(state_changed_header);

			if (connection->get_status() == CLIENT_DISCONNECTED)
				should_close = true;

			// request is fully read -> take action
			else if (connection->get_status() == READ_COMPLETE) {
				connection->execute_method();
				it->events = POLLOUT;
			}
		}

		else if (it->revents & POLLOUT) {

			// send response
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
		}
		if (should_close)
			it = close_tcp_connection(it);
		else
			++it;
	}
	add_all_cgi_sockets();
}

// listen to all cgi sockets:
// - POLLIN -> read from the pipe and append to cgi body
// - POLLOUT -> push the request body into the pipe
void	ServerMonitor::monitor_cgis() {

	char	buff[BUFF_SIZE];
	int		bytes_received;

	std::vector<pollfd>::iterator	it = connected_socket_end();

	while (it != _pollfds.end() && _is_running) {

        if (_map_cgis.find(it->fd) == _map_cgis.end()) {
            ++it;
            continue;
        }

		CGI	&cgi = _map_cgis[it->fd];
		if (it->revents & (POLLIN | POLLHUP)) {

			memset(buff, 0, BUFF_SIZE);
			bytes_received = read(it->fd, buff, BUFF_SIZE);

			if (bytes_received > 0) {
				cgi.append_to_body(buff, bytes_received);
				++it;
				continue;
			}

			// EOF
			else if (bytes_received == 0) {
				int	status;
				waitpid(cgi._pid, &status, 0);
				
				// error
				if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {;
						it = cgi_input_error(it, cgi);
						continue;
				}
				cgi._connection->setStatus(READY_TO_SEND);
				cgi._connection->_response.createFromCgi(cgi);
				it = close_cgi_socket(it);
				continue;
			}

			// error
			else if (bytes_received < 0) {
				it = cgi_input_error(it, cgi);
				continue;
			}
		}
		else if (it->revents & POLLOUT) {
            std::string body = cgi.getCurrentBody();
            
            ssize_t written = write(it->fd, body.c_str(), body.size());

            if (written > 0) {
                cgi.setCurrentBody(body.erase(0, written));
                
                // If body is empty, we are done writing
                if (body.empty()) {
					// Send EOF to CGI stdin
					close(it->fd);
                    _map_cgis.erase(it->fd);
                    it = _pollfds.erase(it);
                    continue;
                }
            }
			// error
			else if (written < 0) {
				close(it->fd); 
				_map_cgis.erase(it->fd);
				it = _pollfds.erase(it);
				continue;
			}
		}
		else
			++it;
	}
}

std::vector<pollfd>::iterator	ServerMonitor::cgi_input_error(std::vector<pollfd>::iterator it, CGI &cgi) {
		cgi.setCode(500);
		cgi._connection->_response._error_();
		cgi._connection->setStatus(READY_TO_SEND);
		return close_cgi_socket(it);
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
			// TIMEOUT !
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
