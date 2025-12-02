#include "webserv.hpp"

int	main(int ac, char **av) {

 	try {
		check_args(ac, av);
		std::string temp_file;
		if (ac == 1)
			temp_file = parseConfig("default.conf");
		else
			temp_file = parseConfig(av[1]);

 		ServerMonitor	ServerMonitor(temp_file);
		ServerMonitor.run();
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
	catch (ParsingException &er) {
		std::cerr << "Error Parsing:" << er.what() << std::endl;
	}
 	catch (std::exception &er) {
 		std::cerr << "Error: " << er.what() << std::endl;
 	}

 	return (0);
 }
