#ifndef TCPCONNECTION_HPP
# define TCPCONNECTION_HPP

# include "webserv.hpp"

//represent one connection of one client, using TCP
class	TCPConnection {

private:
	int			_tcp_socket; // one connection of one client, using TCP
	time_t		_header_start_time;//at what time the connection arrives
	time_t		_body_start_time; // at what time the body arrives
	time_t		_last_tcp_chunk_time;//at what time the last chunk arrives
	std::string	_current_header; // track the header's bytes received after each TCP chunk
	int			_status_code; //if sth wrong happens during the data transfer
	std::string	_remainder; // first bytes of the body, if yhey've been send with last headers' bytes

public:
	TCPConnection(int tcp_socket);
	~TCPConnection();

	void	start_new_message();
	void	start_new_body();
	void	read_data(char buff[BUFF_SIZE], int *bytes_received);
	int		header_complete(char buff[BUFF_SIZE], int bytes);
	int		body_length_complete(char buff[BUFF_SIZE], int bytes, int bytes_written, unsigned long len, unsigned long max_len);
	void	append_to_header(char *buff);

	int		get_status_code() const;
	std::string	get_current_header() const;

};

#endif
