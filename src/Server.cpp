#include "webserv.hpp"

Server	*Server::_instance = NULL;

Server::Server(std::string port)
: _listening(-1), _port(port), _is_running(false), _root("./ressources")
{
	_instance = this;
	set_signals_default();
	create_server_socket();
	bind_server_socket();
}

Server::~Server() {
	stop();
}

void	Server::stop() {

	_is_running = false;

	for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
		close(it->fd);
	}
}

void	Server::run() {

	pollfd	listening_pollfd;//Objet contenant listening

	listening_pollfd.fd = _listening;
	listening_pollfd.events = POLLIN;
	listening_pollfd.revents = 0;

	_fds.push_back(listening_pollfd);//Envoye dans la liste des sockets a surveiller

	//Les clients peuvent se connecter
	if (listen(_listening, MAX_QUEUE))
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
		//Surveillance de listening
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

pollfd	Server::new_non_blocking_socket(int fd) {

	pollfd	new_socket;
	int flags = fcntl(fd, F_GETFL, 0);

	// make the fd non-blocking
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	new_socket.fd = fd;
	new_socket.events = POLLIN;
	new_socket.revents = 0;

	return new_socket;
}

void	Server::create_connected_socket() {

	// if server socket receives data, create a new connected socket 

	sockaddr_in	client;
	memset(&client, 0, sizeof(client));
	socklen_t	client_size = sizeof(client);
	

	//Creation du socket connecte pour le client
	int	connected_socket = accept(_listening, (sockaddr *)&client, &client_size);

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

bool	Server::buffer_incomplete(char buff[BUFF_SIZE], int bytes) {

	if (bytes <= 0)
		return false;

	std::string	str_buff(buff);


	// 408 timeout
	// 413 too large
	// what is the error code ?
	// overall timeout -> 30s
	// between chinks timeout -> 30s
	// 

    /* Set the server socket to non-blocking mode
    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
	on EACH socket (client and server)
	*/
}

//Suppression/ajout et lecture des clients
void	Server::process_clients() {
	
	// if a connected socket receives data, THEN we should do stuff with it

	char			buff[BUFF_SIZE];	
	std::string		message = "";
	int				bytes_received = 0;

	std::vector<pollfd>::iterator it = _fds.begin();
	++it;//Surveillance des clients
	while (it != _fds.end()) {

		message.clear();
		if (it->revents & POLLIN) {

			time_t	start = time(NULL);	
			do {
				memset(buff, 0, sizeof(buff));
				bytes_received = recv(it->fd, buff, sizeof(buff), 0); // (or read())
				message.append(buff);

			}
			while (buffer_incomplete(buff, bytes_received, start));	// AND bytes_received > 0 AND no timeout

			if (bytes_received < 0) {
				std::cout << "Error: " << strerror(errno) << std::endl;
				throw HttpException("Invalid Request");
			}

			if (bytes_received == 0) {
				std::cout << "client " << std::distance(_fds.begin(), it) << " disconnected" << std::endl;
				close(it->fd);
				it = _fds.erase(it);
			}

			else {
				process_message(*it, message);
				++it;
			}
		}
		else
			++it;
	}
}

void	Server::process_message(struct pollfd client_fd, std::string message) {
	
	//Request	request(message);	// 

	(void)client_fd;
	(void)message;
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

}

	//send(client_fd.fd, buff, bytes_received + 1, 0); // (or write())

void	Server::handle_sigint(int sig) {

	(void)sig;
	///Evite d'inclure le this
	if (Server::_instance) {
		Server::_instance->_is_running = false;
	}
}

//Gestion Ctrl + C
void	Server::set_signals_default() {

	Server::_instance = this;
	signal(SIGINT, handle_sigint);
}

