#include "webserv.hpp"

int	main(int ac, char **av) {

	(void)ac;
	Request	request(av[1]);

	// try {

	// 	Server	server("8080");
	// 	server.run();
	// }
	// catch (SocketException &er) {
	// 	std::cerr << "Error Socket: " << er.what() << std::endl;
	// }
	// catch (ConnectionException &er) {
	// 	std::cerr << "Error Connection: " << er.what() << std::endl;
	// }
	// catch (HttpException &er) {
	// 	std::cerr << "Error Http: " << er.what() << std::endl;
	// }
	return (0);
}

