#include "webserv.hpp"

ServerMonitor	*ServerMonitor::_instance = NULL;

ServerMonitor::ServerMonitor() {
	_instance = this;
	set_signals_default();
}

ServerMonitor::~ServerMonitor() {}


std::vector<Server>	&ServerMonitor::getServers() {
	return _servers;
}

void	ServerMonitor::add_server(Server server) {
	_servers.push_back(server);
}

void	ServerMonitor::handle_sigint(int sig) {

	(void)sig;
	///Evite d'inclure le this
	for (std::vector<Server>::iterator it = _instance->_servers.begin(); it != _instance->_servers.end(); ++it) {
		it->stop();
	}
}

//Gestion Ctrl + C
void	ServerMonitor::set_signals_default() {

	ServerMonitor::_instance = this;
	signal(SIGINT, handle_sigint);
}

