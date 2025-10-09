#ifndef SERVER_HPP
# define SERVER_HPP

#include "./webserv.hpp"

class	Server {

private:

	static Server*			_instance;
	int						_listening;
	std::string				_port;
	bool					_is_running;
	std::string				_root;
	std::vector<pollfd>		_fds;
	std::map<int, Client *>	_map;

	void		create_server_socket();
	void		bind_server_socket();

	void		create_connected_socket();
	
	void		process_clients();
	void		process_message(struct pollfd client, std::string message);

	static void	handle_sigint(int sig);//static pour etre accessible par signal
	void		set_signals_default();

	pollfd		new_non_blocking_socket(int fd);
	
public:

	Server(std::string port);
	~Server();


	void	run();
	void	stop();
	void	send_data();
};

// should I create my own exception class ?
// potentially multiple constructors depending on the specified protocol ?
//
// stat pour fonction qui telecharge un contenu sur le serveur (POST)

#endif
