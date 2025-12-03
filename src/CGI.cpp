#include "webserv.hpp"

CGI::CGI() {}

CGI::~CGI() {}

void	CGI::copyFrom(HTTPcontent& other) {
		_code = other.getCode();
		_method = other.getMethod();
		_URI = other.getURI();
		_config = other.getConfig();
		_headers = other.getHeaders();
		_path = other.getPath();
		_current_body = other.getCurrentBody();
}

static std::string buildQueryString(const std::string &raw_uri) {
    size_t pos = raw_uri.find('?');
    if (pos == std::string::npos)
        return "QUERY_STRING=";
    return std::string("QUERY_STRING=") + raw_uri.substr(pos + 1);
}

static std::string buildContentLength(const std::string &body) {
    return std::string("CONTENT_LENGTH=") + std::string(body.empty() ? "0" : 
                     static_cast<std::ostringstream&>(std::ostringstream() << body.size()).str());
}

static std::string buildContentType(const std::map<std::string,std::string> &headers) {
    std::map<std::string,std::string>::const_iterator it = headers.find("Content-Type");
    if (it == headers.end())
        return "";
    return std::string("CONTENT_TYPE=") + it->second;
}

static std::string buildServerNameHost(const std::map<std::string,std::string> &headers, LocationConfig & config, int which)
{
    std::map<std::string,std::string>::const_iterator it = headers.find("HOST");

    std::string listen = config.getDirective("listen");
    std::string conf_host = listen;
    std::string conf_port = "8080";

    size_t pos = listen.find(':');
    if (pos != std::string::npos) {
        conf_host = listen.substr(0, pos);
        conf_port = listen.substr(pos + 1);
    }

    if (!which)
    {
        if (it == headers.end()) 
            return std::string("SERVER_NAME=") + conf_host;

        std::string host = it->second;
        size_t p = host.find(':');
        if (p != std::string::npos)
            host = host.substr(0, p);

        return std::string("SERVER_NAME=") + host;
    }
    else
    {
        if (it != headers.end()) {
            const std::string &host = it->second;
            size_t p = host.find(':');
            if (p != std::string::npos)
                return std::string("SERVER_PORT=") + host.substr(p + 1);
        }

        return std::string("SERVER_PORT=") + conf_port;
    }
}

std::string buildScriptFilename(const std::string &root, const std::string &uri)
{
    size_t q = uri.find('?');
    std::string path = (q == std::string::npos) ? uri : uri.substr(0, q);

    std::string full_path = root;
    
    if (!full_path.empty() && full_path[full_path.size() - 1] == '/' && !path.empty() && path[0] == '/')
        full_path.erase(full_path.size() - 1); 
    else if ((full_path.empty() || full_path[full_path.size() - 1] != '/') && (path.empty() || path[0] != '/'))
        full_path += '/';
    full_path += path;

    return std::string("SCRIPT_FILENAME=") + full_path;
}

static std::string buildScriptName(const std::string &uri)
{
    size_t pos = uri.find('?');
    std::string path = (pos == std::string::npos) ? uri : uri.substr(0, pos);
    return std::string("SCRIPT_NAME=") + path;
}

static std::string buildRemoteAddr(const struct sockaddr_storage &addr, socklen_t len)
{
    // Vérifier qu'on a bien au moins la taille d'un sockaddr_in
    if (len < sizeof(struct sockaddr_in))
        return "REMOTE_ADDR=0.0.0.0";

    const struct sockaddr_in *a = reinterpret_cast<const struct sockaddr_in*>(&addr);

    // Vérifier la famille (IPv4) : ne traite pas les IPv6 car pas de fonctions autorisees par le sujet
	// pour les traiter
    if (a->sin_family != AF_INET)
        return "REMOTE_ADDR=0.0.0.0";

    // Récupérer l'adresse au format binaire
    uint32_t ip = ntohl(a->sin_addr.s_addr);

    // Extraire chaque octet
    unsigned char o1 = (ip >> 24) & 0xFF;
    unsigned char o2 = (ip >> 16) & 0xFF;
    unsigned char o3 = (ip >> 8)  & 0xFF;
    unsigned char o4 = ip & 0xFF;

    std::ostringstream oss;
    oss << "REMOTE_ADDR="
        << (int)o1 << "."
        << (int)o2 << "."
        << (int)o3 << "."
        << (int)o4;

    return oss.str();
}


