#include "webserv.hpp"

//405: Methode interdite pour la ressource

Response::Response(): HTTPcontent() {
	_autoindex = false;
}

//Si changement, penser a changer la version de Request + les prototypes + CGI
void	Response::copyFrom(HTTPcontent& other) {
		_code = other.getCode();
		_method = other.getMethod();
		_URI = other.getURI();
		_config = other.getConfig();
		_headers = other.getHeaders();
		_autoindex = false;
}

Response::~Response() {}

//fonction static
static std::string		parentDir(const std::string &path) {
	std::string::size_type pos = path.rfind('/');
	if (pos == std::string::npos) 
		return "."; // pas de / → répertoire courant
	else if (pos == 0)
		return "/"; // le parent de /fichier est /
	else
		return path.substr(0, pos);
}


void		Response::checkAllowedMethods() {

	// On verifie allowed-methods
	/* CHECK ALLOWED METHODS DELTE POST :
	s'il existe une directive allowed-method dans la locationConfig et que _method n'en fait pas partie
	alors on setCode

	*/
}

std::string		Response::try_multiple_indexes(std::vector<std::string> indexes) {
	std::string	idx_path;

	for (std::vector<std::string>::iterator idx = indexes.begin(); idx != indexes.end(); ++idx) {
		idx_path = _path + *idx;
		checkPermissions(idx_path);
		if (getCode() == 0) {
			return *idx;
		}
		else
			setCode(0);
	}
	return "";
}

//1) https://www.alimnaqvi.com/blog/webserv
// A partir de la locationConfig on construit un chemin vers un fichier
// 1) regarder si root/URI correspond a un fichier, si oui _path = ROOT/URI
// 2) regarder si root/URI correspond a un dossier, auquel cas on construit le path a partir des index
// 3) regarger si root/URI correspond a un dossier sans index, mais avec autoindex "on", auquel cas on
// construit un fichier html liste deroulante et _path = root/URI/"ce fichier"

void			Response::buildPath() {

	// A replacer au bon endroit
	if (getCode())
		return;

	std::vector<std::string>	indexes;

	//On regarde s'il existe une location nomme URI
	if (_config.getDirective("uri") != _URI || _URI.empty())
		return setCode(404);

	// root par defaut
	std::string root = _config.getDirective("root");
	if (root.empty())
		root = "./ressources";
	
	
	_path = root + _URI;

	// -----------------Differencie les types de requetes------------------------
	if (_URI[_URI.size() - 1] == '/') {
		// Si URI == directory

		std::string index = _config.getDirective("index");
		if (index.empty()) {
			// Si pas d'index

			if (_method == "GET" && _config.getDirective("autoindex") == "on")
				_autoindex = true;
			else
				return setCode(403);
		}
		else {
			indexes = split(index, ' ');
			index = try_multiple_indexes(indexes);
			
			if (index.empty())
				return setCode(404);
			else
				_path += index;
			
		}
	}
}

// A noter: on pourrait implementer l'URL encoding (%20, ?, +, etc..). Pas demande mais ca peut etre interessant pour comprendre comment fonctionnent les URLs
// fonction static
static bool	is_valid_path(std::string filename) {

	if (filename.empty() ||
		filename.find("%") != std::string::npos ||
		filename.find("..") != std::string::npos ||
		filename.find("~") != std::string::npos ||
		filename.find("#") != std::string::npos)
		return false;
	return true;
}

