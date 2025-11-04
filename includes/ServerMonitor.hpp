#ifndef SERVERMONITOR_HPP
# define SERVERMONITOR_HPP

#include "./webserv.hpp"

class	ServerMonitor {

private:

	static ServerMonitor*			_instance;
	static GlobalConfig				_global_config;
	bool							_is_running;
	std::vector<pollfd>				_pollfds; //all sockets we monitor, wrapped up for poll()
	std::map<int, TCPConnection *>	_map_connections;// map all tcp_sockets to their corresponding TCP connection
	std::map<int, ServerConfig>		_map_server_configs; // map all listening sockets to their corresponding server configs 
	std::map<int, CGI>				_map_cgis; // map all cgi sockets with their corresponding cgis 

	void							create_all_listening_sockets();
	void							bind_listening_socket(int socket);
	void							add_new_client_socket(int socket);
	void							add_new_cgi_socket(int socket, CGI cgi);
	pollfd							pollfd_wrapper(int fd);

	
	std::vector<pollfd>::iterator	close_tcp_connection(std::vector<pollfd>::iterator);
	std::vector<pollfd>::iterator	close_cgi_socket(std::vector<pollfd>::iterator);
	std::vector<pollfd>::iterator	last_socket() ;
	void							monitor_listening_sockets();
	void							monitor_connections();//monitor the tcp_socket (client connected)
	void							monitor_cgis();


	static void						handle_sigint(int sig);//static pour etre accessible par signal
	void							set_signals_default();

	int								calculate_next_timeout();
	void							check_timeouts();

public:

	ServerMonitor(const std::string & filename);
	~ServerMonitor();

	void	run();// monitor the listening socket and launch monitor_connections
	void	stop();

};

// on pourra utilser run() et stop() a partir de l'instance de chaque serveur


#endif
