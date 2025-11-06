#include "webserv.hpp"

//405: Methode interdite pour la ressource

Response::Response(): HTTPcontent() {}

//Si changement, penser a changer la version de Request + les prototypes + CGI
void	Response::copyFrom(HTTPcontent& other) {
		_code = other.getCode();
		_method = other.getMethod();
		_URI = other.getURI();
		_config = other.getConfig();
		_headers = other.getHeaders();
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

//1) https://www.alimnaqvi.com/blog/webserv
void			Response::buildPath() {

	// Host doit etre formalise en mode "IP:Port" sinon erreur
	//ATTENTION SI JE FAIS PAS D'ITERATOR CA CREE UNE NOUVELLE CLE
	std::map<std::string, std::string>::const_iterator it = _headers.find("HOST");
	if (it->second != _config.getDirective("listen"))
		return setCode(400);

	//On regarde si dans ce server il existe une location == _URI ==> sinon setCode()
	std::string location = _config.getDirective(_URI);
	if (location == "")
		return setCode(404);

	//On construit le path (root + _URI ou / + _URI) et on check les permissions  ==> sinon setCode()
	//root obligatoire ?
	_path = _config.getDirective("root") + _URI;
	
	//On gere HEAD ?? Pour les siege de curl
	if (this->getMethod() == "GET")
	{

		//On check si index file : si oui on retourne son chemin
		this->checkPermissions();//?

	}
	if (this->getMethod() == "POST")
	{

		//Checker si y'a une logic server qui s'appelle _URI.
		//Ou si y'a un CGI
		//Si pas de CGI on regarde si y'a une directive upload_store

	}

	if (this->getMethod() == "DELETE")
	{


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

void	Response::checkPermissions() {

	if (_code) return;//Si j'ai un code ici a priori c'est forcement un code d'erreur

	struct stat path_properties;//path_properties

	// stat() : Récupération des métadonnées du fichier/répertoire, échoue si la ressource n'existe pas ou n'est pas accessible.
	if (_method != "GET" && _method != "HEAD" && _method != "DELETE" && _method != "POST")
		setCode(405);

	else if (!is_valid_path(_path))
		setCode(400);

	else if (stat(_path.c_str(), &path_properties) == -1) {

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

    if ((_method == "GET" || _method == "HEAD") && access(_path.c_str(), R_OK) != 0) return(setCode(403)); // Vérifie que le fichier est lisible
	else if (_method == "PUT" || _method == "POST") {
		//Si fichier existant mais non modifiable || Repertoire parent ne permet pas de creer un fichier
		if ((access(_path.c_str(), F_OK) == 0 && access(_path.c_str(), W_OK) != 0)
			|| (access(_path.c_str(), F_OK) != 0 && access(parentDir(_path).c_str(), W_OK | X_OK) != 0))
			return setCode(403);
	}
	else if (_method == "DELETE" && access(parentDir(_path).c_str(), W_OK | X_OK) != 0)
		return setCode(403);//On a acces au repertoire parent pour faire des modifications
}

int	Response::hub() {

	buildPath();
	checkPermissions();
	// Si le code d'erreur est positif alors je remplis _body avec ma page .html et je m'arrete ici

	// Je regarde si la LocationConfig indique que ce path correspond a une cgi
	//Si oui alors :
	try {
		this->_cgi.copyFrom(*this);
		this->_cgi.buildEnv();
		this->_cgi.buildArgv();
		this->_cgi.openPipes();
	} catch (std::exception &er) {
		// what to do when exception ?
		return (-1);
	}
	return 0;
		//Je verifie que tout s'est bien passe, sinon je setcode, je remplis mon body avec la page html et je m'arrete ici

	// Si non alors:
		// je remplis le body de cette instance avec la page statique necessaire et je m'arrete ici 
}
