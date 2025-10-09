#ifndef SERVER_HPP
# define SERVER_HPP

#include "./webserv.hpp"

class	Server {

private:

	static Server*			_instance;
	int						_listening;
	std::string				_port;
	bool					_is_running;
	std::string				_ressources_path;
	std::vector<pollfd>		_pollfds; //all sockets we monitor, wrapped up for poll()
	std::map<int, TCPConnection *>	_map;//_map : many clients can call at the same time, with map we know who's sending data

	void		create_server_socket();// create a listening socket for the server
	void		bind_server_socket();// bind listening socket with an IP/Port

	void		create_tcp_socket();// create a tcp socket, wrapp it in a pollfd and add it to the track list
	
	void		monitor_connections();//monitor the tcp_socket (client connected)

	static void	handle_sigint(int sig);//static pour etre accessible par signal
	void		set_signals_default();

	pollfd		pollfd_wrapper(int fd);
	
public:

	Server(std::string port);
	~Server();

	void	run();// monitor the listening socket and launch monitor_connections
	void	stop();
	void	send_data();// prend en param la TCPConnection et la Response a envoyer
};

// should I create my own exception class ?
// potentially multiple constructors depending on the specified protocol ?
//
// stat pour fonction qui telecharge un contenu sur le serveur (POST)

#endif
