#include "webserv.hpp"

// CGI doit être instancié dans la même portée que celle où tu gères la boucle d’événements,
// ou au moins conservé tant que le descripteur associé (pipe, pid) est surveillé.
// pas forcément instancier dans la même fonction que poll(),
// mais il faut que la durée de vie du CGI couvre toute la période où poll() surveille ses pipes.

// Sinon l’objet serait détruit avant la fin de l’exécution du CGI,
// et ses champs (_inpipe, _outpipe, _pid, _envp, etc.) deviendraient invalides.

// NB : _outpipe[0] au pollfd
// Quand le poll() détecte l’événement (lecture terminée ou EOF),
// alors seulement tu détruis l’objet CGI

CGI::CGI() {}

CGI::~CGI() {}

// //Si changement, penser a changer la version de Response + les prototypes + CGI
void	CGI::copyFrom(HTTPcontent& other) {
		_code = other.getCode();
		_method = other.getMethod();
		_URI = other.getURI();
		_config = other.getConfig();
		_headers = other.getHeaders();

}

static std::string buildQueryString(const std::string &raw_uri) {
    size_t pos = raw_uri.find('?');
    if (pos == std::string::npos)
        return "QUERY_STRING=";
    return std::string("QUERY_STRING=") + raw_uri.substr(pos + 1);
}

static std::string buildContentLength(const std::string &body) {
    return std::string("CONTENT_LENGTH=") + std::string(body.empty() ? "0" : 
                     static_cast<std::ostringstream&>(std::ostringstream() << body.size()).str());
}

static std::string buildContentType(const std::map<std::string,std::string> &headers) {
    std::map<std::string,std::string>::const_iterator it = headers.find("Content-Type");
    if (it == headers.end())
        return ""; // ne pas ajouter
    return std::string("CONTENT_TYPE=") + it->second;
}

static std::string buildServerNameHost(const std::map<std::string,std::string> &headers, LocationConfig & config, int which)
{
    std::map<std::string,std::string>::const_iterator it = headers.find("HOST");

    // --- Correction point 2 : séparer correctement host et port depuis la config ---
    // config.listen contient typiquement "127.0.0.1:8080"
    std::string listen = config.getDirective("listen");
    std::string conf_host = listen;
    std::string conf_port = "8080"; // fallback raisonnable

    size_t pos = listen.find(':');
    if (pos != std::string::npos) {
        conf_host = listen.substr(0, pos);
        conf_port = listen.substr(pos + 1);
    }

    if (!which)
    {
        if (it == headers.end()) 
            return std::string("SERVER_NAME=") + conf_host;

        std::string host = it->second;
        size_t p = host.find(':');
        if (p != std::string::npos)
            host = host.substr(0, p);

        return std::string("SERVER_NAME=") + host;
    }
    else
    {
        if (it != headers.end()) {
            const std::string &host = it->second;
            size_t p = host.find(':');
            if (p != std::string::npos)
                return std::string("SERVER_PORT=") + host.substr(p + 1);
        }

        // --- Correction point 3 : fallback dynamique depuis listen ---
        return std::string("SERVER_PORT=") + conf_port;
    }
}

// root           : chemin absolu du dossier racine
// location_uri   : chemin défini dans la config (ex: "cgi-bin")
// uri            : URI complète fournie par le client (ex: "/cgi-bin/run.cgi?x=1")

std::string buildScriptFilename(const std::string &root, const std::string &location_uri, const std::string &uri)
{
    // 1. enlever la query string
    size_t q = uri.find('?');
    std::string path = (q == std::string::npos) ? uri : uri.substr(0, q);

    // 2. extraire le script_name (après le dernier '/')
    size_t slash = path.rfind('/');
    std::string script_name = (slash == std::string::npos)
                                ? path
                                : path.substr(slash + 1);

    // 3. construire le chemin absolu : root + location_uri + script_name
    std::string full = root;

    if (!full.empty() && full[full.size() - 1] != '/')
        full += '/';

    if (!location_uri.empty()) {
        full += location_uri;
        if (full[full.size() - 1] != '/')
            full += '/';
    }

    full += script_name;

    return std::string("SCRIPT_FILENAME=") + full;
}


