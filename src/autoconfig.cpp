#include "autoconfig.hpp"

// --- Helper : indentation lisible ---
// static void printIndent(int depth) {
// 	for (int i = 0; i < depth; ++i)
// 		std::cout << "    ";
// }
//
// // --- LOCATION CONFIG ---
// static void printLocationConfig(const std::string &path, const LocationConfig &loc, int depth = 2) {
// 	printIndent(depth);
// 	std::cout << "location " << path << " {" << std::endl;
//
// 	const std::map<std::string, std::string> &directives = loc.getDirectives();
// 	for (std::map<std::string, std::string>::const_iterator it = directives.begin();
// 		 it != directives.end(); ++it) {
// 		printIndent(depth + 1);
// 		std::cout << it->first << " " << it->second << ";" << std::endl;
// 	}
//
// 	printIndent(depth);
// 	std::cout << "}" << std::endl;
// }
//
// // --- SERVER CONFIG ---
// static void printServerConfig(const std::string &key, const ServerConfig &server, int depth = 1) {
// 	printIndent(depth);
// 	std::cout << "server (" << key << ") {" << std::endl;
//
// 	const std::map<std::string, std::string> &directives = server.getDirectives();
// 	for (std::map<std::string, std::string>::const_iterator it = directives.begin();
// 		 it != directives.end(); ++it) {
// 		printIndent(depth + 1);
// 		std::cout << it->first << " " << it->second << ";" << std::endl;
// 	}
//
// 	// Locations
// 	const std::map<std::string, LocationConfig> &locs = server.getLocations();
// 	for (std::map<std::string, LocationConfig>::const_iterator it = locs.begin();
// 		 it != locs.end(); ++it)
// 		printLocationConfig(it->first, it->second, depth + 1);
//
// 	printIndent(depth);
// 	std::cout << "}" << std::endl;
// }
//
// // --- GLOBAL CONFIG ---
// static void printGlobalConfig(GlobalConfig &global) {
//
// 	const std::map<std::string, std::string> &directives = global.getDirectives();
// 	for (std::map<std::string, std::string>::const_iterator it = directives.begin();
// 		 it != directives.end(); ++it) {
// 		printIndent(1);
// 		std::cout << it->first << " " << it->second << ";" << std::endl;
// 	}
//
// 	std::map<std::string, ServerConfig> &servers = global.accessServers();
// 	for (std::map<std::string, ServerConfig>::const_iterator it = servers.begin();
// 		 it != servers.end(); ++it)
// 		printServerConfig(it->first, it->second, 1);
// }

static std::string resolvePath(const std::string &root, const std::string &target)
{
	std::string full = root;

	// Ajouter un slash si root n'en a pas
	if (!full.empty() && full[full.size() - 1] == '/')
		full.erase(full.size() - 1);

	full += target; // target commence par "/"

	return full;
}

static void parseRedir(const LocationConfig tmp_loc, const ServerConfig &corresponding_server) {

	std::string ret = tmp_loc.getDirective("return"); 
	if (ret.empty())
		return;

	std::istringstream iss(ret);
	int code;
	std::string target;

	// Syntaxe de base
	if (!(iss >> code >> target))
		throw ParsingException("Invalid return syntax");

	std::string extra;
	if (iss >> extra)
		throw ParsingException("Too many arguments in return directive");

	// 1) Code valide ?
	if (code != 301 && code != 302 && code != 307 && code != 308)
		throw ParsingException("Invalid redirect code");

	// 2) URL valide ?
	if (target.empty() || target[0] != '/')
		throw ParsingException("Invalid redirect target");

	// 3) Auto-redirection ?
	if (target == tmp_loc.getDirective("location_uri"))
		throw ParsingException("Infinite self redirect");

	// 4) Redir vers une redir ?
	const std::map<std::string, LocationConfig> &locs = corresponding_server.getLocations();
	std::map<std::string, LocationConfig>::const_iterator it = locs.find(target);//Pour eviter de creer un nouveau couple cle/valeur en cas d'inexistance de target
	
	if (it != locs.end()) {
		std::string ret2 = it->second.getDirective("return");
		if (!ret2.empty())
			throw ParsingException("Redirection towards redirection forbidden");
	}
	else {
		throw ParsingException("Redirect target does not match any location");
	}
}

