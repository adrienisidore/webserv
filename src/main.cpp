#include "webserv.hpp"

// int	main(int ac, char **av) {

// 	(void)ac;
// 	(void)av;
// 	try {
// 		Server	server("8080");
// 		server.run();
// 	}
// 	catch (SocketException &er) {
// 		std::cerr << "Error Socket: " << er.what() << std::endl;
// 	}
// 	catch (ConnectionException &er) {
// 		std::cerr << "Error Connection: " << er.what() << std::endl;
// 	}
// 	catch (HttpException &er) {
// 		std::cerr << "Error Http: " << er.what() << std::endl;
// 	}
// 	catch (std::exception &er) {
// 		std::cerr << "Error: " << er.what() << std::endl;
// 	}
// 	return (0);
// }

//TIMEOUT : que se passe t il si CGI dure plus longtemps que le TIMETOUT ?
//valgrind --trace-children=yes --track-fds=yes --leak-check=full --show-leak-kinds=definite ./webserv

//Creer une classe CGI avec inpipe, outpipe

// Rôle du CGI

// Génère la totalité de la réponse HTTP :

// Headers (ex. Content-Type, Status, Content-Length si nécessaire).

// Ligne vide (\n\n) séparant headers et corps.

// Body (contenu dynamique).


// Rôle du serveur

// Ne connaît pas le contenu du body ou des headers.

// Se contente de :

// Préparer les variables d’environnement (env_strings).

// Fournir le stdin au CGI si POST.

// Lire le stdout du CGI.

// Transmettre tel quel la sortie au client HTTP.
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstdio>

int main() {
    pid_t pid;
    int status;
    int inpipe[2];   // pour envoyer au CGI
    int outpipe[2];  // pour lire la sortie du CGI

    if (pipe(inpipe) < 0 || pipe(outpipe) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Environnement CGI
    std::vector<std::string> env_strings;
    env_strings.push_back("REQUEST_METHOD=POST");
    env_strings.push_back("SCRIPT_NAME=./test.cgi");
    env_strings.push_back("QUERY_STRING=");
    env_strings.push_back("CONTENT_LENGTH=11");
    env_strings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
    env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env_strings.push_back("SERVER_NAME=localhost");
    env_strings.push_back("SERVER_PORT=8080");
    std::vector<char*> envp;
    for (size_t i = 0; i < env_strings.size(); ++i)
        envp.push_back(const_cast<char*>(env_strings[i].c_str()));
    envp.push_back(NULL);

    // Arguments du programme CGI
    std::vector<std::string> argv_strings;
    argv_strings.push_back("./test.cgi");
    std::vector<char*> argv;
    for (size_t i = 0; i < argv_strings.size(); ++i)
        argv.push_back(const_cast<char*>(argv_strings[i].c_str()));
    argv.push_back(NULL);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Enfant :
		//le STDIN de l'enfant devient inpipe[0]
        dup2(inpipe[0], STDIN_FILENO);
		//la sortie du terminal (STDOUT) de l'enfant devient outpipe[1]
        dup2(outpipe[1], STDOUT_FILENO);
        close(inpipe[1]);
        close(outpipe[0]);

		//envp : le CGI recoit ces variables d'env pour tourner
        execve(argv[0], &argv[0], &envp[0]);
        perror("execve");
        exit(EXIT_FAILURE);
    } else {
        // Parent
        close(inpipe[0]);
        close(outpipe[1]);

        // Simule un corps POST
        const char *post_data = "name=adrien";
		// Parent : ecrit dans inpipe
        write(inpipe[1], post_data, 11);
        close(inpipe[1]);

        // Lit la sortie du CGI (outpipe) : ce qu'on envoie au client
        char buffer[1024];
        ssize_t n;
        while ((n = read(outpipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            std::cout << buffer;
        }
        close(outpipe[0]);

        waitpid(pid, &status, 0);
        std::cout << "\n[CGI terminé avec code " << WEXITSTATUS(status) << "]\n";
    }

    return 0;
}




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