void	Response::checkPermissions(std::string path) {

	if (_code) return;//Si j'ai un code ici a priori c'est forcement un code d'erreur

	struct stat path_properties;//path_properties

	// stat() : Récupération des métadonnées du fichier/répertoire, échoue si la ressource n'existe pas ou n'est pas accessible.
	if (_method != "GET" && _method != "HEAD" && _method != "DELETE" && _method != "POST")
		setCode(405);

	else if (!is_valid_path(path))
		setCode(400);

	else if (stat(path.c_str(), &path_properties) == -1) {

		int er = errno;

		if (er == ENOENT && (_method == "GET" || _method == "HEAD" || _method == "DELETE"))
		    setCode(404);// ========== ENOENT ========== La ressource (fichier ou dossier) n’existe pas.
		else if (er == EACCES)
			setCode(403);// ========== EACCES ========== Accès aux metadonnees refusé
		else if (er == ENOTDIR || er == ELOOP)
			setCode(404);// ========== ENOTDIR / ELOOP ========== Le chemin contient un composant qui n’est pas un répertoire 
		else if (er == ENAMETOOLONG)
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

    if ((_method == "GET" || _method == "HEAD") && access(path.c_str(), R_OK) != 0) return(setCode(403)); // Vérifie que le fichier est lisible
	else if (_method == "PUT" || _method == "POST") {
		//Si fichier existant mais non modifiable || Repertoire parent ne permet pas de creer un fichier
		if ((access(path.c_str(), F_OK) == 0 && access(path.c_str(), W_OK) != 0)
			|| (access(path.c_str(), F_OK) != 0 && access(parentDir(path).c_str(), W_OK | X_OK) != 0))
			return setCode(403);
	}
	else if (_method == "DELETE" && access(parentDir(path).c_str(), W_OK | X_OK) != 0)
		return setCode(403);//On a acces au repertoire parent pour faire des modifications
}

bool	Response::is_cgi() {
	std::string s_ = _config.getDirective("cgi_handler");
	std::string::size_type pos = s_.find(' ');
	bool		ok = false;

	if (pos == std::string::npos) {
		return false;
	} else {
		// check que le 1er arg de cgi_handler est bien une extension  (ex .py)
		std::string extension  = s_.substr(0, pos);
		ok =
        extension.size() >= 2 &&
        extension[0] == '.' &&
        extension.find('.', 1) == std::string::npos;
		// check que le fichier indique dans la location finit bien par la meme extension
		ok = ok && extension == _path.substr(_path.rfind('.'));
		// check que le 2eme arg de cgi_handler va bien permettre de lancer l'exec
		checkPermissions(s_.substr(pos + 1));
		return ok;
	}
}

// fetch s'assure de la compatibilite entre la config de la location et la requete :

// renvoie -1 si le body s'est remplie avec une page d'erreur
// renvoie 0 si le body s'est rempli avec une page statique html (tout s'est bien passé)
// renvoie le fd du pipe si un CGI a été enclenché (tout s'est bien passé)
int	Response::fetch() {

	checkAllowedMethods();

	// Host doit etre formalise en mode "IP:Port" sinon erreur (peut etre deja present ailleurs)
	std::map<std::string, std::string>::const_iterator it = _headers.find("HOST");
	if (it->second != _config.getDirective("listen")) {
		setCode(400);
		return (0);
	}
		
	buildPath();

	checkPermissions(_path);

	// Je regarde si la LocationConfig indique que ce path correspond a une cgi.
	//Si oui alors :
	if (is_cgi() && !getCode()) {
		try {
			this->_cgi.copyFrom(*this);
			this->_cgi.buildEnv();
			this->_cgi.buildArgv();
			this->_cgi.openPipes();
			// Si tout s'est bien passé je retourne le pipe d'écriture (ou lecture?) de l'enfant
			return this->_cgi._outpipe[0];
		} catch (std::exception &er) {
			setCode(500);
			return (0);
		}
	}
	// Si non alors:
		// je remplie le body avec la page statique de l'URL demandée et je renvoie 0
	return 0;
}


static std::string loadFile(const std::string &path) {
	std::ifstream f(path.c_str());
	if (!f.is_open())
		return "<html><body>File error</body></html>";
	std::string content, line;
	while (std::getline(f, line))
		content += line + "\n";
	return content;
}

void Response::_error_() {
	// Pages HTML des erreurs (chargées une seule fois)
	static std::map<int, std::string> pages;
	if (pages.empty()) {
		pages[400] = loadFile("errors/400.html");
		pages[403] = loadFile("errors/403.html");
		pages[404] = loadFile("errors/404.html");
		pages[405] = loadFile("errors/405.html");
		pages[409] = loadFile("errors/409.html");
		pages[411] = loadFile("errors/411.html");
		pages[413] = loadFile("errors/413.html");
		pages[414] = loadFile("errors/414.html");
		pages[500] = loadFile("errors/500.html");
		pages[501] = loadFile("errors/501.html");
	}

	// Reason phrases (HTTP/1.1)
	static std::map<int, std::string> reason;
	if (reason.empty()) {
		reason[400] = "Bad Request";
		reason[403] = "Forbidden";
		reason[404] = "Not Found";
		reason[405] = "Method Not Allowed";
		reason[409] = "Conflict";
		reason[411] = "Length Required";
		reason[413] = "Payload Too Large";
		reason[414] = "URI Too Long";
		reason[500] = "Internal Server Error";
		reason[501] = "Not Implemented";
	}

	// Sélection du corps
	std::map<int, std::string>::iterator it = pages.find(_code);
	const std::string &body = (it != pages.end()) ? it->second : pages[500];

	// Reason phrase
	std::string rp = "Error";
	if (reason.find(_code) != reason.end())
		rp = reason[_code];

	std::ostringstream out;
	out << "HTTP/1.1 " << _code << " " << rp << "\r\n"
		<< "Content-Type: text/html\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;

	_current_body = out.str();
}




// Gerer les redirections redirection directive -> vers une nouvelle location, ATTENTION AUX REDIRECTIONS INFINIES


// post le _body de _response, le vide et prerempli le nouveau body pour l'envoyer au client
void		Response::_post_() {

	// ATTENTION :	si une nouvelle ressource est cree : 201
	// 				si j'ecrase un fichier alors : 200 ou 204
	int	success_code;
	if (access(_path.c_str(), F_OK) == 0)
		success_code = 204;// 200 (body optionnel indiquant que c'est ok) ou 204 (pas de body)
	else
		success_code = 201;// body optionnel representant la ressource

	// 1) si le fichier existe on l'ecrase, sinon on le cree (on y inclut _body)
	// Créer (ou écraser) le fichier: -rw-r--r-- (0644), affecté par umask
    int fd = open(_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
		setCode(405);
        return ;
    }

    if (write(fd, _current_body.c_str(), _current_body.size()) < 0) {
        close(fd);
        return;
    }
    close(fd);

	if (getCode())
		return _error_();

	// size_t content_length = _current_body.length();// si je ne renvoie pas de corps, alors pas de content_length
	// 2) on erase le body pour le remplir avec les nouvelles infos
	setCurrentBody("");
	// 3) on set les headers appropries
	std::map<std::string, std::string>::const_iterator it = _headers.find("CONTENT-TYPE");
	_current_body = _protocol + " " + std::to_string(success_code) + " ";
	if (success_code == 201)
		_current_body += "Created\r\n";
	else if (success_code == 204)
		_current_body += "No Content\r\n";
	if (success_code == 201) {
		if (it == _headers.end())
			_current_body = _current_body + "Content-Type: " + "Unknown\r\n";
		else
			_current_body = _current_body + "Content-Type: " + it->second + "\r\n";
	}

	// _current_body += "Content-Length: " + std::to_string(content_length) + "\r\n";
	if (success_code == 201)
		_current_body +="Location: " + _path + "\r\n";// Location: /api/ressources/123 si dans la location /api/ressources j'ai cree 123
	_current_body += "\r\n";// On decide arbitrairement de ne pas renvoyer de body ici.
}

void		Response::_delete_() {

    if (std::remove(_path.c_str()) == 0)
        return; // supprimé (fichier ou dossier vide)

    const int e = errno;
    if (e == ENOENT)
		setCode(404);	// n'existe pas
    if (e == EACCES || e == EPERM)
		setCode(403); // droits insuffisants
    if (e == ENOTEMPTY || e == EEXIST) 
		setCode(409); // dossier non vide
	else
		setCode(500);
	
	setCurrentBody("");

	if (getCode())
		return _error_();
	
	// 3) on set les headers appropries

	int success_code = 204; 
	setCurrentBody(""); 
	_current_body = _protocol + " " + std::to_string(success_code) + " No Content\r\n";
	_current_body += "\r\n";
}

#include <string>
#include <map>


static const std::map<std::string, std::string>& get_mime_map() {
	static std::map<std::string, std::string> mime_types;
	
	// Initialisation conditionnelle (C++98 safety check pour static init)
	if (mime_types.empty()) {
		// Encodage de texte et HTML
		mime_types[".html"] = "text/html";
		mime_types[".htm"] = "text/html";
		mime_types[".css"] = "text/css";
		mime_types[".js"] = "application/javascript";
		mime_types[".txt"] = "text/plain";
		
		// Images
		mime_types[".jpg"] = "image/jpeg";
		mime_types[".jpeg"] = "image/jpeg";
		mime_types[".png"] = "image/png";
		mime_types[".gif"] = "image/gif";
		mime_types[".ico"] = "image/x-icon";
		
		// Documents et autres
		mime_types[".pdf"] = "application/pdf";
		mime_types[".xml"] = "application/xml";
		mime_types[".json"] = "application/json";
	}
	return mime_types;
}

static std::string get_mime_type_from_path(const std::string& path) {
	const std::map<std::string, std::string>& mime_map = get_mime_map();
	
	// 1. Trouver le dernier point ('.')
	size_t pos = path.find_last_of('.');
	
	// Si pas de point ou si le point est le dernier caractère, utiliser le type par défaut
	if (pos == std::string::npos || pos == path.length() - 1) {
		return "application/octet-stream";
	}
	
	// 2. Extraire l'extension (y compris le point)
	// Nous présumons ici que cette extension est déjà en minuscules.
	std::string ext = path.substr(pos);
	
	// 3. Rechercher dans la map
	std::map<std::string, std::string>::const_iterator it = mime_map.find(ext);
	
	if (it != mime_map.end()) {
		return it->second;
	}
	
	// 4. Renvoyer le type par défaut pour les fichiers inconnus
	return "application/octet-stream";
}

void	Response::_get_() {
	
	std::ifstream ifs(_path.c_str(), std::ios::in);
    
	if (!ifs)
		return (setCode(404));
    _current_body.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    if (!ifs.eof() && ifs.fail())
		return setCode(500);
	
	// Fermer le descripteur de fichier dès que la lecture est complète.
	ifs.close();
	
	// Vérification finale des erreurs avant de construire la réponse
	if (getCode())
		return _error_();
	
	// --- Construction de la réponse HTTP/1.1 (200 OK) ---
	
	// 2. Sauvegarder le contenu du fichier et sa taille
	std::string response_body = _current_body; // Contient les données du fichier
	size_t content_length = response_body.length(); // Taille exacte du corps de la réponse
	
	// 3. Vider _current_body pour y CONSTRUIRE les en-têtes de la réponse
	_current_body.clear(); 
	
	// 4. Ligne de statut (200 OK)
	_current_body += _protocol + " 200 OK\r\n";
	
	// 5. En-tête Content-Type (Détermination dynamique)
	std::string content_type = get_mime_type_from_path(_path); 
	_current_body += "Content-Type: " + content_type + "\r\n";
	
	// 6. En-tête Content-Length (Obligatoire pour les connexions non chunked)
	_current_body += "Content-Length: " + std::to_string(content_length) + "\r\n";

	// 7. En-tête Connection (Fermeture de la connexion après l'envoi)
	std::map<std::string, std::string>::const_iterator it = _headers.find("CONNECTION");
	if (it->second == "close")
		_current_body += "Connection: close\r\n"; 

	// 8. Séparateur double \r\n (Fin des en-têtes)
	_current_body += "\r\n";
	
	// 9. Corps de la Réponse (Le contenu du fichier)
	_current_body += response_body;
}
