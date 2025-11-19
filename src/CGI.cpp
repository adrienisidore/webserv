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
		_path = other.getPath();

}

// A FINIR
void CGI::buildEnv() {

	//Securite (surement useless)
	_env_strings.clear();
    _envp.clear();
	//Voir tableau : https://www.alimnaqvi.com/blog/webserv car certains elements sont obligatoires.

	//Doit-on egalement ajouter l'environnement global ? Non je crois pas

	// Ajouter tous les _headers + "HTTP_" (en modifiant - pour _) A FAIRE
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

	// for (std::map<std::string, std::string>::const_iterator it = cgi_headers.begin(); it != cgi_headers.end(); ++it)
	// 	std::cout << it->first << " => " << it->second << std::endl;

	for (std::map<std::string, std::string>::const_iterator it = cgi_headers.begin();
		it != cgi_headers.end(); ++it) {
		_env_strings.push_back(it->first + "=" + it->second);
	}

	//On inclut egalement un certain nombre de var d'env :

	_env_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
	std::string request_method = "REQUEST_METHOD=" + _method;
	_env_strings.push_back(request_method.c_str());//attribut _methods de HTTPcontent
	std::string request_uri = "REQUEST_URI=" + _URI;
	_env_strings.push_back(request_uri.c_str());//attribut _URI de HTTPcontent

	_env_strings.push_back("SCRIPT_FILENAME/home/.../test.cgi");//Chemin absolu complet vers l'executable cgi
	_env_strings.push_back("SCRIPT_NAME=./test.cgi");//Chemin relatif vers le CGI

	// _env_strings.push_back("QUERY_STRING=TEST1TEST2TEST3");//la partie apres le "?" de l'URI

	// N'est pas forcement fourni si la request est chunked
	_env_strings.push_back("CONTENT_LENGTH=11");//taille du body de la REQUEST (total sieze of unchunked_body si le body etait chunked)

	// A Aller chercher dans le fichier config
	_env_strings.push_back("SERVER_NAME=localhost");//le hostname du serveur (LocationConfig) ou le
	_env_strings.push_back("SERVER_PORT=8080");


	_env_strings.push_back("REMOTE_ADDR=127.0.0.1");//L'adresse IP du client

    for (size_t i = 0; i < _env_strings.size(); ++i)
        _envp.push_back(const_cast<char*>(_env_strings[i].c_str()));
    _envp.push_back(NULL);
}

void CGI::buildArgv() {

	//Securite (surement useless)
	_argv_strings.clear();
    _argv.clear();

	std::string	handler_directive = _config.getDirective("cgi_handler");
	_argv_strings.push_back(handler_directive.substr(handler_directive.find(' ') + 1));
    _argv_strings.push_back(_path); // Chemin vers exécutable

	//Si j'ai d'autres arguments a donner au programme c'est ICI
	// *** ICI ***
    // _argv_strings.push_back("param1");
    // _argv_strings.push_back("param2");

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
    fcntl(_inpipe[0], F_SETFL, O_NONBLOCK);
    fcntl(_inpipe[1], F_SETFL, O_NONBLOCK);
    fcntl(_outpipe[0], F_SETFL, O_NONBLOCK);
    fcntl(_outpipe[1], F_SETFL, O_NONBLOCK);
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
void	CGI::execute_cgi() {

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


		if (getMethod() == "POST" && !getCode()) {
			ssize_t written = write(_inpipe[1], _current_body.c_str(), _current_body.size());
        	std::cerr << "Wrote " << written << " bytes to CGI stdin\n";
		}

        close(_inpipe[1]);
		    int status;
		pid_t result = waitpid(_pid, &status, WNOHANG);
		if (result != 0) {
			std::cerr << "CGI died immediately! status=" << status << "\n";
			throw ServerException("CGI failed to start");
		}
		
		std::cerr << "CGI started successfully, PID=" << _pid << " outpipe[0]=" << _outpipe[0] << "\n";
        //close(_outpipe[0]);

		//waitpid(_pid, &_status, 0);//Attention il faut que ce soit non-blocking, lire article : https://www.alimnaqvi.com/blog/webserv
		// -> NOT HERE
		///
		// if (WIFEXITED(_status) && WEXITSTATUS(_status) == 0)
		// 	std::cout << "[CGI success]\n";
		// else {
		// 	std::cout << "[CGI failed with code " << WEXITSTATUS(_status) << "]\n";
		// 	setCode(500);
		// }
	}
}
