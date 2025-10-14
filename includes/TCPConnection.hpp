#ifndef TCPCONNECTION_HPP
# define TCPCONNECTION_HPP

# include "webserv.hpp"

//represent one connection of one client, using TCP
class	TCPConnection {

private:
	int			_tcp_socket; // one connection of one client, using TCP
	
	time_t		_end_of_request_time; // at what time has the last request arrived
	time_t		_header_start_time;//at what time the connection arrives
	time_t		_body_start_time; // at what time the body arrives
	time_t		_last_tcp_chunk_time;//at what time the last chunk arrives
	
	char		_buff[BUFF_SIZE];
	std::string	_body_buff;	
	int			_bytes_received;
	int			_status; //statut de la livraison de la donnee
	Request		_request;	// current request

public:
	TCPConnection(int tcp_socket);
	~TCPConnection();

	Request	*start_new_request();
	Request	getRequest() const;

	void	start_new_body();
	void	read_header();
	void	read_body();
	int		header_complete(char buff[BUFF_SIZE], int bytes);
	int		body_length_complete(char buff[BUFF_SIZE], int bytes, int bytes_written, unsigned long len, unsigned long max_len);
	void	append_to_header(char *buff);
	int		get_status() const;
	void	set_status(int status);
	void	end_request();

	int		getTCPSocket() const;

	void	write_body_chunked();
	void	write_body_length();

	time_t	getEndOfRequestTime() const;  
	time_t	getHeaderTime() const;
	time_t	getBodyTime() const;
	time_t	getLastChunkTime() const;

};

#endif
