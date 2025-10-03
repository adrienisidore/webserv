#include "./webserv.hpp"

class	Server {

private:

	int				_socket;
	struct pollfd	_fds[MAX_CLIENTS];
	int				_nfds;
	char			_buff[BUFF_SIZE];	// should I put the buffer here ?


}

// potentially multiple constructors depending on the specified protocol ?
