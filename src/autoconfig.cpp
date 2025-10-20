#include "autoconfig.hpp"

// --- Helper : indentation lisible ---
// static void printIndent(int depth) {
// 	for (int i = 0; i < depth; ++i)
// 		std::cout << "    ";
// }

// --- LOCATION CONFIG ---
// static void printLocationConfig(const std::string &path, const LocationConfig &loc, int depth = 2) {
// 	printIndent(depth);
// 	std::cout << "location " << path << " {" << std::endl;

// 	const std::map<std::string, std::string> &directives = loc.getDirectives();
// 	for (std::map<std::string, std::string>::const_iterator it = directives.begin();
// 		 it != directives.end(); ++it) {
// 		printIndent(depth + 1);
// 		std::cout << it->first << " " << it->second << ";" << std::endl;
// 	}

// 	printIndent(depth);
// 	std::cout << "}" << std::endl;
// }

// --- SERVER CONFIG ---
// static void printServerConfig(const std::string &key, const ServerConfig &server, int depth = 1) {
// 	printIndent(depth);
// 	std::cout << "server (" << key << ") {" << std::endl;

// 	const std::map<std::string, std::string> &directives = server.getDirectives();
// 	for (std::map<std::string, std::string>::const_iterator it = directives.begin();
// 		 it != directives.end(); ++it) {
// 		printIndent(depth + 1);
// 		std::cout << it->first << " " << it->second << ";" << std::endl;
// 	}

// 	// Locations
// 	const std::map<std::string, LocationConfig> &locs = server.getLocations();
// 	for (std::map<std::string, LocationConfig>::const_iterator it = locs.begin();
// 		 it != locs.end(); ++it)
// 		printLocationConfig(it->first, it->second, depth + 1);

// 	printIndent(depth);
// 	std::cout << "}" << std::endl;
// }

// --- GLOBAL CONFIG ---
// static void printGlobalConfig(GlobalConfig &global) {

// 	const std::map<std::string, std::string> &directives = global.getDirectives();
// 	for (std::map<std::string, std::string>::const_iterator it = directives.begin();
// 		 it != directives.end(); ++it) {
// 		printIndent(1);
// 		std::cout << it->first << " " << it->second << ";" << std::endl;
// 	}

// 	std::map<std::string, ServerConfig> &servers = global.accessServers();
// 	for (std::map<std::string, ServerConfig>::const_iterator it = servers.begin();
// 		 it != servers.end(); ++it)
// 		printServerConfig(it->first, it->second, 1);
// }


GlobalConfig AutoConfig(const std::string & filename) {
    GlobalConfig global;
    std::ifstream reader(filename.c_str());
    if (!reader.is_open())
        throw std::exception();

    std::string line;
    ServerConfig currentServer;
    LocationConfig currentLocation;
    std::string currentServerId;
    bool inServer = false;
    bool inLocation = false;
    std::string currentLocationURI;

    while (std::getline(reader, line)) {
        if (line.empty()) continue; // ignorer les lignes vides

        if (line == "server") {
            if (inServer) {
                if (inLocation) { // on termine la location en cours
                    currentServer.addLocation(currentLocationURI, currentLocation);
                    inLocation = false;
                }
                // ajouter le serveur terminé
                global.addServer(currentServerId, currentServer);
            }
            currentServer = ServerConfig();
            inServer = true;
            continue;
        }

        if (line.rfind("location ", 0) == 0) { // ligne commence par "location "
            if (inLocation) { // sauvegarder la location précédente
                currentServer.addLocation(currentLocationURI, currentLocation);
            }
            inLocation = true;
            currentLocation = LocationConfig();
            size_t pos = line.find(' ');
            currentLocationURI = line.substr(pos + 1);
            continue;
        }

        // ligne de directive
        if (inLocation) {
            AutoConfig_setDirective(currentLocation, line);
        } else if (inServer) {
            AutoConfig_setDirective(currentServer, line);
            //Verifier que listen contient IP et le Port
            currentServerId = currentServer.getDirective("server_name") + "/" + currentServer.getDirective("listen");
        } else {
            AutoConfig_setDirective(global, line);
        }
    }

    // sauvegarder le dernier serveur/location
    if (inLocation) {
        currentServer.addLocation(currentLocationURI, currentLocation);
    }
    if (inServer) {
        global.addServer(currentServerId, currentServer);
    }

    reader.close();

    return global;
    // printGlobalConfig(global);
}