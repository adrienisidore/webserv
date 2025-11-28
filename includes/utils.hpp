#ifndef UTILS_HPP
# define UTILS_HPP

# include "webserv.hpp"

bool findCgiProgramForPath(const std::string &handlers, const std::string &path, std::string &outProgram);

#endif