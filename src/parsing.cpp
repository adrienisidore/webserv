#include "webserv.hpp"

void	check_args(int ac, char **av) {

	std::string	config_file_name;

	if (ac != 2)
		throw (std::invalid_argument("wrong number of arguments"));

	config_file_name = av[1];

	size_t index = config_file_name.find(".conf");
	if (index == std::string::npos) // dangerous
		throw (std::invalid_argument("invalid config file format"));

	if (access(av[1], R_OK) != 0)
		throw std::invalid_argument("impossible to read config file");

    int fd = open(av[1], O_RDONLY);
    if (fd == -1)
		throw std::invalid_argument("impossible to read config file");
}

//ATTENTION : Ajouter en dur dans le fichier toutes les directives par defaut (par ex. max_body_size ...) AU NIVEAU DES DIRECTIVES GLOBALCONFIG


// void	syntaxic_parsing_config_file(char *filename) {

//     std::ifstream file(filename);
//     std::string line;
    
//     if (!file.is_open()) {
//         std::cerr << "Error opening file" << std::endl;
//         return 1;
//     }
    
//     while (std::getline(file, line)) {
//         std::cout << line << std::endl;
//     }
    
//     file.close();
//     return 0;
// }

// bool	syntaxic_parsing_context(int fd, int level) {

// 	// VERIFIER DIRECTIVES

// }

// idee: parsing recursif
// -> keep track of open_brackets
	// verifier que les brackets sont fermes
	// verifier que l'ordre et l'emplacement des server et location est respecte
	// verifier que les directives sont bien situes avant les contextes 
	// lignes vides ignorees -> EOF a la fin du fichier

/*

---------------- CONFIG FILE --------------------

http {

	root /blabla
	error_page errrrr.html

	server {

		listen 8002;
		host 127.0.01;
		server_name lala;
		root docs/fusion_web;
		client_max_body_size 800000;

		location / {
			allow_methods DELETE POST;
			index index.html;
		}

		location /doc/ {
			index index.html;
		}
	}

	server {

		listen localhost:9743;
		server_name localhost;
		root ./assets/default_website;
		cgi_handler .py /usr/bin/python3;

		location /cgi-demo {
			index hello.py;
		}

		location /red {
			return /tours;
		}

		location /cgi-bin {
			root ./;
			allow_methods GET POST DELETE;
			index time.py;
			cgi_path /usr/bin/python3 /bin/bash;
			cgi_ext .py .sh;
		}
	}
}

------------------------------------------------
*/
