#ifndef CGI_HPP
# define CGI_HPP

# include "webserv.hpp"

// A AJOUTER DANS webserv.hpp
# include <sys/wait.h>
# include <cstdlib>
# include <cstdio>

// class Response : public HTTPcontent {
class CGI {

	public:
		CGI();
		~CGI();
		void	launchExecve();

		pid_t	pid;
		int		status;

		//On surveille uniquement inpipe
		int		inpipe[2];   // pour envoyer le _nody a execve depuis stdin (necessaire pour faire les choses proprement)
		int		outpipe[2];  // pour lire la sortie du CGI

	private:

		std::vector<std::string> _env_strings;
		std::vector<char*> _envp;
		void buildEnv();

		// void			copyFrom(HTTPcontent& other);

		
		std::vector<std::string> _argv_strings;
		std::vector<char*> _argv;
		void buildArgv();
};

#endif