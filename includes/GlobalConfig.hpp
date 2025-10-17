#ifndef GLOBALCONFIG_HPP
# define GLOBALCONFIG_HPP

//Code handled :
//100, 200, 201, 400, 403, 404, 405, 408, 411, 413, 414, 500, 501

#include "./webserv.hpp"

// Global → paramètres généraux pour tout le serveur (ex. root, error_page, etc.), le fichier de configuration
//  └── server → paramètres pour un “virtual host” (une IP:port, un site)
//        └── location → paramètres pour une route spécifique (ex. /images/)
			// ├── location / {}
			// ├── location /images {}
			// └── location /cgi-bin {}
			// }

// ex de server context :
// server {
//     listen 8080;
//     root html;
//		limit_except GET
// }

// ex de location context :
// location /images {
//     root /var/www/pics;
//     autoindex on;
// }

//Chaque server herite des attributs (et des blockes de directive -> contexts) de GlobalConfig, sans etre un globalconfig.
//Chaque location contenu dans server, herite des attributs de server mais n'est pas un server :

// root /var/www/html;
// client_max_body_size 10M;

// server {
//     root /var/www/site1;

//     location /img {
//         autoindex on;
//     }
// }

// Résultat :

//     /img hérite de client_max_body_size 10M

//     /img hérite aussi de root /var/www/site1

//     mais surcharge autoindex localement


class GlobalConfig {
    std::vector<ServerConfig> servers;
};

class ServerConfig {
    std::vector<LocationConfig> locations;
};

class LocationConfig {
    // Contient les directives du bloc location
};


#endif






////
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdlib>

class LocationConfig {
public:
    std::string root;
    bool autoindex;
    size_t client_max_body_size;

    LocationConfig() : root("html"), autoindex(false), client_max_body_size(1000000) {}

    // Héritage depuis le serveur parent
    void inherit(const class ServerConfig& parent);

    // Override local
    void setDirective(const std::string& key, const std::string& value) {
        if (key == "root")
            root = value;
        else if (key == "autoindex")
            autoindex = (value == "on");
        else if (key == "client_max_body_size")
            client_max_body_size = std::atoi(value.c_str());
    }
};

class ServerConfig {
public:
    std::string root;
    bool autoindex;
    size_t client_max_body_size;
    std::vector<LocationConfig> locations;

    ServerConfig() : root("html"), autoindex(false), client_max_body_size(1000000) {}

    // Héritage depuis le global
    void inherit(const class GlobalConfig& parent);

    LocationConfig& addLocation() {
        LocationConfig loc;
        loc.inherit(*this);
        locations.push_back(loc);
        return locations.back();
    }
};

class GlobalConfig {
public:
    std::string root;
    bool autoindex;
    size_t client_max_body_size;
    std::vector<ServerConfig> servers;

    GlobalConfig() : root("html"), autoindex(false), client_max_body_size(1000000) {}

    ServerConfig& addServer() {
        ServerConfig srv;
        srv.inherit(*this);
        servers.push_back(srv);
        return servers.back();
    }
};

// =================== Héritage ===================
void LocationConfig::inherit(const ServerConfig& parent) {
    root = parent.root;
    autoindex = parent.autoindex;
    client_max_body_size = parent.client_max_body_size;
}

void ServerConfig::inherit(const GlobalConfig& parent) {
    root = parent.root;
    autoindex = parent.autoindex;
    client_max_body_size = parent.client_max_body_size;
}


// =================== Exemple ===================
int main() {
    GlobalConfig global;
    global.root = "/var/www/html";
    global.client_max_body_size = 5000000;

    ServerConfig& srv = global.addServer();
    srv.root = "/var/www/site1";

    LocationConfig& loc = srv.addLocation();
    loc.setDirective("autoindex", "on");

    std::cout << "Server root: " << srv.root << std::endl;
    std::cout << "Location root: " << loc.root << std::endl;
    std::cout << "Location autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;

    return 0;
}