static void		parseAutoConfig(GlobalConfig & global) {

	std::map<std::string, ServerConfig> tmp_servers = global.getServers();

	// Ici on fait des tests sur les directives globales :
	autoconfig_basic_tests(global, "Global");

	// On parcourt toutes les locations (contenant toutes les directives) de chaque serveur et on verifie les incoherences :
	for (std::map<std::string, ServerConfig>::iterator it = tmp_servers.begin(); it != tmp_servers.end(); ++it)
	{
		// Ici on fait les tests pour chaque serveur :
		ServerConfig		tmp_serv = tmp_servers[it->first];
		autoconfig_basic_tests(tmp_serv, "Server");

		// Ici on fait les tests pour chaque location :
		std::map<std::string, LocationConfig>	tmp_locations = tmp_serv.getLocations();
		for (std::map<std::string, LocationConfig>::iterator itloc = tmp_locations.begin(); itloc != tmp_locations.end(); ++itloc)
		{
			LocationConfig	tmp_loc = itloc->second;
			autoconfig_basic_tests(tmp_loc, "Location");

			// Il faut checker que toutes mes locations ont un resolved_path qui est autorise
			if (!tmp_loc.getDirective("root").empty() && !tmp_loc.getDirective("location_uri").empty()) {
				std::string resolved = resolvePath(tmp_loc.getDirective("root"), tmp_loc.getDirective("location_uri")); // tmp_loc_uri = "/img", etc.
				if (!isSafePath(resolved))
					throw ParsingException("Resolved location path is unsafe");
			}

			parseRedir(tmp_loc, tmp_serv);
		}
	}
}

GlobalConfig AutoConfig(const std::string & filename) {
    GlobalConfig global;
    std::ifstream reader(filename.c_str());
    if (!reader.is_open())
        throw std::runtime_error("Cannot open temporary config file");

    std::string line;
    ServerConfig currentServer;
    LocationConfig currentLocation;
    std::string currentServerId;
    bool inServer = false;
    bool inLocation = false;
    std::string currentLocationURI;

    while (std::getline(reader, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line == "server") {
            if (inServer) {
                if (inLocation) {
                    currentServer.addLocation(currentLocationURI, currentLocation);
                    inLocation = false;
                }
                global.addServer(currentServerId, currentServer);
            }
            currentServer = ServerConfig();
            inServer = true;
            continue;
        }

        if (line.rfind("location ", 0) == 0) {
            if (inLocation) {
                currentServer.addLocation(currentLocationURI, currentLocation);
            }
            inLocation = true;
            currentLocation = LocationConfig();
            size_t pos = line.find(' ');
            currentLocationURI = line.substr(pos + 1);
            continue;
        }

        if (inLocation) {
            AutoConfig_setDirective(currentLocation, line);
        } else if (inServer) {
            AutoConfig_setDirective(currentServer, line);
            currentServerId = currentServer.getDirective("listen");
        } else {
            AutoConfig_setDirective(global, line);
        }
    }

    if (inLocation) {
        currentServer.addLocation(currentLocationURI, currentLocation);
    }
    if (inServer) {
        global.addServer(currentServerId, currentServer);
    }

    reader.close();

    // === HÉRITAGE ICI ===
    {
        std::map<std::string, ServerConfig> &servers = global.accessServers();
        for (std::map<std::string, ServerConfig>::iterator sit = servers.begin();
             sit != servers.end(); ++sit)
        {
            ServerConfig &srv = sit->second;

            // global -> server
            srv.inheritFromGlobal(global);

            // server -> locations
            std::map<std::string, LocationConfig> &locs = srv.accessLocations();
            for (std::map<std::string, LocationConfig>::iterator lit = locs.begin();
                 lit != locs.end(); ++lit)
            {
                lit->second.inheritFromServer(srv);
            }
        }
    }

    // vérifs sur la config déjà héritée
    parseAutoConfig(global);

    // printGlobalConfig(global);
    return global;
}

bool isSafePath(const std::string &path) {

	// 1) doit commencer par "./ressources"
	const std::string base = "./ressources";
	if (path.compare(0, base.size(), base) != 0)
		return false;

	// 2) doit ne contenir aucun ".."
	if (path.find("..") != std::string::npos)
		return false;

		// Sous windows peut poser pb, 
	if (path.find('\\') != std::string::npos)
		return false;

	if (path.find('~') != std::string::npos)
		return false;

	// %2e%2e est l'encodage URL de ".."
	std::string lower = path;
	for (size_t i = 0; i < lower.size(); ++i)
		lower[i] = std::tolower(lower[i]);

	if (lower.find("%2e") != std::string::npos)
		return false;
	
	return true;
}

