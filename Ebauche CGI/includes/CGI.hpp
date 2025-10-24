#ifndef CGI_HPP
# define CGI_HPP

# include "webserv.hpp"

// A AJOUTER DANS webserv.hpp
# include <sys/wait.h>
# include <cstdlib>
# include <cstdio>

//Dans Response, CGI prend en argument this [Response], et accede a tous les elements
// class Response : public HTTPcontent {
class CGI {

	public:
		CGI();//CGI(Response *parent_resp);
		~CGI();
		void	launchExecve();
		int		getOutPipe() const;

	private:
		// Response * parent_resp;
		pid_t	_pid;
		int		_status;
		//A priori 1 seul pipe necessaire, car l'instance CGI possede deja les elements
		int		_inpipe[2];   // pour envoyer au CGI, plutot que de creer inpipe on peut simplement write le _body de CGI dans stdin
		int		_outpipe[2];  // pour lire la sortie du CGI

		std::vector<std::string> _env_strings;
		std::vector<char*> _envp;
		void buildEnv();

		
		std::vector<std::string> _argv_strings;
		std::vector<char*> _argv;
		void buildArgv();
};

#endif