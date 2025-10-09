#include "webserv.hpp"

Request::Request(const std::string & message, const int & s_c): _status_code(s_c) {

	setStartLine(message);
	setHeaders(message);
	// addBody(message);//setBody hors du constructeur
}

void	Request::setStatusCode(const int & st) {
	_status_code = st;
	std::cout << _status_code;//POUR TESTER (A SUPPRIMER)
}

void	Request::setStartLine(const std::string & message) {

	if (_status_code) return ;
	if (message.find("\r\n\r\n") == std::string::npos) return (setStatusCode(400)); // mauvaise requête : pas de fin de headers

	// Recuperation de la 1ere ligne
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
	start_line = message;//POUR TESTER (A SUPPRIMER)
	std::cout << "Start line: [" << start_line << "]" << std::endl;//POUR TESTER (A SUPPRIMER)
	//<METHOD> <PATH> [HTTP/1.1] : 2 espaces sont necessaires pour une requete valide.
	size_t first_space = start_line.find(' ');
	size_t second_space = start_line.find(' ', first_space + 1);
	if (first_space == std::string::npos || second_space == std::string::npos) return (setStatusCode(400));
	//Remplissage des attributs
	_method = start_line.substr(0, first_space);
	_request_target = start_line.substr(first_space + 1, second_space - first_space - 1);
	_protocol = start_line.substr(second_space + 1);
	if (_method.empty() || _protocol != "HTTP/1.1") return (setStatusCode(400));
	if (_method != "GET" && _method != "HEAD" && _method != "POST" && _method != "PUT" && _method != "DELETE") return (setStatusCode(501));
	// Méthode non implémentée dans ce serveur (exemple : GIT / HTTP/1.1)
}

void	Request::setHeaders(const std::string & message) {

	if (_status_code) return ;
	std::istringstream stream(message);//format compatible pour getline
	std::string line;//buffer remplit par getline

	std::getline(stream, line);//Pour ignorer la start_line

	while (std::getline(stream, line))
	{
		//\r : pas de headers
		//empty : pas de headers ni de body
		if (line == "\r" || line.empty())
			break ;
		//Si on ne trouve pas de ":" ou pas de \r dans la ligne ou " :" Error 400
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos || line[line.size() - 1] != '\r' || line[colonPos - 1] == ' ') return (setStatusCode(400));
		//Suppression de \r
		line.erase(line.size() - 1);

		std::string key = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 1);
		//Suppression des espaces/tab en debut et fin de valeur (HTTP/1.1 trim)
		while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0,1);
		while (!value.empty() && (value[value.size()-1] == ' ' || value[value.size()-1] == '\t'))
		//Serveur non sensible a la casse : content-length == CONTENT-LENGTH
		for (std::string::size_type i = 0; i < key.size(); ++i) {
			unsigned char c_ = static_cast<unsigned char>(key[i]);//Sinon comp. indef. sur caracteres accentues
			if (std::islower(c_)) key[i] = std::toupper(c_);
		}
		_headers[key] = value;	
	}
	//Verifie que HOST est present (obligatoire)
	std::map<std::string, std::string>::const_iterator it = _headers.find("HOST");
    if (it == _headers.end() || it->second.empty()) return (setStatusCode(400));	
}

void		Request::addBody(const std::string & message) {

	if (_status_code || _method == "GET" || _method == "HEAD" || _method == "DELETE") return ;
	// Cherche la séparation entre headers et body : "\r\n\r\n"
	size_t pos = message.find("\r\n\r\n");
	// Tout ce qui vient après "\r\n\r\n" est le body
	_body = message.substr(pos + 4);	
}

std::string	Request::getMethod() {return (_method);}

std::string	Request::getRequestTarget() {return (_request_target);}

std::string	Request::getProtocol() {return (_protocol);}

int	Request::getStatusCode() {return (_status_code);}

std::map<std::string, std::string> Request::getHeaders() {return (_headers);}

std::string	Request::getBody() {return (_protocol);}

Request::~Request() {}