// Utile pour le template autoconfig.hpp //

bool isValidPort(const std::string& port_str) {
	long port_num = 0;
	std::istringstream iss(port_str);
	
	if (!(iss >> port_num) || !iss.eof()) {
		return false; 
	}

	const long MIN_PORT = 1;
	const long MAX_PORT = 65535;

	return port_num >= MIN_PORT && port_num <= MAX_PORT;
}

bool isValidIP(const std::string& ip_str) {
	if (ip_str.empty())
		return false;
	return true; 
}

void validateListenFormat(const std::string& listen_value, const std::string& context) {
	size_t colon_pos = listen_value.find(':');

	// Vérifie la présence et la position unique du côlon.
	if (colon_pos == std::string::npos || 
		colon_pos == 0 || 
		colon_pos == listen_value.length() - 1 || 
		listen_value.find(':', colon_pos + 1) != std::string::npos) {
		throw ParsingException(context + ": listen must be in format IP:PORT (e.g., 127.0.0.1:8080). Found '" + listen_value + "'");
	}

	// Séparer l'IP et le Port
	std::string ip = listen_value.substr(0, colon_pos);
	std::string port_str = listen_value.substr(colon_pos + 1);

	// Valider l'IP
	if (!isValidIP(ip)) {
		throw ParsingException(context + ": listen directive has an invalid IP address: '" + ip + "'");
	}

	// Valider le Port
	if (!isValidPort(port_str)) {
		throw ParsingException(context + ": listen directive has an invalid or out-of-range port number: '" + port_str + "'. Must be between 1 and 65535.");
	}
}


void validateCgiDirective(const std::string &handlers, const std::string &context)
{
    if (handlers.empty())
        return; // rien à valider

    std::size_t start = 0;
    while (start < handlers.size()) {
        std::size_t end = handlers.find('\n', start);
        if (end == std::string::npos)
            end = handlers.size();

        std::string line = handlers.substr(start, end - start);
        start = end + 1;

        // trim début
        std::size_t i = 0;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        if (i == line.size())
            continue; // ligne vide -> on ignore
        line = line.substr(i);

        // ext = jusqu'au 1er espace ou tab
        std::size_t sp = line.find_first_of(" \t");
        if (sp == std::string::npos) {
            throw ParsingException(context + ": invalid cgi_handler line (missing binary): '" + line + "'");
        }

        std::string ext = line.substr(0, sp);

        // début du binaire (skip espaces)
        std::size_t j = line.find_first_not_of(" \t", sp);
        if (j == std::string::npos) {
            throw ParsingException(context + ": invalid cgi_handler line (missing binary): '" + line + "'");
        }
        std::string prog = line.substr(j);

        // validation de l'extension
        bool ok =
            ext.size() >= 2 &&
            ext[0] == '.' &&
            ext.find('.', 1) == std::string::npos;

        if (!ok) {
            throw ParsingException(context + ": invalid CGI extension in cgi_handler: '" + ext + "'");
        }

        // éventuellement, tu peux aussi vérifier ici que prog n'est pas vide / commence par '/'
        if (prog.empty()) {
            throw ParsingException(context + ": empty CGI binary in cgi_handler");
        }
    }
}


void validateAllowedMethods(const std::string &value, const std::string &context) {
    if (value.empty())
        throw ParsingException(context + ": allowed_methods cannot be empty");

    std::size_t i = 0;
    bool foundAny = false;

    while (i < value.size()) {
        // skip espaces
        while (i < value.size() && std::isspace(static_cast<unsigned char>(value[i])))
            ++i;
        if (i >= value.size())
            break;

        // début du mot
        std::size_t start = i;
        while (i < value.size() && !std::isspace(static_cast<unsigned char>(value[i])))
            ++i;
        std::string token = value.substr(start, i - start);

        if (token != "GET" && token != "HEAD" && token != "POST" && token != "DELETE") {
            throw ParsingException(
                context + ": invalid method in allowed_methods: '" + token +
                "' (must be GET, HEAD, POST, DELETE)"
            );
        }
        foundAny = true;
    }

    if (!foundAny)
        throw ParsingException(context + ": allowed_methods must contain at least one method");
}
