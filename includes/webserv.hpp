#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# define MAX_QUEUE 3
# define MAX_CONNECTIONS 10
# define BUFF_SIZE 4096
# define REQUEST_MAX_TIME 5
# define CHUNK_MAX_TIME 5
# define HEADER_MAX_SIZE 10000 // at least BUFF_SIZE because of the do while
# define BODY_MAX_SIZE 1000000

enum {READING_HEADER, 
	READ_COMPLETE, 
	HEADER_TOO_LARGE, 
	REQUEST_TOO_LARGE,
	READ_TIMEOUT, 
	READ_ERROR,	// keskece?
	CLIENT_DISCONNECTED,
	READING_BODY,
	BODY_TOO_LARGE,
	END};

//checker les librairies que j'utilise pas
# include <iostream>//std::cout, std::cerr.
# include <sys/types.h>//socklen_t et les socket types et autres...
# include <unistd.h>//close(), read()
# include <sys/socket.h>//socket(), bind(), listen(), accept(), setsockopt()
# include <netdb.h>//getaddrinfo(), freeaddrinfo(), gai_strerror()
# include <arpa/inet.h>//ntohs()
# include <string.h>//memset() : NON AUTORISEE DANS LES FONCTIONS DU SUJET
# include <string>//std::string : FONCTION INTERDITE ?
# include <poll.h>//poll(), struct pollfd
# include <errno.h>//errno
# include <signal.h>
# include <vector>
# include <map>
# include <stdlib.h>

#include "./Exceptions.hpp"
#include "./Request.hpp"
#include "./TCPConnection.hpp"
#include "./Server.hpp"

// ADRI
#include "./Response.hpp"
#include <sys/stat.h>   // pour struct stat et stat()
#include <fcntl.h>      // pour open()
#include <cstring>      // pour strlen(), memcpy() : FONCTION INTERDITE
#include <sstream> // ostream : INTERDIT


void serverReply(int clientSocket, const char *filename);

#endif
