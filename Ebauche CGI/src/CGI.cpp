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

void CGI::buildEnv() {

	//Securite (surement useless)
	_env_strings.clear();
    _envp.clear();
	//Voir tableau : https://www.alimnaqvi.com/blog/webserv car certains elements sont obligatoires.
	//Doit-on egalement ajouter l'environnement global ?
	// Ajouter tous les _headers + "HTTP_" (en modifiant - pour _)

    _env_strings.push_back("REQUEST_METHOD=POST");
    _env_strings.push_back("SCRIPT_NAME=./test.cgi");
    _env_strings.push_back("QUERY_STRING=TEST1TEST2TEST3");
    _env_strings.push_back("CONTENT_LENGTH=11");
    _env_strings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
    _env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
    _env_strings.push_back("SERVER_NAME=localhost");
    _env_strings.push_back("SERVER_PORT=8080");

    for (size_t i = 0; i < _env_strings.size(); ++i)
        _envp.push_back(const_cast<char*>(_env_strings[i].c_str()));
    _envp.push_back(NULL);
}

void CGI::buildArgv() {

	//Securite (surement useless)
	_argv_strings.clear();
    _argv.clear();

    _argv_strings.push_back("./test2.cgi"); // Chemin vers exécutable
	// _argv_strings.push_back("./test.cgi");//Chemin vers l'executable

	//Si j'ai d'autres arguments a donner au programme c'est ICI
	// *** ICI ***
    _argv_strings.push_back("param1");
    _argv_strings.push_back("param2");

    for (size_t i = 0; i < _argv_strings.size(); ++i)
        _argv.push_back(const_cast<char*>(_argv_strings[i].c_str()));
    _argv.push_back(NULL);
}

void	CGI::launchExecve() {

	//A chaque fois qu'on lance execve on recupere les nouveaux parametres de la reponse
	// this->copyFrom(*parent_resp);
	buildEnv();//LocationConfig en argument + _headers ?
    buildArgv();//_path en argument ?

	//Dans constructeur
    if (pipe(_inpipe) < 0 || pipe(_outpipe) < 0) {
        // perror("pipe");
		//thorw une Exception, ne pas exit SINON LEAKS
        exit(EXIT_FAILURE);
    }

    _pid = fork();
    if (_pid < 0) {
        // perror("fork");
		//thorw une Exception, ne pas exit SINON LEAKS
        exit(EXIT_FAILURE);
    }

    if (_pid == 0) {
        // Enfant :
		//le STDIN de l'enfant devient _inpipe[0]
        dup2(_inpipe[0], STDIN_FILENO);
		//la sortie du terminal (STDOUT) de l'enfant devient _outpipe[1]
        dup2(_outpipe[1], STDOUT_FILENO);
        close(_inpipe[1]);
        close(_outpipe[0]);

		//envp : le CGI recoit ces variables d'env pour tourner
        execve(_argv[0], &_argv[0], &_envp[0]);
		//thorw une Exception, ne pas exit SINON LEAKS
        perror("execve");
        exit(EXIT_FAILURE);
    } else {
        // Parent
        close(_inpipe[0]);
        close(_outpipe[1]);

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
    }
}