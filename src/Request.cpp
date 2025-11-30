#include "webserv.hpp"

Request::Request(void): HTTPcontent() {}

//Si changement, penser a changer la version de Response + les prototypes + CGI
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
    
	// std::cout << "--- DEBUG UNCHUNK ---" << std::endl;
	//    std::cout << "Body size: " << _current_body.size() << std::endl;
	//    if (_current_body.size() > 0) {
	//        std::cout << "First 10 chars integers: ";
	//        for (size_t i = 0; i < 10 && i < _current_body.size(); i++) {
	//            std::cout << (int)_current_body[i] << " ";
	//        }
	//        std::cout << "\nFirst 10 chars strings: " << _current_body.substr(0, 10) << std::endl;
	//    }
    while (pos < _current_body.size()) {
        
        // 1. Find the end of the chunk size line
        header_end = _current_body.find("\r\n", pos);
        if (header_end == std::string::npos)
            return setCode(400); // Malformed chunk header

        // 2. Parse the size (Hexadecimal)
        std::string size_str = _current_body.substr(pos, header_end - pos);
        char *endptr;
        chunk_size = std::strtoul(size_str.c_str(), &endptr, 16);

        // Check for invalid hex characters
        if (endptr == size_str.c_str() || *endptr != '\0')
            return setCode(400);

        // 3. Move position to start of data
        pos = header_end + 2; // Skip \r\n

        // 4. Check for the End Chunk (Size 0)
        if (chunk_size == 0) {
            // Check if there is a trailing \r\n after the 0
            if (_current_body.substr(pos) != "\r\n")
                return setCode(400); // Trailing garbage or missing CRLF
            break; // Success
        }

        // 5. Validation: Do we have enough data?
        // We need: chunk_size bytes + 2 bytes for the trailing \r\n
        if (pos + chunk_size + 2 > _current_body.size()) {
            // We shouldn't be here if read_body() did its job correctly,
            // but for safety:
            return setCode(400); 
        }

        // 6. Check the trailing \r\n after data
        if (_current_body.substr(pos + chunk_size, 2) != "\r\n")
            return setCode(400); // Data length mismatch

        // 7. Append data to new body
        new_body += _current_body.substr(pos, chunk_size);

        // 8. Move to next chunk
        pos += chunk_size + 2;
    }

    // Replace the old chunked body with the clean one
    _current_body = new_body;
    
    // Update Content-Length to match the unchunked size
    _content_length = _current_body.size();
}

// void	Request::unchunk_body() {
//
// 	std::string	unchunked_body;
//
// 	std::istringstream stream(_current_body);
// 	std::string line;
// 	std::string line_without;//line without '\r'
//
// 	int		i = 0;
// 	size_t	line_len;
// 	size_t	backr;
// 	double	real_len;
// 	char	*end;
// 	//Remplissage de la map contenant les headers
// 	while (std::getline(stream, line))
// 	{
// 		//\r : pas de headers ou c'est fini
// 		if (line == "0\r")
// 			break ;// Verifier ensuite que la prochaine ligne est \r
//
// 		backr = line.find('\r');
// 		if (backr == std::string::npos)
// 			return setCode(400); // Verifier le code d'erreur
// 		line_without = line.substr(0, backr);
//
// 		if (i % 2 == 0) {
// 			//verifier que toute la string a ete convertit, ou pas un nb a virgule
// 			//floor : fonction interdite I guess, supprimer librairie math
// 			real_len = std::strtod(line_without.c_str(), &end);
// 			if (*end != '\0' || std::floor(real_len) != real_len)
// 				return setCode(400);
// 		}
// 		else {
// 			//Checker que line_len == real_len
// 			line_len = line_without.size();
// 			if (line_len != real_len)
// 				return setCode(400);
// 			//Si tout va bien unchuncked_body.append(line_without); (doit etre init ?)
// 			unchunked_body.append(line_without);
// 		}
//
// 		i++;
// 	}
//
// 	//Checker que la dernier ligne est bien "\r" sinon error 400
// 	if(std::getline(stream, line))
// 		return setCode(400);
// 	if (line != "\r")
// 		return setCode(400);
//
// 	_current_body = unchunked_body;
// }

//400 Bad Request : on recupere la start line : La 1ere ligne ne respecte pas la syntaxe <METHOD> <PATH> HTTP/<VERSION>\r\n (trop d'espaces, trop de mots, caracteres interdits). Ou carrement pas de 1er mot
void	Request::setStartLine() {

	if (_code) return ;
	// if (message.find("\r\n\r\n") == std::string::npos) return (setCode(400)); // pas de fin de headers, donc requete mal specifiee

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

	// os << "\nRequest : " << request.getMethod() << " " << request.getURI() << " " << request._protocol << std::endl;

	std::map<std::string, std::string> tmp_headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = tmp_headers.begin(); it != tmp_headers.end(); ++it) {
		os << it->first << ": " << it->second << std::endl;
	}

	os << "Status code[" << request.getCode() << "]" << std::endl;

	return os;
}
