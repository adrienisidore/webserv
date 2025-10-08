#include "webserv.hpp"

Request::Request(const std::string &str, const int &s_c) {
	(void)str;
	_status_code = s_c;
	// TAKE ADRI VERSION
}

Request::~Request() {}
