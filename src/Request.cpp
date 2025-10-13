#include "webserv.hpp"

Request::Request(void) {
	reset();
}

/*
Request::Request(const std::string & message, const int & s_c): _code(s_c) {

	setStartLine(message);
	setHeaders(message);
	// addBody(message);//setBody hors du constructeur
}
*/

void	Request::reset() {
	_code = 0;
	_current_header.clear();
	_method.clear();
	_request_target.clear();
	_protocol.clear();
	_headers.clear();
	_remainder.clear();
	_body.clear();
}

void	Request::append_to_header(char buff[BUFF_SIZE], int bytes) {

	_current_header.append(buff, (size_t)bytes);
}

void	Request::setStatusCode(const int & st) {
	_code = st;
}

void	Request::parse_header() {
	setStartLine();
	setHeaders();
}

//400 Bad Request : on recupere la start line : La 1ere ligne ne respecte pas la syntaxe <METHOD> <PATH> HTTP/<VERSION>\r\n (trop d'espaces, trop de mots, caracteres interdits). Ou carrement pas de 1er mot
void	Request::setStartLine() {

	if (_code) return ;
	// if (message.find("\r\n\r\n") == std::string::npos) return (setStatusCode(400)); // pas de fin de headers, donc requete mal specifiee

	// Recuperation de la 1ere ligne
	const std::string &message = _current_header;
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

// void		Request::addBody(const std::string & message) {

// 	if (_code || _method == "GET" || _method == "HEAD" || _method == "DELETE") return ;
// 	// Cherche la séparation entre headers et body : "\r\n\r\n"
// 	size_t pos = message.find("\r\n\r\n");
// 	// Tout ce qui vient après "\r\n\r\n" est le body
// 	_body = message.substr(pos + 4);	
// }

std::string		Request::getParentDirectory(const std::string &path) const {

    std::string::size_type pos = path.rfind('/');
    if (pos == std::string::npos) 
        return "."; // pas de / → répertoire courant
    else if (pos == 0)
        return "/"; // le parent de /fichier est /
    else
        return path.substr(0, pos);
}

std::string		Request::getCurrentHeader() const {

	return _current_header;
}

void	Request::setRemainder(std::string remainder) {
	
	_remainder = remainder;
}

void	Request::checkAccessAndPermissions() {

	if (_code) return;

	struct stat request_target_properties;

	// Récupération des métadonnées du fichier/répertoire.
	// stat() échoue si la ressource n'existe pas ou n'est pas accessible.
	if (stat(_request_target.c_str(), &request_target_properties) == -1) {

		// ========== ENOENT ==========
		// La ressource (fichier ou dossier) n’existe pas.
		// - Pertinent pour : GET, HEAD, DELETE → on ne peut pas lire/supprimer ce qui n’existe pas → 404
		// - Pour PUT/POST : ce n’est *pas une erreur*, car ces méthodes peuvent créer la ressource.
		if (errno == ENOENT) {
			if (_method == "GET" || _method == "HEAD" || _method == "DELETE")
				setStatusCode(404);// ========== ENOENT ==========
		}
		else if (errno == EACCES)
			setStatusCode(403);// ========== EACCES ==========
		else if (errno == ENOTDIR || errno == ELOOP)
			setStatusCode(404);// ========== ENOTDIR / ELOOP ==========
		else if (errno == ENAMETOOLONG)
			setStatusCode(414);// ========== ENAMETOOLONG ==========
		else
			setStatusCode(500);// ========== Cas restants ==========

		// ========== ENOENT ==========
		// La ressource (fichier ou dossier) n’existe pas.
		// - Pertinent pour : GET, HEAD, DELETE → on ne peut pas lire/supprimer ce qui n’existe pas → 404
		// - Pour PUT/POST : ce n’est *pas une erreur*, car ces méthodes peuvent créer la ressource.
		// ========== EACCES ==========
		// Accès aux metadonnees refusé :
		// - Soit à cause d'un répertoire du chemin (ex : /upload/ a des droits 000) : le droit x (execution) n'est pas present sur au moins un des repertoire du chemin
		// → 403 Forbidden pour toutes les méthodes.
		// ========== ENOTDIR / ELOOP ==========
		// Le chemin contient un composant qui n’est pas un répertoire :
		// Exemple : /dossier/fichier.txt/truc.json → fichier.txt n’est pas un répertoire.
		// Ou bien : boucle de liens symboliques (ELOOP).
		// → Le chemin est invalide → 404 Not Found.
		// ========== ENAMETOOLONG ==========
		// Un des éléments du chemin, ou le chemin complet, dépasse la longueur maximale.
		// → Mauvaise requête → 414 URI Too Long.
		// ========== Cas restants ==========
		// Erreurs internes au serveur, ex :
		// - EFAULT : pointeur invalide
		// - ENOMEM : manque de mémoire
		// - EIO : erreur d’entrée/sortie
		// Ces cas relèvent d’un problème serveur, pas du client.
	}

	// ========= Si on arrive ici =========
	// stat() a réussi → la ressource existe et on a les droits suffisants pour la "voir".
	// Mais il reste à vérifier :
	//  - Pour GET/HEAD → droit de lecture sur la ressource
	//  - Pour PUT/POST → droit d’écriture sur la ressource ou le répertoire parent
	//  - Pour DELETE → droit d’écriture sur le répertoire parent
	// Ces vérifications se font en utilisant access() ou les bits de request_target_properties.st_mode.

    // ---------------- GET / HEAD ----------------
    if (_method == "GET" || _method == "HEAD") {
        if (access(_request_target.c_str(), R_OK) != 0) { // Vérifie que le fichier est lisible
            setStatusCode(403);
            return;
        }
    }

    // ---------------- PUT / POST ----------------
    else if (_method == "PUT" || _method == "POST") {
        // Si le fichier existe : vérifier qu’il est modifiable
        if (access(_request_target.c_str(), F_OK) == 0) { 
            if (access(_request_target.c_str(), W_OK) != 0) { // Droit d’écriture manquant
                setStatusCode(403);
                return;
            }
        }
        // Si le fichier n’existe pas : vérifier le répertoire parent
        else {
            std::string parentDir = getParentDirectory(_request_target); // Fonction à implémenter
            if (access(parentDir.c_str(), W_OK | X_OK) != 0) { // Vérifie qu’on peut créer un fichier
                setStatusCode(403);
                return;
            }
        }
    }

	else if (_method == "DELETE") {
		std::string parentDir = getParentDirectory(_request_target);
		// Vérifie qu’on peut écrire et traverser le répertoire parent
		if (access(parentDir.c_str(), W_OK | X_OK) != 0) {
			setStatusCode(403);
			return;
		}
		// Le fichier existe déjà, donc aucune autre vérification nécessaire ici
	}


}

//Checker taille du fichier ?

std::string	Request::getMethod() const {return (_method);}

std::string	Request::getRequestTarget() const {return (_request_target);}

std::string	Request::getProtocol() const {return (_protocol);}

int	Request::getStatusCode() const {return (_code);}

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
