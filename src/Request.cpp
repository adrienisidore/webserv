#include "webserv.hpp"

Request::Request(const std::string & message, const int & s_c): _status_code(s_c) {

	setStartLine(message);
	setHeaders(message);
	// addBody(message);//setBody hors du constructeur
}

void	Request::setStatusCode(const int & st) {
	_status_code = st;
}

//400 Bad Request : on recupere la start line : La 1ere ligne ne respecte pas la syntaxe <METHOD> <PATH> HTTP/<VERSION>\r\n (trop d'espaces, trop de mots, caracteres interdits). Ou carrement pas de 1er mot
void	Request::setStartLine(const std::string & message) {

	if (_status_code) return ;
	// if (message.find("\r\n\r\n") == std::string::npos) return (setStatusCode(400)); // pas de fin de headers, donc requete mal specifiee

	// Recuperation de la 1ere ligne
	size_t pos = message.find("\r\n");
	std::string start_line;
	if (pos != std::string::npos)
		start_line = message.substr(0, pos);
	else
		return (setStatusCode(400));

	//<METHOD> <RT> [HTTP/1.1] : 2 espaces sont necessaires pour une requete valide.
	size_t first_space = start_line.find(' ');
	size_t second_space = start_line.find(' ', first_space + 1);
	if (first_space == std::string::npos || second_space == std::string::npos) return (setStatusCode(400));

	//Remplissage des attributs
	_method = start_line.substr(0, first_space);
	_request_target = start_line.substr(first_space + 1, second_space - first_space - 1);
	_protocol = start_line.substr(second_space + 1);

	//Verification de la methode et du protocole : verifier logique (adri) du empty
	if (_method.empty() || _protocol != "HTTP/1.1") return (setStatusCode(400));
	// Méthode non implémentée dans ce serveur (exemple : GIT / HTTP/1.1)
	if (_method != "GET" && _method != "HEAD" && _method != "POST" && _method != "PUT" && _method != "DELETE") return (setStatusCode(501));
}

void	Request::setHeaders(const std::string & message) {

	if (_status_code) return ;
	std::istringstream stream(message);//format compatible pour getline
	std::string line;//buffer remplit par getline

	std::getline(stream, line);//Pour ignorer la start_line

	//Remplissage de la map contenant les headers
	while (std::getline(stream, line))
	{
		//\r : pas de headers ou c'est fini
		if (line == "\r")
			break ;//On sort et on verifie qu'il y a HOST
			
		//Si on ne trouve pas de ":" ou pas de \r dans la ligne ou " :" Error 400
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos || line[line.size() - 1] != '\r' || line[colonPos - 1] == ' ' || line[colonPos + 1] != ' ') return (setStatusCode(400));
		//Suppression de \r
		line.erase(line.size() - 1);

		std::string key = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 1);

		//Suppression des espaces/tab en debut et fin de valeur (HTTP/1.1 trim)
		while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0,1);
		while (!value.empty() && (value[value.size()-1] == ' ' || value[value.size()-1] == '\t')) value.erase(value.size() - 1);

		//Serveur non sensible a la casse : content-length == CONTENT-LENGTH
		for (std::string::size_type i = 0; i < key.size(); ++i) {
			unsigned char c_ = static_cast<unsigned char>(key[i]);//Sinon comp. indef. sur caracteres accentues
			if (std::islower(c_)) key[i] = std::toupper(c_);
		}
		_headers[key] = value;
	}

	// std::cout << "inside setHeaders() 1 -> after filling the header's map :" << getStatusCode() << std::endl;

	//Verifie que HOST est present (obligatoire)
	std::map<std::string, std::string>::const_iterator it = _headers.find("HOST");
    if (it == _headers.end() || it->second.empty()) return (setStatusCode(400));
	// std::cout << "inside setHeaders() 2 -> after filling the header's map :" << getStatusCode() << std::endl;
}

void		Request::addBody(const std::string & message) {

	if (_status_code || _method == "GET" || _method == "HEAD" || _method == "DELETE") return ;
	// Cherche la séparation entre headers et body : "\r\n\r\n"
	size_t pos = message.find("\r\n\r\n");
	// Tout ce qui vient après "\r\n\r\n" est le body
	_body = message.substr(pos + 4);	
}

std::string	Request::getMethod() const {return (_method);}

std::string	Request::getRequestTarget() const {return (_request_target);}

std::string	Request::getProtocol() const {return (_protocol);}

int	Request::getStatusCode() const {return (_status_code);}

std::map<std::string, std::string> Request::getHeaders() const {return (_headers);}

std::string	Request::getBody() const {return (_protocol);}

Request::~Request() {}

std::ostream&	operator<<(std::ostream& os, const Request &request) {

	os << "\nRequest : " << request.getMethod() << " " << request.getRequestTarget() << " " << request.getProtocol() << std::endl;

	std::map<std::string, std::string> tmp_headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = tmp_headers.begin(); it != tmp_headers.end(); ++it) {
		os << it->first << ": " << it->second << std::endl;
	}

	os << "Status code[" << request.getStatusCode() << "]" << std::endl;

	return os;
}
