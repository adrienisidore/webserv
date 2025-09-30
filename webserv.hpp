
#ifndef WEBSERV_HPP
# define WEBSERV_HPP

//checker les librairies que j'utilise pas
# include <iostream>//std::cout, std::cerr.
# include <sys/types.h>//socklen_t et les socket types
# include <unistd.h>//close()
# include <sys/socket.h>//socket(), bind(), listen(), accept(), setsockopt()
# include <netdb.h>//getaddrinfo(), freeaddrinfo(), gai_strerror()
# include <arpa/inet.h>//ntohs()
# include <string.h>//memset() : NON AUTORISEE DANS LES FONCTIONS DU SUJET
# include <string>//std::string
# include <poll.h>//poll(), struct pollfd

#endif