static std::string buildScriptName(const std::string &uri)
{
    size_t pos = uri.find('?');
    std::string path = (pos == std::string::npos) ? uri : uri.substr(0, pos);
    return std::string("SCRIPT_NAME=") + path;
}

static std::string buildRemoteAddr(const struct sockaddr_storage &addr, socklen_t len)
{
	// On suppose IPv4 (sockaddr_in)
	const struct sockaddr_in *a = reinterpret_cast<const struct sockaddr_in*>(&addr);

	// Récupérer l'adresse au format binaire
	uint32_t ip = ntohl(a->sin_addr.s_addr);

	// Extraire chaque octet
	unsigned char o1 = (ip >> 24) & 0xFF;
	unsigned char o2 = (ip >> 16) & 0xFF;
	unsigned char o3 = (ip >> 8)  & 0xFF;
	unsigned char o4 = ip & 0xFF;

	// Construire la string IPv4
	std::ostringstream oss;
	oss << "REMOTE_ADDR="
		<< (int)o1 << "."
		<< (int)o2 << "."
		<< (int)o3 << "."
		<< (int)o4;

	return oss.str();
}


void CGI::buildEnv() {

	_env_strings.clear();
    _envp.clear();
	//Voir tableau : https://www.alimnaqvi.com/blog/webserv car certains elements sont obligatoires.

	//Doit-on egalement ajouter l'environnement global ? Non je crois pas

	std::map<std::string, std::string>	cgi_headers;
	cgi_headers = _headers;
	std::map<std::string, std::string> result;
	for (std::map<std::string, std::string>::const_iterator it = cgi_headers.begin();
		it != cgi_headers.end(); ++it) {
		std::string newKey = "HTTP_";
		newKey.reserve(it->first.size() + 5);
		for (std::string::const_iterator c = it->first.begin(); c != it->first.end(); ++c)
			newKey += (*c == '-') ? '_' : *c;
		result[newKey] = it->second;
	}
	cgi_headers = result;

	for (std::map<std::string, std::string>::const_iterator it = cgi_headers.begin();
		it != cgi_headers.end(); ++it) {
		_env_strings.push_back(it->first + "=" + it->second);
	}

	_env_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
	std::string request_method = "REQUEST_METHOD=" + _method;
	_env_strings.push_back(request_method.c_str());//attribut _methods de HTTPcontent
	std::string request_uri = "REQUEST_URI=" + _URI;
	_env_strings.push_back(request_uri.c_str());//attribut _URI de HTTPcontent

	_env_strings.push_back(buildScriptFilename(_config.getDirective("root"), _config.getDirective("location_uri"), _URI));//Chemin absolu complet vers l'executable cgi
	_env_strings.push_back(buildScriptName(_URI));//Chemin relatif vers le CGI

	//A CODER
	_env_strings.push_back("REMOTE_ADDR=127.0.0.1");//L'adresse IP du client
	// _env_strings.push_back(buildRemoteAddr(_connection->_client_addr, _connection->_client_addr_len));

	///////////////////////////////////////////////

	_env_strings.push_back(buildQueryString(_URI));//la partie apres le "?"
	_env_strings.push_back(buildContentLength(_current_body));
	_env_strings.push_back(buildContentType(_headers));
	_env_strings.push_back(buildServerNameHost(_headers, _config, 0));
	_env_strings.push_back(buildServerNameHost(_headers, _config, 1));

    for (size_t i = 0; i < _env_strings.size(); ++i)
        _envp.push_back(const_cast<char*>(_env_strings[i].c_str()));
    _envp.push_back(NULL);

	//Affichage du resultat :
	for (std::vector<std::string>::const_iterator it = _env_strings.begin(); it != _env_strings.end(); ++it)
		std::cout << *it << std::endl;
}



