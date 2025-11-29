#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# define MAX_QUEUE 3
# define MAX_CONNECTIONS 255
# define BUFF_SIZE 4096
# define HEADER_MAX_SIZE 100000 // at least BUFF_SIZE because of the do while
# define BODY_MAX_SIZE 1000000

# define BETWEEN_CHUNK_MAX_TIME 10000	// max time between TCP chunks WHEN A REQUEST 
# define HEADER_MAX_TIME 10000	// max time for the header to be sent
# define BODY_MAX_TIME 100 // max time for the body to be sent
# define NO_REQUEST_MAX_TIME 20000 // max time between request
# define CGI_TIMEOUT 10000

enum {READING_HEADER,
	READING_BODY,
	WAIT_FOR_BODY,
	READ_COMPLETE, 
	CLIENT_DISCONNECTED,
	END,
	READY_TO_SEND,
	NOT_READY_TO_SEND};

enum {
	CONTENT_LENGTH,
	CHUNKED
};

//checker les librairies que j'utilise pas
# include <iostream>//std::cout, std::cerr.
# include <sys/types.h>//socklen_t et les socket types et autres...
# include <unistd.h>//close(), read()
# include <sys/socket.h>//socket(), bind(), listen(), accept(), setsockopt()
# include <netdb.h>//getaddrinfo(), freeaddrinfo(), gai_strerror()
# include <arpa/inet.h>//ntohs()
# include <arpa/inet.h> // htons() ?
# include <string.h>//memset() : NON AUTORISEE DANS LES FONCTIONS DU SUJET
# include <string>//std::string : FONCTION INTERDITE ?
# include <poll.h>//poll(), struct pollfd
# include <errno.h>//errno
# include <signal.h>
# include <vector>
# include <map>
# include <stdlib.h>
# include <sys/stat.h>   // pour struct stat et stat()
# include <fcntl.h>      // pour open()
# include <cstring>      // pour strlen(), memcpy() : FONCTION INTERDITE
# include <sstream> // ostream : INTERDIT
# include <fstream>
# include <dirent.h>
# include <algorithm>

# include "./GlobalConfig.hpp"
# include "./ServerConfig.hpp"
# include "./LocationConfig.hpp"
# include "ConfigParser.hpp"
# include "HTTPcontent.hpp"
# include "./Request.hpp"
# include "./CGI.hpp"
# include "./Response.hpp"
# include "./TCPConnection.hpp"
# include "./ServerMonitor.hpp"

# include "./autoconfig.hpp"
# include "./Exceptions.hpp"

// ADRI


# include <cmath>//Interdit

# include "./autoconfig.hpp"
# include "./Exceptions.hpp"

// utile pour CGI (checker si fonctions interdites ou useless)
# include <sys/wait.h>
# include <cstdlib>
# include <cstdio>


// Parsing
void	check_args(int ac, char **av);
std::string parseConfig(const std::string& configFile);
std::vector<std::string> split(const std::string& str, char delimiter);

# endif