void CGI::buildEnv() {

	_env_strings.clear();
    _envp.clear();

	std::map<std::string, std::string>	cgi_headers;
	cgi_headers = _headers;
	std::map<std::string, std::string> result;
	for (std::map<std::string, std::string>::const_iterator it = cgi_headers.begin();
		it != cgi_headers.end(); ++it) {
		std::string newKey = "HTTP_";
		newKey.reserve(it->first.size() + 5);
		for (std::string::const_iterator c = it->first.begin(); c != it->first.end(); ++c)
			newKey += (*c == '-') ? '_' : *c;
		result[newKey] = it->second;
	}
	cgi_headers = result;

	for (std::map<std::string, std::string>::const_iterator it = cgi_headers.begin();
		it != cgi_headers.end(); ++it) {
		_env_strings.push_back(it->first + "=" + it->second);
	}

	_env_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_env_strings.push_back("REDIRECT_STATUS=200");//CGI appele uniquement pour du 200 (pas de CGI pour les erreurs)
	std::string request_method = "REQUEST_METHOD=" + _method;
	_env_strings.push_back(request_method.c_str());//attribut _methods de HTTPcontent
	std::string request_uri = "REQUEST_URI=" + _URI;
	_env_strings.push_back(request_uri.c_str());//attribut _URI de HTTPcontent
	_env_strings.push_back(buildScriptFilename(_config.getDirective("root"), _URI));//Chemin absolu complet vers l'executable cgi
	_env_strings.push_back(buildScriptName(_URI));//Chemin relatif vers le CGI
	_env_strings.push_back(buildRemoteAddr(_connection->_client_addr, _connection->_client_addr_len));//L'adresse IP du client
	_env_strings.push_back(buildQueryString(_URI));//la partie apres le "?"
	_env_strings.push_back(buildContentLength(_current_body));
	_env_strings.push_back(buildContentType(_headers));
	_env_strings.push_back(buildServerNameHost(_headers, _config, 0));
	_env_strings.push_back(buildServerNameHost(_headers, _config, 1));

    for (size_t i = 0; i < _env_strings.size(); ++i)
        _envp.push_back(const_cast<char*>(_env_strings[i].c_str()));
    _envp.push_back(NULL);
}

void CGI::buildArgv() {
	_argv_strings.clear();
	_argv.clear();

	std::string handlers = _config.getDirective("cgi_handler");
	std::string program;

	if (!findCgiProgramForPath(handlers, _path, program))
		throw std::runtime_error("No matching cgi_handler for requested path");

	_argv_strings.push_back(program);
	_argv_strings.push_back(_path);

	for (size_t i = 0; i < _argv_strings.size(); ++i)
		_argv.push_back(const_cast<char*>(_argv_strings[i].c_str()));
	_argv.push_back(NULL);
}


void	CGI::openPipes() {

    if (pipe(_inpipe) < 0 || pipe(_outpipe) < 0) {
        throw std::runtime_error("Can't open pipe !");
    }
    fcntl(_inpipe[0], F_SETFL, O_NONBLOCK);
    fcntl(_inpipe[1], F_SETFL, O_NONBLOCK);
    fcntl(_outpipe[0], F_SETFL, O_NONBLOCK);
    fcntl(_outpipe[1], F_SETFL, O_NONBLOCK);
}

void	CGI::execute_cgi() {

    _pid = fork();
    if (_pid < 0) {
		throw (ServerException("fork"));
    }

    if (_pid == 0) {
        // Enfant :
		//le STDIN de l'enfant devient _inpipe[0]
        dup2(_inpipe[0], STDIN_FILENO);
		//la sortie du terminal (STDOUT) de l'enfant devient _outpipe[1]
        dup2(_outpipe[1], STDOUT_FILENO);

		close(_inpipe[0]);
        close(_inpipe[1]);
        close(_outpipe[0]);
        close(_outpipe[1]);

		// reset to BLOCKING -> the child waits for data to arrive in stdin if call to read()
		fcntl(STDIN_FILENO, F_SETFL, 0); 
        fcntl(STDOUT_FILENO, F_SETFL, 0);
		//envp : le CGI recoit ces variables d'env pour tourner
        execve(_argv[0], &_argv[0], &_envp[0]);
		//modifier le code d'error (500 probleme serveur ??)
		throw (ServerException("execve"));
    } else {
        // Parent
        close(_inpipe[0]);
        close(_outpipe[1]);

		// non-blocking
		int flags;
		flags = fcntl(_inpipe[1], F_GETFL, 0);
		fcntl(_inpipe[1], F_SETFL, flags | O_NONBLOCK);
		flags = fcntl(_outpipe[0], F_GETFL, 0);
		fcntl(_outpipe[0], F_SETFL, flags | O_NONBLOCK);
	}
}
