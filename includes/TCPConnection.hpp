#ifndef TCPCONNECTION_HPP
# define TCPCONNECTION_HPP

# include "webserv.hpp"

//represent one connection of one client, using TCP
class	TCPConnection {

private:
	int			_tcp_socket; // one connection of one client, using TCP
	time_t		_request_start_time;//at what time the connection arrives
	time_t		_last_tcp_chunk_time;//at what time the last chunk arrives
	std::string	_current_message; // track the bytes received after each chunk
	int			_status_code; //if sth wrong happens during the data transfer
	std::string	_remainder; // first bytes of the body, if yhey've been send with last headers' bytes

public:
	TCPConnection(int tcp_socket);
	~TCPConnection();

	void	start_new_message();
	void	read_data(char buff[BUFF_SIZE], int *bytes_received);
	int		header_complete(char buff[BUFF_SIZE], int bytes);

	int		get_status_code() const;
	std::string	get_current_message() const;

};

#endif
