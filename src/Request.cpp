#include "webserv.hpp"

Request::Request(const std::string & message): _status_code(0) {

	//400 Bad Request : on recupere la start line.
	//La 1ere ligne ne respecte pas la syntaxe <METHOD> <PATH> HTTP/<VERSION>\r\n (trop d'espaces, trop de mots, caracteres interdits)
	//Ou carrement qu'il n'y a pas de premier mot (mais un espace ou une chaine vide a la place)
	
	//501 Not Implemented : on recupere la start line.
	//Si le 1er mot ne fait pas partie des methodes supportes (GE / HTTP/1.1)
}

Request::~Request() {}

std::ostream&	operator<<(std::ostream& os, const Request &request)
{
	// os << request.toFloat();//On injecte dans os le nombre qui a ete retransforme
	//dans sa valeur d'origine
	return (os);
}