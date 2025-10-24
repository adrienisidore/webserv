#ifndef WEBSERV_HPP
# define WEBSERV_HPP

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

# include <sys/stat.h>   // pour struct stat et stat()
# include <fcntl.h>      // pour open()
# include <cstring>      // pour strlen(), memcpy() : FONCTION INTERDITE
# include <sstream> // ostream : INTERDIT
# include <fstream>

# include <cmath>//Interdit

# include "./CGI.hpp"

#endif
