#include "webserv.hpp"

// check the number and type of the arguments
void	check_args(int ac, char **av) {

	std::string	config_file_name;

	if (ac == 1)
		return ;
	if (ac > 2)
		throw (std::invalid_argument("wrong number of arguments"));

	config_file_name = av[1];

	size_t index = config_file_name.find(".conf");
	if (index == std::string::npos) // dangerous
		throw (std::invalid_argument("invalid config file format"));

	if (access(av[1], R_OK) != 0)
		throw std::invalid_argument("impossible to read config file");

    int fd = open(av[1], O_RDONLY);
    if (fd == -1)
		throw std::invalid_argument("impossible to read config file");
}