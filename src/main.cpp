#include "webserv.hpp"
#include "Server.hpp"
#include "Exceptions.hpp"

int	main() {

	try {

		Server	server;
		server.run();
	}
	catch (SocketException &er) {
		std::cerr << "Error Socket: " << er.what() << std::endl;
	}
	catch (ConnectionException &er) {
		std::cerr << "Error Connection: " << er.what() << std::endl;
	}
	catch (HttpException &er) {
		std::cerr << "Error Http: " << er.what() << std::endl;
	}
	return (0);
}

