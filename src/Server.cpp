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
	std::cout << "end of loop" << std::endl;
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

	std::cout << "next timeout" << next_timeout << std::endl;
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

		else if (conn->getLastChunkTime() && (now - conn->getLastChunkTime() > CHUNK_MAX_TIME))
			timeout = 1;

		if (timeout) {
			std::cout << "TIMEOUT !" << std::endl;
			it = close_tcp_connection(it);
		}

		else {
			std::cout << "no TIMEOUT " << std::endl;
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

// There is something off with this function.
// Je trouve ca bizarre que la TCPConnection aie un header et un status code. tout cela devrait etre dans la requete
// => TCPConnection et Server::monitor_connections devraient s'occuper uniquement du protocole TCP.
// Oui mais il faut etre capable d'interrompre ce protocole si quelque chose tourne mal...

void	Server::monitor_connections() {

	std::vector<pollfd>::iterator	it = _pollfds.begin();
	++it;
	
	while (it != _pollfds.end()) {
		if (it->revents & POLLIN) {	// what if chunk size > 4096? --> wait for next turn

			TCPConnection	*connection = _map[it->fd];

			connection->set_last_tcp_chunk_time(time(NULL));

			//La requete precedente a etre geree
			if (connection->get_status() == END)
				connection->start_new_request();

			//Header en cours de transfert
			if (connection->get_status() == READING_HEADER)
				connection->read_header();

			if (connection->get_status() == READING_BODY)
				connection->read_body();

			if (connection->get_status() == READ_COMPLETE) {
				if (connection->getRequest().getStatusCode() >= 400) 
					std::cout << "Client sent invalid request" << std::endl;
				else
					std::cout << "Client sent valid request" << std::endl;

				// decide what to respond
				simple_reply(connection->getTCPSocket(), "ressources/ServerInterface.html");
				connection->set_status(END);
				connection->set_end_of_request_time(time(NULL));
				++it;
			}

			// Erreur dans le header
			// !! TIMEOUT -> thread independant
			else if (connection->get_status() == HEADER_TOO_LARGE || 
					connection->get_status() == READ_ERROR ||
					connection->get_status() == CLIENT_DISCONNECTED) {
				std::cout << "Closing connection due to error/disconnect" << std::endl;
				// DELETE the TCP connecion
				it = close_tcp_connection(it);
			}
			else
				++it;
		}
		else
			++it;
	}	
}

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

    // Construire réponse complète (headers + body)
    // const char *header =
    //     "HTTP/1.1 200 OK\r\n"
    //     "Content-Type: text/html\r\n"
    //     "Connection: keep-alive\r\n\r\n";
    
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

    send(clientSocket, response, responseSize, 0);

    delete[] fileContent;
    delete[] response;
}

/*
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

			//TCP : Recuperation de la data avant transfert dans Request
			do {
				connection->read_data(buff, &bytes_received);
				connection->append_to_header(buff);
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
				Request	request = Request(connection->get_current_header(), connection->get_status_code());
				std::cout << "After reading header: ";
				std::cout << request;


				// DON'T FORGET THE POTENTIAL REMAINDER
				if ((request.getMethod() == "PUT" || request.getMethod() == "POST") && !request.getStatusCode())
				{
					std::string name;
					int			read_body_status;
					int			bytes_written;

					if (request.getHeaders().find("NAME") != request.getHeaders().end())
						name = request.getHeaders()["NAME"] + get_time_stamp();
					else
						name = get_time_stamp(); 

					int fd_write = open(("./ressources/" + name).c_str(), O_APPEND, O_CREAT); // POST ou PUT ?

					if (fd_write == -1)
						request.setStatusCode(403);	// should also break from this

					// TRANSFER-ENCODING: CHUNKED
					if (request.getHeaders().find("TRANSFER-ENCODING") != request.getHeaders().end() 
						&& request.getHeaders()["TRANSFER-ENCODING"] == "chunked") {
						

						// WITHOUT CGI FOR NOW
						do {
							std::string	http_chunk = "";
							int			one_http_chunk_complete = 1;
							int			all_http_chunks_complete = 1;	// for 1 TCP chunk...

							do {
								connection->read_data(buff, &bytes_received);
								if (bytes_received > 0 || (bytes_received < 0 && !buff[0])) {
									do {
										one_http_chunk_complete = connection->parse_http_chunk(buff, bytes_received, &http_chunk);
									}
									while (!one_http_chunk_complete);
									bytes_written = write(fd_write, http_chunk.c_str(), http_chunk.size());
									http_chunk = "";
								// + remaining...
								}
								all_http_chunks_complete = connection->
							}
							while (!all_http_chunks_complete)

							read_body_status = 
						} while (read_body_status);
					}

					// CONTENT-LENGTH SPECIFIED
					else if (request.getHeaders().find("CONTENT-LENGTH") != request.getHeaders().end()
						&& is_valid_length(request.getHeaders()["CONTENT-LENGTH"])) {
			
						// checker si valeur content-length existe
						// que se passe t il si content-length > qte de donnees envoyes ?
						// peut etre eal a 0 ?
						// gerer content-length : https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Content-Length
						unsigned long	max_body_len = atol(request.getHeaders()["CONTENT-LENGTH"].c_str());	// forbidden ?
						unsigned long	curr_body_len = 0;

						do {
							connection->read_data(buff, &bytes_received);
							curr_body_len += bytes_received;
							if (curr_body_len <= max_body_len && (bytes_received > 0 || (bytes_received < 0 && !buff[0]))) {
								bytes_written = write(fd_write, buff, BUFF_SIZE);
							}
							else if (bytes_received > 0 || (bytes_received < 0 && !buff[0])) {
								bytes_written = write(fd_write, buff, curr_body_len - max_body_len);
							}
							read_body_status = connection->body_length_complete(buff, bytes_received, bytes_written, curr_body_len, max_body_len);
						} while (!read_body_status);
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
						   */
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