void CGI::buildArgv() {

	_argv_strings.clear();
	_argv.clear();

	_argv_strings.push_back(buildScriptFilename(_config.getDirective("root"), _config.getDirective("location_uri"), _URI)); // Chemin vers exécutable

	for (size_t i = 0; i < _argv_strings.size(); ++i)
		_argv.push_back(const_cast<char*>(_argv_strings[i].c_str()));
	_argv.push_back(NULL);
}

void	CGI::openPipes() {

	//Dans constructeur
    if (pipe(_inpipe) < 0 || pipe(_outpipe) < 0) {
        // perror("pipe");
		//thorw une Exception, ne pas exit SINON LEAKS
        exit(EXIT_FAILURE);
    }
    fcntl(_inpipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(_inpipe[1], F_SETFD, FD_CLOEXEC);
    fcntl(_outpipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(_outpipe[1], F_SETFD, FD_CLOEXEC);
}

/*
Il faut ecrire dans stdin apres fork()
Dans Parent:
	- ferme extremites inutiles
	- met descripteurs en non-blocking
	- inscrit descripteur dans poll/epoll (maintenant ou plus tard ?)
	- ecrit le body dans stdin
	- ferme l'ecriture pour signaler EOF
*/
void	CGI::launchExecve() {

    _pid = fork();
    if (_pid < 0) {
		//Ou modifier setCode puis return ;
		throw (ServerException("fork"));
    }

    if (_pid == 0) {
        // Enfant :
		//le STDIN de l'enfant devient _inpipe[0]
        dup2(_inpipe[0], STDIN_FILENO);
		//la sortie du terminal (STDOUT) de l'enfant devient _outpipe[1]
        dup2(_outpipe[1], STDOUT_FILENO);

		close(_inpipe[0]);
        close(_inpipe[1]);
        close(_outpipe[0]);
        close(_outpipe[1]);

		//envp : le CGI recoit ces variables d'env pour tourner
        execve(_argv[0], &_argv[0], &_envp[0]);
		//modifier le code d'error (500 probleme serveur ??)
        perror("execve");
		throw (ServerException("execve"));
    } else {


        // Parent
        close(_inpipe[0]);
        close(_outpipe[1]);

		// non-blocking
		int flags;
		flags = fcntl(_inpipe[1], F_GETFL, 0);
		fcntl(_inpipe[1], F_SETFL, flags | O_NONBLOCK);
		flags = fcntl(_outpipe[0], F_GETFL, 0);
		fcntl(_outpipe[0], F_SETFL, flags | O_NONBLOCK);


		/*
        // Simule un corps POST
        const char *post_data = "name=adrien";//redondant avec _body mais necessaire car STDIN_FILENO est en lecture seule,
		//si je veux ecrire _body dans dans STDIN depuis l'enfant c'est galere
		// Parent : ecrit _body dans _inpipe
        write(_inpipe[1], post_data, 11);
        close(_inpipe[1]);

        // Lit la sortie du CGI (_outpipe) : ce qu'on envoie au client ==> le body

		//////////////////////////////////////////////////////////////////////////////
		// Il faut faire un appel a read a chaque fois qu'on passe sur poll()
        char buffer[1024];
        ssize_t n;
        while ((n = read(_outpipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            std::cout << buffer;
        }
        close(_outpipe[0]);
		waitpid(_pid, &_status, 0);//Attention il faut que ce soit non-blocking, lire article : https://www.alimnaqvi.com/blog/webserv
		//////////////////////////////////////////////////////////////////////////////
   
		//Useless
        std::cout << "\n[CGI terminé avec code " << WEXITSTATUS(_status) << "]\n";
		*/
    }
}
