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
//
//
static std::string resolvePath(const std::string &root, const std::string &target)
{
	std::string full = root;

	// Ajouter un slash si root n'en a pas
	if (!full.empty() && full[full.size() - 1] == '/')
		full.erase(full.size() - 1);

	full += target; // target commence par "/"

	return full;
}

static bool isSafePath(const std::string &path) {

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


// Check la pertinence des valeurs du fichier de config : code redirection incorrect, target redir manquant, ...
static void		parseAutoConfig(GlobalConfig & global) {

	std::map<std::string, ServerConfig> tmp_servers = global.getServers();

	// Ici on fait des tests sur les directives globales :
	std::string global_root = global.getDirective("root");
	if (!global_root.empty() && !isSafePath(global_root))
		throw ParsingException("Global root path is unsafe");


	// On parcourt toutes les locations (contenant toutes les directives) de chaque serveur et on verifie les incoherences :
	for (std::map<std::string, ServerConfig>::iterator it = tmp_servers.begin(); it != tmp_servers.end(); ++it)
	{
		// Ici on fait les tests pour chaque serveur :
		ServerConfig		tmp_serv = tmp_servers[it->first];
		std::string server_root = tmp_serv.getDirective("root");
		if (!server_root.empty() && !isSafePath(server_root))
			throw ParsingException("Server root path is unsafe");

		// Ici on fait les tests pour chaque location :
		std::map<std::string, LocationConfig>	tmp_locations = tmp_serv.getLocations();
		for (std::map<std::string, LocationConfig>::iterator itloc = tmp_locations.begin(); itloc != tmp_locations.end(); ++itloc)
		{
			
			LocationConfig	tmp_loc = itloc->second;

			// Il faut checker que toutes mes locations ont un resolved_path qui est autorise !!!!!!!!!!!
			std::string tmp_loc_root = tmp_loc.getDirective("root");
			std::string tmp_loc_uri  = tmp_loc.getDirective("location_uri");
			// Vérifier que le root de la location est sûr
			if (!tmp_loc_root.empty() && !isSafePath(tmp_loc_root))
				throw ParsingException("Location root path is unsafe");
			// Vérifier le resolved_path de la location
			if (!tmp_loc_root.empty() && !tmp_loc_uri.empty()) {
				std::string resolved = resolvePath(tmp_loc_root, tmp_loc_uri); // tmp_loc_uri = "/img", etc.
				if (!isSafePath(resolved))
					throw ParsingException("Resolved location path is unsafe");
			}

			parseRedir(tmp_loc, tmp_serv);

		}
	}
}

// GlobalConfig AutoConfig(const std::string & filename) {
//     GlobalConfig global;
//     std::ifstream reader(filename.c_str());
//     if (!reader.is_open())
//         throw std::runtime_error("Cannot open temporary config file");

//     std::string line;
//     ServerConfig currentServer;
//     LocationConfig currentLocation;
//     std::string currentServerId;
//     bool inServer = false;
//     bool inLocation = false;
//     std::string currentLocationURI;

//     while (std::getline(reader, line)) {
//         if (line.empty() || line[0] == '#') continue; // ignorer les lignes vides et les commentaires

//         if (line == "server") {
//             if (inServer) {
//                 if (inLocation) { // on termine la location en cours
//                     currentServer.addLocation(currentLocationURI, currentLocation);
//                     inLocation = false;
//                 }
//                 // ajouter le serveur terminé
//                 global.addServer(currentServerId, currentServer);
//             }
//             currentServer = ServerConfig();
//             inServer = true;
//             continue;
//         }

//         if (line.rfind("location ", 0) == 0) { // ligne commence par "location "
//             if (inLocation) { // sauvegarder la location précédente
//                 currentServer.addLocation(currentLocationURI, currentLocation);
//             }
//             inLocation = true;
//             currentLocation = LocationConfig();
//             size_t pos = line.find(' ');
//             currentLocationURI = line.substr(pos + 1);
//             continue;
//         }

//         // ligne de directive
//         if (inLocation) {
//             AutoConfig_setDirective(currentLocation, line);
//         } else if (inServer) {
//             AutoConfig_setDirective(currentServer, line);
// 			//CHECKER QUE CES DIRECTIVES SONT ABSOLUMENT OBLIGATOIRE
//             currentServerId = currentServer.getDirective("listen");
//         } else {
//             AutoConfig_setDirective(global, line);
//         }
//     }

//     // sauvegarder le dernier serveur/location
//     if (inLocation) {
//         currentServer.addLocation(currentLocationURI, currentLocation);
//     }
//     if (inServer) {
//         global.addServer(currentServerId, currentServer);
//     }

//     reader.close();

// 	parseAutoConfig(global);

//     printGlobalConfig(global);
//     return global;
// }


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
