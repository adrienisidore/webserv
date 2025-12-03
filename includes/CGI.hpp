#ifndef CGI_HPP
# define CGI_HPP

# include "webserv.hpp"

class CGI : public HTTPcontent {

	public:
		CGI();
		virtual ~CGI();
		void	execute_cgi();

		pid_t	_pid;
		int		_status;

		//On surveille uniquement inpipe
		int		_inpipe[2];
		int		_outpipe[2];
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
