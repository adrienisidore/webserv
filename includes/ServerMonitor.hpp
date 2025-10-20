#ifndef SERVER_MONITOR_HPP
# define SERVER_MONITOR_HPP

#include "./webserv.hpp"

class	ServerMonitor {

private:

	static ServerMonitor*	_instance;
	std::vector<Server>		_servers;		
	bool					_is_running;

public:

	static void	handle_sigint(int sig);//static pour etre accessible par signal
	void		set_signals_default();
	void		add_server(Server server);

	std::vector<Server>	&getServers();

	ServerMonitor();
	~ServerMonitor();

};

// on pourra utilser run() et stop() a partir de l'instance de chaque serveur
//


#endif
