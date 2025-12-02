#include "webserv.hpp"

Request::Request(void): HTTPcontent() {}

void	Request::copyFrom(HTTPcontent& other) {
		_code = other.getCode();
		_method = other.getMethod();
		_URI = other.getURI();
		_config = other.getConfig();
		_headers = other.getHeaders();
}

void	Request::parse_header() {
	setStartLine();
	setHeaders();
}

void Request::unchunk_body() {
    std::string new_body = "";
    size_t  pos = 0;
    size_t  header_end;
    size_t  chunk_size;
    
    while (pos < _current_body.size()) {
        
        header_end = _current_body.find("\r\n", pos);
        if (header_end == std::string::npos)
            return setCode(400);

        // Parse the size (Hexadecimal)
        std::string size_str = _current_body.substr(pos, header_end - pos);
        char *endptr;
        chunk_size = std::strtoul(size_str.c_str(), &endptr, 16);

        if (endptr == size_str.c_str() || *endptr != '\0')
            return setCode(400);

        // Move position to start of data
        pos = header_end + 2; // Skip \r\n

        // Check for the End Chunk (Size 0)
        if (chunk_size == 0) {
            if (_current_body.substr(pos) != "\r\n")
                return setCode(400);
            break;
        }

        // We need: chunk_size bytes + 2 bytes for the trailing \r\n
        if (pos + chunk_size + 2 > _current_body.size()) {
            return setCode(400); 
        }

        if (_current_body.substr(pos + chunk_size, 2) != "\r\n")
            return setCode(400);

        new_body += _current_body.substr(pos, chunk_size);
        pos += chunk_size + 2;
    }

    // Replace the old chunked body with the clean one
    _current_body = new_body;
    
    // Update Content-Length to match the unchunked size
    _content_length = _current_body.size();
}

void	Request::setStartLine() {

	if (_code) return ;

	// Recuperation de la 1ere ligne
	const std::string &message = _current_header;
	size_t pos = message.find("\r\n");
	std::string start_line;
	if (pos != std::string::npos)
		start_line = message.substr(0, pos);
	else
		return setCode(400);

	//<METHOD> <RT> [HTTP/1.1] : 2 espaces sont necessaires pour une requete valide.
	size_t first_space = start_line.find(' ');
	size_t second_space = start_line.find(' ', first_space + 1);
	if (first_space == std::string::npos || second_space == std::string::npos)
		return setCode(400);
	//Remplissage des attributs
	_method = start_line.substr(0, first_space);
	_URI = start_line.substr(first_space + 1, second_space - first_space - 1);
	//Verification de la methode et du protocole : verifier logique (adri) du empty
	if (_method.empty() || _protocol != start_line.substr(second_space + 1)) return (setCode(400));
	// Méthode non implémentée dans ce serveur (exemple : GIT / HTTP/1.1)
	if (_method != "GET" && _method != "HEAD" && _method != "POST" && _method != "DELETE")// PUT
		return setCode(501);
}

void	Request::setHeaders() {

	if (_code) return ;
	const std::string &message = _current_header;
	std::istringstream stream(message);
	std::string line;

	std::getline(stream, line);// ignorer la start_line

	//Remplissage de la map contenant les headers
	while (std::getline(stream, line))
	{
		//\r : pas de headers ou c'est fini
		if (line == "\r")
			break ;//On sort et on verifie qu'il y a HOST
			
		//Si on ne trouve pas de ":" ou pas de \r dans la ligne ou " :" Error 400
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos || line[line.size() - 1] != '\r' || line[colonPos - 1] == ' ' || line[colonPos + 1] != ' ')
			return setCode(400);
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

	std::map<std::string, std::string>::const_iterator it = _headers.find("HOST");
    if (it == _headers.end() || it->second.empty()) return (setCode(400));
}

Request::~Request() {}

std::ostream&	operator<<(std::ostream& os, const Request &request) {

	std::map<std::string, std::string> tmp_headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = tmp_headers.begin(); it != tmp_headers.end(); ++it) {
		os << it->first << ": " << it->second << std::endl;
	}

	os << "Status code[" << request.getCode() << "]" << std::endl;

	return os;
}
