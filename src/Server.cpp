#include "webserv.hpp"
#include "Server.hpp"
#include "Exceptions.hpp"

Server	*Server::_instance = NULL;

Server::Server() {

	_instance = this;
	_socket = -1;
	_is_running = false;
	_root = NULL;
	_port = "8080";

	start();
}

Server::Server(std::string port) {

	_instance = this;
	_socket = -1;
	_is_running = false;
	_root = NULL;
	_port = port;

	start();
}

Server::~Server() {
	stop();
}

void	Server::start() {

	set_signals_default();
	create_server_socket();
	bind_server_socket();
}

void	Server::stop() {

	_is_running = false;

	for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
		close(it->fd);
	}
}

void	Server::run() {

	pollfd	server_pollfd;

	server_pollfd.fd = _socket;
	server_pollfd.events = POLLIN;
	server_pollfd.revents = 0;

	_fds.push_back(server_pollfd);

	if (listen(_socket, MAX_QUEUE))
		throw SocketException(strerror(errno));

	_is_running = true;

	while (_is_running) {

		if (poll(&_fds[0], _fds.size(), -1) == -1) {
			int	saved_er = errno;
			if (saved_er == EINTR)
				return;
			else
				throw SocketException(strerror(errno));
		}

		if (_fds[0].revents & POLLIN)
			create_connected_socket();

		process_clients();
	}
	std::cout << "end of loop" << std::endl;
}

void	Server::create_server_socket() {
	
	// Create a socket by specifying:
	//	- domain / adress family (Ipv4, AppleTalk ...), 
	//	- semantics / type / unit (Streams, datagrams ...) 
	//	- protocol: dependent of domain + type

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == -1)
		throw SocketException(strerror(errno));

	// bypass the TIME_WAIT socket security
	
    int opt = 1;
    setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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
	
	if (bind(_socket, res->ai_addr, res->ai_addrlen) == -1) {
		freeaddrinfo(res);
		throw SocketException(strerror(errno));
	}

	freeaddrinfo(res);
}

void	Server::create_connected_socket() {

	// if server socket receives data, create a new connected socket 

	sockaddr_in	client;
	memset(&client, 0, sizeof(client));
	socklen_t	client_size = sizeof(client);
	

	int	connected_socket = accept(_socket, (sockaddr *)&client, &client_size);

	if (connected_socket == -1)
		throw SocketException(strerror(errno));	// where will the exception rise

	if (_fds.size() < MAX_CLIENTS) {

		pollfd	new_client;
		new_client.fd = connected_socket;
		new_client.events = POLLIN;
		new_client.revents = 0;
		_fds.push_back(new_client);
		std::cout << "A new client is on the server !" << std::endl;
	}
	else {
		std::cout << "Too many clients, impossible to connect" << std::endl;
		close(connected_socket);
	}
}

void	Server::process_clients() {
	
	// if a connected socket receives data, THEN we should do stuff with it

	char			buff[BUFF_SIZE];	

	std::vector<pollfd>::iterator it = _fds.begin();
	++it;
	while (it != _fds.end()) {

		if (it->revents & POLLIN) {

			memset(buff, 0, sizeof(buff));

			int	bytes_received = recv(it->fd, buff, sizeof(buff), 0); // (or read())

			if (bytes_received < 0) {
				// client might also have just disconnected
				std::cout << "Error: " << strerror(errno) << std::endl;
				throw HttpException("Invalid Request");
			}

			if (bytes_received == 0) {
				std::cout << "client " << std::distance(_fds.begin(), it) << " disconnected" << std::endl;
				close(it->fd);
				it = _fds.erase(it);
			}
			else {
				process_message(*it, buff, bytes_received);
				++it;
			}
		}
		else
			++it;
	}
}

void	Server::process_message(struct pollfd client_fd, char buff[BUFF_SIZE], int bytes_received) {

	std::cout << "Client says: " << buff << std::endl;
	send(client_fd.fd, buff, bytes_received + 1, 0); // (or write())
}

// requires declaring _instance as global variable and setting it to NULL at startup
void	Server::handle_sigint(int sig) {

	(void)sig;
	if (Server::_instance) {
		Server::_instance->_is_running = false;
	}
}

void	Server::set_signals_default() {

	Server::_instance = this;
	signal(SIGINT, handle_sigint);
}

