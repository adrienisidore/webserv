#include "webserv.hpp"

 int	main(int ac, char **av) {

 	try {
		check_args(ac, av);
		// syntaxic_parsing_config_file(av[1]);
		// temp_config_file = semantic_parsing(av[1]);

 		ServerMonitor	ServerMonitor("nginx_test2.conf");
		ServerMonitor.run();
		// add servers to Server Monitor and run them
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









// #include <sys/wait.h>
// #include <cstdlib>
// #include <cstdio>

// int main() {
//     pid_t pid;
//     int status;
//     int inpipe[2];   // pour envoyer au CGI
//     int outpipe[2];  // pour lire la sortie du CGI

//     if (pipe(inpipe) < 0 || pipe(outpipe) < 0) {
//         // perror("pipe");
// 		//thorw une Exception, ne pas exit SINON LEAKS
//         exit(EXIT_FAILURE);
//     }

//     // Environnement CGI
// 	//Voir tableau : https://www.alimnaqvi.com/blog/webserv car certains elements sont obligatoires.
// 	//Doit-on egalement ajouter l'environnement global ?
// 	//build_env() : donc CGI doit forcement etre dans Response (qui a chope la request dans son constructeur)
//     std::vector<std::string> env_strings;
//     env_strings.push_back("REQUEST_METHOD=POST");
//     env_strings.push_back("SCRIPT_NAME=./test.cgi");
//     env_strings.push_back("QUERY_STRING=fgdiughddfiuh");
//     env_strings.push_back("CONTENT_LENGTH=11");
//     env_strings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
//     env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
//     env_strings.push_back("SERVER_NAME=localhost");
//     env_strings.push_back("SERVER_PORT=8080");
//     std::vector<char*> envp;
//     for (size_t i = 0; i < env_strings.size(); ++i)
//         envp.push_back(const_cast<char*>(env_strings[i].c_str()));
//     envp.push_back(NULL);

//     // Arguments du programme CGI
//     std::vector<std::string> argv_strings;
//     argv_strings.push_back("./test.cgi");
//     std::vector<char*> argv;
//     for (size_t i = 0; i < argv_strings.size(); ++i)
//         argv.push_back(const_cast<char*>(argv_strings[i].c_str()));
//     argv.push_back(NULL);

//     pid = fork();
//     if (pid < 0) {
//         // perror("fork");
// 		//thorw une Exception, ne pas exit SINON LEAKS
//         exit(EXIT_FAILURE);
//     }

//     if (pid == 0) {
//         // Enfant :
// 		//le STDIN de l'enfant devient inpipe[0]
//         dup2(inpipe[0], STDIN_FILENO);
// 		//la sortie du terminal (STDOUT) de l'enfant devient outpipe[1]
//         dup2(outpipe[1], STDOUT_FILENO);
//         close(inpipe[1]);
//         close(outpipe[0]);

// 		//envp : le CGI recoit ces variables d'env pour tourner
//         execve(argv[0], &argv[0], &envp[0]);
// 		//thorw une Exception, ne pas exit SINON LEAKS
//         perror("execve");
//         exit(EXIT_FAILURE);
//     } else {
//         // Parent
//         close(inpipe[0]);
//         close(outpipe[1]);

//         // Simule un corps POST
//         const char *post_data = "name=adrien";
// 		// Parent : ecrit dans inpipe
//         write(inpipe[1], post_data, 11);
//         close(inpipe[1]);

//         // Lit la sortie du CGI (outpipe) : ce qu'on envoie au client
//         char buffer[1024];
//         ssize_t n;
//         while ((n = read(outpipe[0], buffer, sizeof(buffer) - 1)) > 0) {
//             buffer[n] = '\0';
//             std::cout << buffer;
//         }
//         close(outpipe[0]);

//         waitpid(pid, &status, 0);//Attention il faut que ce soit non-blocking, lire article : https://www.alimnaqvi.com/blog/webserv
// 		//Je crois qu'il faut faire surveiller inpipe et outpipe par poll()
//         std::cout << "\n[CGI terminé avec code " << WEXITSTATUS(status) << "]\n";
//     }

//     return 0;
// }


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






// int main() {
// 	//ATTENTION ORDRE IMPORTANT :
//     AutoConfig("nginx_test2.conf");

// 	// GlobalConfig global;
// 	// global.setDirective("root", "html");
// 	// global.setDirective("autoindex", "off");

// 	// // --- Serveur ---
// 	// ServerConfig server;
// 	// server.setDirective("listen", "127.0.0.1:8080");
// 	// server.setDirective("index", "index.html");

// 	// // Ajouter le serveur au global pour hériter des directives globales
// 	// global.addServer("webserv1/127.0.0.1:8080", server);

// 	// // --- Location ---
// 	// LocationConfig loc;
// 	// loc.setDirective("autoindex", "on");
// 	// loc.setDirective("cgi_handler", ".php /usr/bin/php-cgi");

// 	// // Ajouter la location au serveur après que le serveur ait hérité du global
// 	// global.accessServers().at("webserv1/127.0.0.1:8080").addLocation("/images", loc);

// 	// // --- Serveur 2 ---
// 	// ServerConfig server2;
// 	// server2.setDirective("listen", "127.0.0.1:8080");
// 	// server2.setDirective("index", "index2.html");

// 	// // Ajouter le serveur au global pour hériter des directives globales
// 	// global.addServer("webserv2/127.0.0.1:8080", server2);

// 	// Afficher la config complète
// 	return 0;

// }

