#ifndef CGI_HPP
# define CGI_HPP

# include "webserv.hpp"

// class Response : public HTTPcontent {
class CGI : public HTTPcontent {

	public:
		CGI();
		virtual ~CGI();
		void	launchExecve();

		pid_t	_pid;
		int		_status;

		//On surveille uniquement inpipe
		int		_inpipe[2];   // pour envoyer le _nody a execve depuis stdin (necessaire pour faire les choses proprement)
		int		_outpipe[2];  // pour lire la sortie du CGI
		void openPipes();

		std::vector<std::string> _env_strings;
		std::vector<char*> _envp;
		void buildEnv();

		void			copyFrom(HTTPcontent& other);

		std::vector<std::string> _argv_strings;
		std::vector<char*> _argv;
		void buildArgv();

};

#endif