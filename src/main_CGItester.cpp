#include "webserv.hpp"

// static void buildEnv(std::vector<std::string>& env_strings, std::vector<char*>& envp) {


// 	//Voir tableau : https://www.alimnaqvi.com/blog/webserv car certains elements sont obligatoires.
// 	//Doit-on egalement ajouter l'environnement global ?
// 	// Ajouter tous les _headers + "HTTP_" (en modifiant - pour _)

//     env_strings.push_back("REQUEST_METHOD=POST");
//     env_strings.push_back("SCRIPT_NAME=./test.cgi");
//     env_strings.push_back("QUERY_STRING=TEST1TEST2TEST3");
//     env_strings.push_back("CONTENT_LENGTH=11");
//     env_strings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
//     env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
//     env_strings.push_back("SERVER_NAME=localhost");
//     env_strings.push_back("SERVER_PORT=8080");

//     for (size_t i = 0; i < env_strings.size(); ++i)
//         envp.push_back(const_cast<char*>(env_strings[i].c_str()));
//     envp.push_back(NULL);
// }

// static void buildArgv(std::vector<std::string>& argv_strings, std::vector<char*>& argv) {

//     argv_strings.push_back("./test2.cgi"); // Chemin vers exécutable
// 	// argv_strings.push_back("./test.cgi");//Chemin vers l'executable

// 	//Si j'ai d'autres arguments a donner au programme c'est ICI
// 	// *** ICI ***
//     argv_strings.push_back("param1");
//     argv_strings.push_back("param2");

//     for (size_t i = 0; i < argv_strings.size(); ++i)
//         argv.push_back(const_cast<char*>(argv_strings[i].c_str()));
//     argv.push_back(NULL);
// }

// static void	launchExecve() {
// 	//Les pointeurs (argv[i], envp[i]) doivent rester valides jusqu’à l’appel à execve()
//     std::vector<std::string> env_strings;
//     std::vector<char*> envp;
//     buildEnv(env_strings, envp);//LocationConfig en argument + _headers ?

//     std::vector<std::string> argv_strings;
//     std::vector<char*> argv;
//     buildArgv(argv_strings, argv);//_path en argument ?

//     pid_t pid;
//     int status;
//     int inpipe[2];   // pour envoyer au CGI
//     int outpipe[2];  // pour lire la sortie du CGI

// 	//Dans constructeur
//     if (pipe(inpipe) < 0 || pipe(outpipe) < 0) {
//         // perror("pipe");
// 		//thorw une Exception, ne pas exit SINON LEAKS
//         exit(EXIT_FAILURE);
//     }

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
//         const char *post_data = "name=adrien";//un CGI recupere le body depuis stdin
// 		// Parent : ecrit dans inpipe
//         write(inpipe[1], post_data, 11);
//         close(inpipe[1]);

//         // Lit la sortie du CGI (outpipe) : ce qu'on envoie au client ==> le body
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
// }

// int main() {

// 	// launchExecve();

// 	CGI	cgi;

// 	cgi.launchExecve();

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


