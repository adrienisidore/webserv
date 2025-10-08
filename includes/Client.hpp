#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "webserv.hpp"

class	Client {

private:
	time_t		_request_start_time;
	time_t		_last_chunk_time;
	int			_fd;
	std::string	_current_message;
	int			_status_code;
	std::string	_remainder;

public:
	Client(int fd);
	~Client();

	void	start_new_message();
	void	read_data(char buff[BUFF_SIZE], int *bytes_received);
	bool	header_complete(int bytes_received);

	int		get_status_code();
	std::string	get_current_message();

};
// each client could contain a reference to the fd in the fd list

#endif
