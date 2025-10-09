#include "webserv.hpp"

Server	*Server::_instance = NULL;

Server::Server(std::string port)
: _listening(-1), _port(port), _is_running(false), _ressources_path("./ressources")
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

		if (poll(&_pollfds[0], _pollfds.size(), -1) == -1) {
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

	char			buff[BUFF_SIZE];	
	int				bytes_received = 0;
	int				read_header_status;

	std::vector<pollfd>::iterator it = _pollfds.begin();
	++it;//On ne surveille pas listening

	while (it != _pollfds.end()) {

		if (it->revents & POLLIN) {

			// TCP DATA READ
			TCPConnection	*connection = _map[it->fd];//_map : many clients can call at the same time, with map we know who's sending data
			connection->start_new_message();

			//TCP : Recuperation de la data avant transfert dans Request
			do {
				connection->read_data(buff, &bytes_received);
				read_header_status = connection->header_complete(buff, bytes_received);
			}
			while (!read_header_status);

			if (read_header_status == 1) {
				// TCP ERROR ON READING HEADER

				if (bytes_received == 0)
					std::cout << "connection " << std::distance(_pollfds.begin(), it) << " is over" << std::endl;
				else if (bytes_received < 0 && buff[0])
					std::cout << "connection " << std::distance(_pollfds.begin(), it) << " data transfer failed" << std::endl;

				close(it->fd);
				it = _pollfds.erase(it);
			}

			else {
				// header_complete == 2, on doit apporter une reponse
				// CREATE REQUEST FROM HEADER
				Request	request = Request(connection->get_current_message(), connection->get_status_code());
				std::cout << "After reading header: ";
				std::cout << request;
				if ((request.getMethod() == "PUT" || request.getMethod() == "POST") && !request.getStatusCode())
				{
					if (request.getHeaders().find("TRANSFER-ENCODING") != request.getHeaders().end() 
						&& request.getHeaders()["TRANSFER-ENCODING"] == "chunked") {
							connection->read_data_chunked(buff, &bytes_received);
						}
					else if (request.getHeaders().find("CONTENT-LENGTH") != request.getHeaders().end()) {
						// checker si valeur content-length existe
						// que se passe t il si content-length > qte de donnees envoyes ?
						// peut etre eal a 0 ?
						// gerer content-length : https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Content-Length
						//Droit de mettre n'importe quoi dans le body ?
					}
					else {
						request.setStatusCode(411);
					}
				}
				// ADD BODY TO REQUEST
				
				// Response response(request);
				// SEND RESPONSE
				//Si dans les headers on a pas de keep alive, ou une erreur qui necessite fin de connexion,
				//on doit fermer aussi la connection ?
				++it; // Ici on ferme pas la connexion
			}
			
		}
		else
			++it;
	}
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

