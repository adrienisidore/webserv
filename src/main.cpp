#include "webserv.hpp"

 int	main(int ac, char **av) {

 	try {
		check_args(ac, av);
		std::string temp_file;
		if (ac == 1)
			temp_file = parseConfig("default.conf");
		else
			temp_file = parseConfig(av[1]);
 		ServerMonitor	ServerMonitor(temp_file);
		ServerMonitor.run();
 	}
 	catch (SocketException &er) {
 		std::cerr << "Error Socket: " << er.what() << std::endl;
 	}
 	catch (ConnectionException &er) {
 		std::cerr << "Error Connection: " << er.what() << std::endl;
 	}
 	catch (HttpException &er) {
 		std::cerr << "Error Http: " << er.what() << std::endl;
 	}
	catch (ParsingException &er) {
		std::cerr << "Error Parsing:" << er.what() << std::endl;
	}
 	catch (std::exception &er) {
 		std::cerr << "Error: " << er.what() << std::endl;
 	}

 	return (0);
 }

/*
TIMEOUT : que se passe t il si CGI dure plus longtemps que le TIMETOUT ?
valgrind --trace-children=yes --track-fds=yes --leak-check=full --show-leak-kinds=definite ./webserv

Creer une classe CGI avec inpipe, outpipe
*/




// Principe

// Les serveurs HTTP peuvent mapper certaines URL à un CGI.

// Exemple : /upload/ → upload.cgi

// Quand le client fait un POST /upload/ HTTP/1.1 :

// Le serveur reçoit la requête.

// Il détecte que l’URL correspond à un CGI (configuration du serveur).

// Il crée un processus CGI avec :

// stdin → corps de la requête (fichier envoyé, formulaire).

// envp → variables comme CONTENT_LENGTH, CONTENT_TYPE, REQUEST_METHOD.

// Le CGI lit stdin et génère la réponse HTTP.

// Le serveur transmet la sortie au client.

// Pertinence

// /upload/ n’est pas spécial par lui-même.

// Le serveur doit avoir un script/CGI configuré pour cette route.

// On ne peut pas poster n’importe où et s’attendre à ce qu’un CGI se déclenche automatiquement.


