#include "../webserv.hpp"

int	create_server_socket(void) {
	
	// Create a socket by specifying:
	//	- domain / adress family (Ipv4, AppleTalk ...), 
	//	- semantics / type / unit (Streams, datagrams ...) 
	//	- protocol: dependent of domain + type

	int	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
		throw std::runtime_error(strerror(errno));

	// bypass the TIME_WAIT socket security
	
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	return server_socket;
}

void	bind_server_socket(int server_socket) {
	
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

	int	status = getaddrinfo(NULL, "8080", &hints, &res);
	if (status)
		throw std::runtime_error(gai_strerror(status));
	
	if (bind(server_socket, res->ai_addr, res->ai_addrlen) == -1)
		throw std::runtime_error(strerror(errno));

	freeaddrinfo(res);	// /!\ res is not freed if an exception is thrown
}

void	create_connected_socket(int server_socket, struct pollfd poll_fd[MAX_CLIENTS], int *nfds) {

	// if server socket receives data, create a new connected socket 

	sockaddr_in	client;
	memset(&client, 0, sizeof(client));
	socklen_t	client_size = sizeof(client);
	

	int	connected_socket = accept(server_socket, (sockaddr *)&client, &client_size);
	if (connected_socket == -1)
		throw std::runtime_error(strerror(errno));

	if (*nfds < MAX_CLIENTS) {
		poll_fd[*nfds].fd = connected_socket;
		poll_fd[*nfds].events = POLLIN;
		(*nfds)++;
		std::cout << "A new client is on the server !" << std::endl;
	}
	else {
		std::cerr << "Too many clients, last one has been disconnected" << std::endl;
		close(connected_socket);
	}
}

// This function should be defined as the method of a class to have all the attributes accessible (like server socket) to make complex actions

void	process_message(struct pollfd client_fd, char buff[BUFF_SIZE], int bytes_received) {

	std::cout << "Client says: " << buff << std::endl;
	send(client_fd.fd, buff, bytes_received + 1, 0); // (or write())
}

void	process_clients(struct pollfd poll_fd[MAX_CLIENTS], int *nfds) {
	
	// if a connected socket receives data, THEN we should do stuff with it

	char			buff[BUFF_SIZE];	

	for (int i = 1; i < *nfds; i++) {

		if (poll_fd[i].revents & POLLIN) {

			memset(buff, 0, sizeof(buff));

			int	bytes_received = recv(poll_fd[i].fd, buff, sizeof(buff), 0); // (or read())

			if (bytes_received < 0)
				throw std::runtime_error(strerror(errno));

			if (bytes_received == 0) {
				std::cout << "client " << i << " disconnected" << std::endl;
				close(poll_fd[i].fd);
				poll_fd[i] = poll_fd[*nfds - 1];
				(*nfds)--;
				i--;
			}
			else {
				process_message(poll_fd[i], buff, bytes_received);
			}
		}
	}
}

void	run_server(int server_socket) {

	int				nfds = 1;
	struct pollfd	poll_fd[MAX_CLIENTS];

	poll_fd[0].fd = server_socket;
	poll_fd[0].events = POLLIN;

	if (listen(server_socket, MAX_QUEUE))
		throw std::runtime_error(strerror(errno));

	while (true) {

		if (poll(poll_fd, nfds, -1) == -1)
			throw std::runtime_error(strerror(errno));

		if (poll_fd[0].revents & POLLIN)
			create_connected_socket(server_socket, poll_fd, &nfds);

		process_clients(poll_fd, &nfds);
	}
	
	// is never called
	for (int i = 1; i < nfds; i++) {
		close(poll_fd[i].fd);
	}
}

void	start_server() {

	int	server_socket;

	server_socket = create_server_socket();
	bind_server_socket(server_socket);
	run_server(server_socket);
}

int	main() {

	try {
		start_server();
	}
	catch (std::exception &er) {
		std::cerr << "Error on server side" << er.what() << std::endl;
	}
	return (0);
}

