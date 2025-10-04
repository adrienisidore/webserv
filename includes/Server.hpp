#include "./webserv.hpp"

class	Server {

private:

	static Server*		_instance;
	int					_socket;
	std::string			_port;
	bool				_is_running;
	char*				_root;
	std::vector<pollfd>	_fds;

	void		create_server_socket();
	void		bind_server_socket();

	void		create_connected_socket();
	
	void		process_clients();
	void		process_message(struct pollfd client, char buff[BUFF_SIZE], int bytes);
	
	static void	handle_sigint(int sig);
	void		set_signals_default();
	
	// getters and setters...
	
public:

	Server();
	Server(std::string port);
	~Server();

	void	start();
	void	run();
	void	stop();
	void	send_data();
};

// should I create my own exception class ?
// potentially multiple constructors depending on the specified protocol ?
//
// stat pour fonction qui telecharge un contenu sur le serveur (POST)
