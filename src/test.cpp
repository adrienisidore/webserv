#include "../webserv.hpp"

int	global_function() {

	// SERVER-SIDE
	
	// 1. Create a socket by specifying:
	//	- domain / adress family (Ipv4, AppleTalk ...), 
	//	- semantics / type / unit (Streams, datagrams ...) 
	//	- protocol: dependent of domain + type

	int	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
		throw std::runtime_error(strerror(errno));

	// bypass the TIME_WAIT socket security
	
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// 2. Bind it to a local IP / port
	
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;	// + NULL node -> suitable for bind()

	// allocate res with a linked list containing all interfaces (ip adresses):
	//	- 127.0.0.1 / localhost
	//	- 192.168.1.100 / eth0
	//	- 127.17.0.1 / docker
	//	and some other...

	int	status = getaddrinfo(NULL, "8080", &hints, &res);
	if (status)
		throw std::runtime_error(gai_strerror(status));
	
	// 3. bind the socket with the first adress of the list and mark it as passive
	
	if (bind(server_socket, res->ai_addr, res->ai_addrlen) == -1)
		throw std::runtime_error(strerror(errno));

	freeaddrinfo(res);	// res is not freed if an exception is thrown

	if (listen(server_socket, MAX_QUEUE))
		throw std::runtime_error(strerror(errno));

	// 4. watch over the socket
	
	struct pollfd	poll_fd[MAX_CLIENTS];
	poll_fd[0].fd = server_socket;
	poll_fd[0].events = POLLIN;

	int		nfds = 1;
	char	buff[BUFF_SIZE];	


	while (true) {

		// poll blocks execution until something happens, but it watches all the sockets at once 
		// if poll returns, something happened on at least one socket, we must find which one
		
		if (poll(poll_fd, nfds, -1) == -1)
			throw std::runtime_error(strerror(errno));

		if (poll_fd[0].revents & POLLIN) {
			// if server socket receives data, create a new connected socket 

			sockaddr_in	client;
			memset(&client, 0, sizeof(client));
			socklen_t	client_size = sizeof(client);
			

			int	connected_socket = accept(server_socket, (sockaddr *)&client, &client_size);
			if (connected_socket == -1)
				throw std::runtime_error(strerror(errno));

			if (nfds < MAX_CLIENTS) {
				poll_fd[nfds].fd = connected_socket;
				poll_fd[nfds].events = POLLIN;
				nfds++;
				std::cout << "A new client is on the server !" << std::endl;
			}
			else {
                std::cerr << "Too many clients, last one has been disconnected" << std::endl;
                close(connected_socket);
            }
		}
		for (int i = 1; i < nfds; i++) {

			if (poll_fd[i].revents & POLLIN) {
				// if a connected socket receives data, THEN we should do stuff with it

				memset(buff, 0, sizeof(buff));
				int	bytes_received = recv(poll_fd[i].fd, buff, sizeof(buff), 0); // (or read())

				if (bytes_received < 0)
					throw std::runtime_error(strerror(errno));

				if (bytes_received == 0) {
					std::cout << "client " << i << " disconnected" << std::endl;
					close(poll_fd[i].fd);
					poll_fd[i] = poll_fd[nfds - 1];
					nfds--;
					i--;
				}
				else {
					std::cout << "Client says: " << buff << std::endl;
					send(poll_fd[i].fd, buff, bytes_received + 1, 0); // (or write())
				}
			}

		}
	}
	for (int i = 1; i < nfds; i++) {
		close(poll_fd[i].fd);
	}
	return (0);
}

int	main() {

	try {
		global_function();
	}
	catch (std::exception &er) {
		std::cerr << er.what() << std::endl;
	}
	return (0);
}

