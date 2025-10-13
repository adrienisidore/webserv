#include "webserv.hpp"

// Request::Request(void): HTTPcontent(void) {
Request::Request(void): HTTPcontent() {
	reset();
}
// Request::Request(const std::string & message, const int & code): HTTPcontent(code) {
Request::Request(const std::string & message, const int & code): HTTPcontent(code) {

	setStartLine(message);
	setHeaders(message);
	checkPermissions();
	// addBody(message);//setBody hors du constructeur

}

void	Request::setCode(const int & code) {
	_code = code;
}

void	Request::reset() {
	setCode(0);
	_current_header.clear();
	_method.clear();
	_target.clear();
	_headers.clear();
	_remainder.clear();
	_body.clear();
}

//400 Bad Request : on recupere la start line : La 1ere ligne ne respecte pas la syntaxe <METHOD> <PATH> HTTP/<VERSION>\r\n (trop d'espaces, trop de mots, caracteres interdits). Ou carrement pas de 1er mot
void	Request::setStartLine(const std::string & message) {

	if (_code) return ;
	// if (message.find("\r\n\r\n") == std::string::npos) return (setCode(400)); // pas de fin de headers, donc requete mal specifiee

	// Recuperation de la 1ere ligne
	size_t pos = message.find("\r\n");
	std::string start_line;
	if (pos != std::string::npos)
		start_line = message.substr(0, pos);
	else
		return (setCode(400));

	//<METHOD> <RT> [HTTP/1.1] : 2 espaces sont necessaires pour une requete valide.
	size_t first_space = start_line.find(' ');
	size_t second_space = start_line.find(' ', first_space + 1);
	if (first_space == std::string::npos || second_space == std::string::npos)
		return (setCode(400));

	//Remplissage des attributs
	_method = start_line.substr(0, first_space);
	_target = start_line.substr(first_space + 1, second_space - first_space - 1);

	//Verification de la methode et du protocole : verifier logique (adri) du empty
	if (_method.empty() || _protocol != start_line.substr(second_space + 1)) return (setCode(400));
	// Méthode non implémentée dans ce serveur (exemple : GIT / HTTP/1.1)
	if (_method != "GET" && _method != "HEAD" && _method != "POST" && _method != "PUT" && _method != "DELETE")
		return (setCode(501));
}

void	Request::setHeaders(const std::string & message) {

	if (_code) return ;
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
			return (setCode(400));
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

	// std::cout << "inside setHeaders() 1 -> after filling the header's map :" << getCode() << std::endl;

	//Verifie que HOST est present (obligatoire)
	std::map<std::string, std::string>::const_iterator it = _headers.find("HOST");
    if (it == _headers.end() || it->second.empty()) return (setCode(400));
	// std::cout << "inside setHeaders() 2 -> after filling the header's map :" << getCode() << std::endl;
}

// void		Request::addBody(const std::string & message) {

// 	if (_code || _method == "GET" || _method == "HEAD" || _method == "DELETE") return ;
// 	// Cherche la séparation entre headers et body : "\r\n\r\n"
// 	size_t pos = message.find("\r\n\r\n");
// 	// Tout ce qui vient après "\r\n\r\n" est le body
// 	_body = message.substr(pos + 4);	
// }

std::string		Request::parentDir(const std::string &path) const {

    std::string::size_type pos = path.rfind('/');
    if (pos == std::string::npos) 
        return "."; // pas de / → répertoire courant
    else if (pos == 0)
        return "/"; // le parent de /fichier est /
    else
        return path.substr(0, pos);
}

//Faut il remettre errno a 0 apres ?
void	Request::checkPermissions() {

	if (_code) return;

	struct stat target_properties;

	// stat() : Récupération des métadonnées du fichier/répertoire, échoue si la ressource n'existe pas ou n'est pas accessible.
	if (stat(_target.c_str(), &target_properties) == -1) {

		if (errno == ENOENT && (_method == "GET" || _method == "HEAD" || _method == "DELETE"))
		    setCode(404);// ========== ENOENT ========== La ressource (fichier ou dossier) n’existe pas.
		else if (errno == EACCES)
			setCode(403);// ========== EACCES ========== Accès aux metadonnees refusé
		else if (errno == ENOTDIR || errno == ELOOP)
			setCode(404);// ========== ENOTDIR / ELOOP ========== Le chemin contient un composant qui n’est pas un répertoire 
		else if (errno == ENAMETOOLONG)
			setCode(414);// ========== ENAMETOOLONG ========== Un des éléments du chemin, ou le chemin complet, dépasse la longueur maximale.
		else
			setCode(500);// ========== Cas restants ========== Erreurs internes au serveur (memoire, ...)

		// ========== ENOENT ==========
		// - Pertinent pour : GET, HEAD, DELETE → on ne peut pas lire/supprimer ce qui n’existe pas → 404
		// - Pour PUT/POST : ce n’est *pas une erreur*, car ces méthodes peuvent créer la ressource.
		// ========== EACCES ==========
		// - A cause d'un répertoire du chemin (ex : /upload/ a des droits 000) : le droit x (execution) n'est pas present sur au moins un des repertoire du chemin
		// → 403 Forbidden pour toutes les méthodes.
		// ========== ENOTDIR / ELOOP ==========
		// Exemple : /dossier/fichier.txt/truc.json → fichier.txt n’est pas un répertoire.
		// Ou bien : boucle de liens symboliques (ELOOP).
		// → Le chemin est invalide → 404 Not Found.
		// ========== ENAMETOOLONG ==========
		// → Mauvaise requête → 414 URI Too Long.
	}
	if (_code) return;
	// ========= Si on arrive ici =========
	// stat() a réussi → SOIT la ressource existe et on a acces a ses metadonnees, soit elle n'existe pas mais on va pouvoir la creer.
	// Il reste à vérifier :
	//  - Pour GET/HEAD → droit de lecture sur la ressource
	//  - Pour PUT/POST → droit d’écriture sur la ressource ou le répertoire parent
	//  - Pour DELETE → droit d’écriture sur le répertoire parent

    if ((_method == "GET" || _method == "HEAD") && access(_target.c_str(), R_OK) != 0) return(setCode(403)); // Vérifie que le fichier est lisible
	else if (_method == "PUT" || _method == "POST") {
		//Si fichier existant mais non modifiable || Repertoire parent ne permet pas de creer un fichier
		if ((access(_target.c_str(), F_OK) == 0 && access(_target.c_str(), W_OK) != 0)
			|| (access(_target.c_str(), F_OK) != 0 && access(parentDir(_target).c_str(), W_OK | X_OK) != 0))
			return (setCode(403));
	}
	else if (_method == "DELETE" && access(parentDir(_target).c_str(), W_OK | X_OK) != 0)
		return (setCode(403));//On a acces au repertoire parent pour faire des modifications


	//Checker taille du fichier ?

}


Request::~Request() {}

std::ostream&	operator<<(std::ostream& os, const Request &request) {

	os << "\nRequest : " << request.getMethod() << " " << request.getTarget() << " " << request.getProtocol() << std::endl;

	std::map<std::string, std::string> tmp_headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = tmp_headers.begin(); it != tmp_headers.end(); ++it) {
		os << it->first << ": " << it->second << std::endl;
	}

	os << "Status code[" << request.getCode() << "]" << std::endl;

	return os;
}
