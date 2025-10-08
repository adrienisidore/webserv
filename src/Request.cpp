#include "webserv.hpp"

Request::Request(const std::string & message): _status_code(0) {

	// Trouver la position du premier \r\n (fin de la première ligne)
	// size_t pos = message.find("\r\n");
	std::string start_line;
	// if (pos != std::string::npos)
	// 	start_line = message.substr(0, pos);
	// else
	// {
	// 	//400 Bad Request : on recupere la start line : La 1ere ligne ne respecte pas la syntaxe <METHOD> <PATH> HTTP/<VERSION>\r\n (trop d'espaces, trop de mots, caracteres interdits). Ou carrement pas de 1er mot
	// 	_status_code = 400;
	// 	std::cout << _status_code;
	// 	return ;
	// }
	start_line = message;
	std::cout << "Start line: [" << start_line << "]" << std::endl;//Verif
	//Checker qu'il n'y ait pas 2 espaces consecutifs
	size_t first_space = start_line.find(' ');
	size_t second_space = start_line.find(' ', first_space + 1);
	if (first_space == std::string::npos || second_space == std::string::npos)
	{
		_status_code = 400;
		std::cout << _status_code;
		return;
	}
	//Remplissage des attributs
	_method = start_line.substr(0, first_space);
	_request_target = start_line.substr(first_space + 1, second_space - first_space - 1);
	_protocol = start_line.substr(second_space + 1);
	std::cout << "Method: [" << _method << "]  Request target: [" << _request_target << "]  Protocole: [" << _protocol << "]" << std::endl;
	if (_method.empty() || _protocol != "HTTP/1.1")
		_status_code = 400;
	else if (_method != "GET" && _method != "HEAD" && _method != "POST" && _method != "PUT" && _method != "DELETE")
		_status_code = 501; // Méthode non implémentée dans ce serveur (exemple : GE / HTTP/1.1)
	std::cout << _status_code;

	//set les Headers dans map
}

void	Request::setHeaders(const std::string & message) {


	std::istringstream stream(message);//format compatible pour getline
	std::string line;//buffer remplit par getline

	std::getline(stream, line);//Pour ignorer la start_line

	while (std::getline(stream, line))
	{
		//\r : pas de headers
		//empty : pas de headers ni de body
		if (line == "\r" || line.empty())
			break ;		
	}
}

Request::~Request() {}