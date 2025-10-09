#ifndef TCPCONNECTION_HPP
# define TCPCONNECTION_HPP

# include "webserv.hpp"

class	TCPConnection {

private:
	time_t		_request_start_time;//time : connexion arrive
	time_t		_last_tcp_chunk_time;//time : dernier chunk lu
	int			_fd; // connected socket
	std::string	_current_message; // contenu brut de la requete en cours de lecture
	int			_status_code; 
	std::string	_remainder; // potentiel debut du body apres lecture des headers

public:
	TCPConnection(int fd);
	~TCPConnection();

	void	start_new_message();
	void	read_data(char buff[BUFF_SIZE], int *bytes_received);
	int		header_complete(char buff[BUFF_SIZE], int bytes);

	int		get_status_code() const;
	std::string	get_current_message() const;

};
// each TCPConnection could contain a reference to the fd in the fd list

#endif
