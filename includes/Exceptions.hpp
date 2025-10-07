#ifndef EXCEPTIONS_HPP
# define EXCEPTIONS_HPP

#include "./webserv.hpp"

class	SocketException : public std::runtime_error {

	// create, bind, listen
	public:
		SocketException(std::string msg): std::runtime_error(msg) {}	
};

class	ConnectionException	: public std::runtime_error {

	// connection closed, reset, timeout, signal interrupt ?
	public:
		ConnectionException(std::string msg): std::runtime_error(msg) {}	
};

class	HttpException : public std::runtime_error {

	// request
	public:
		HttpException(std::string msg): std::runtime_error(msg) {}	
};

// we should also deal with error codes

#endif