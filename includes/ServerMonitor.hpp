#ifndef SERVERMONITOR_HPP
# define SERVERMONITOR_HPP

#include "./webserv.hpp"

class	ServerMonitor {

private:

	static GlobalConfig				_global_config;
	bool							_is_running;
	std::vector<pollfd>				_pollfds; //all sockets we monitor, wrapped up for poll()
	std::map<int, TCPConnection *>	_map_connections;// map all tcp_sockets to their corresponding TCP connection
	std::map<int, ServerConfig>		_map_server_configs; // map all listening sockets to their corresponding server configs 
	std::map<int, CGI>				_map_cgis; // map all cgi sockets with their corresponding cgis 

	void							create_all_listening_sockets();
	void							bind_listening_socket(int socket);
	void							add_new_client_socket(int socket);
	pollfd							pollfd_wrapper(int fd);

	
	std::vector<pollfd>::iterator	close_tcp_connection(std::vector<pollfd>::iterator);
	std::vector<pollfd>::iterator	close_cgi_socket(std::vector<pollfd>::iterator);
	void							close_associated_cgi(int fd);

	std::vector<pollfd>::iterator	connected_socket_end() ;
	std::vector<pollfd>::iterator	find_pollfd_iterator(int fd);
	std::vector<pollfd>::iterator	cgi_input_error(std::vector<pollfd>::iterator it, CGI &cgi);
	void							monitor_listening_sockets();
	void							monitor_connections();//monitor the tcp_socket (client connected)
	void							monitor_cgis();


	static void						handle_sigint(int sig);//static pour etre accessible par signal
	void							set_signals_default();

	int								calculate_next_timeout();
	void							check_timeouts();

public:

	static ServerMonitor*			_instance;

	ServerMonitor(const std::string & filename);
	~ServerMonitor();

	void	add_all_cgi_sockets();
	void	add_new_cgi_socket(CGI cgi);
	void	visualize();
	void	run();// monitor the listening socket and launch monitor_connections
	void	stop();

};

#endif
