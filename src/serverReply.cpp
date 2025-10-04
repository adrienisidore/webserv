#include "../webserv.hpp"


// src/
//     RequestManager/ -> analyse de la requête qu'envoie le client et comment elle est traitée (créer une classe Request)
//          parserRequest.cpp ...
//     DataTransfer/ -> (récupère en argument Request) va chercher la data/transfert de la data (client vers serveur, serveur vers client), gérer les chunk
//         putRessource.cpp ...
//         sendRessource.cpp ...          
//     ResponseManager/ -> construction de la réponse qu'envoie le serveur, en fonction de la ressource (taille, existe ou non etc...)
//         responseBuilder.cpp ... (construire une réponse cohérente avec code 2xx 3xx etc)

//ATTENTION : gérer l'ouverture sécurisée d'un fichier :
// 1) Vérifier que le fichier existe avant de tenter de l’ouvrir (stat ou access).
// 2) S’assurer que c’est bien un fichier régulier, pas un répertoire ou un lien symbolique.
// 3) Gérer correctement les erreurs lors de l’ouverture (open) et de la lecture (read).
// 4) Limiter la taille lue pour éviter de surcharger la mémoire si le fichier est énorme.
// 5) Éviter les chemins dangereux (ex. ../) pour empêcher l’accès à des fichiers sensibles si le chemin est fourni par l’utilisateur.

//stat sert à obtenir les informations d’un fichier (taille, permissions, dates…)
// struct stat {
//     dev_t     st_dev;     // ID du périphérique contenant le fichier
//     ino_t     st_ino;     // Numéro d'inode
//     mode_t    st_mode;    // Type et permissions (ex: fichier, dossier, droits rwx)
//     nlink_t   st_nlink;   // Nombre de liens matériels
//     uid_t     st_uid;     // UID du propriétaire
//     gid_t     st_gid;     // GID du groupe
//     dev_t     st_rdev;    // ID périphérique si fichier spécial
//     off_t     st_size;    // Taille du fichier en octets (utile pour Content-Length)
//     blksize_t st_blksize; // Taille optimale de bloc pour I/O
//     blkcnt_t  st_blocks;  // Nombre de blocs alloués
//     time_t    st_atime;   // Date dernier accès
//     time_t    st_mtime;   // Date dernière modification
//     time_t    st_ctime;   // Date dernier changement d’inode (métadonnées)
// };

//filename : chemin relatif à partir de la localisation de ./webserv
void serverReply(int clientSocket, const char *filename)
{
    struct stat fileStat;
    if (stat(filename, &fileStat) == -1) return;//Envoyer code d'erreur serveur
    int fileSize = fileStat.st_size;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) return;//Il faudra envoyer le bon code d'erreur 4xx/5xx si problème

    // Charger fichier
    char *fileContent = new char[fileSize];
    read(fd, fileContent, fileSize);
    close(fd);

    // Construire réponse complète (headers + body)
    // const char *header =
    //     "HTTP/1.1 200 OK\r\n"
    //     "Content-Type: text/html\r\n"
    //     "Connection: keep-alive\r\n\r\n";
    
    std::ostringstream string_fileSize;
    string_fileSize << fileSize;
    std::string header = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + string_fileSize.str() + "\r\n"
        "Connection: keep-alive\r\n\r\n";


    int responseSize = strlen(header.c_str()) + fileSize;//Fonction interdite
    char *response = new char[responseSize];
    memcpy(response, header.c_str(), strlen(header.c_str()));//Fonction interdite
    memcpy(response + strlen(header.c_str()), fileContent, fileSize);//Fonction interdite

    std::cout << "Server's response: " << response << std::endl;

    send(clientSocket, response, responseSize, 0);

    delete[] fileContent;
    delete[] response;
}





// void sendResponse(int clientSocket, const char *filename)
// {
//     struct stat fileStat;
//     if (stat(filename, &fileStat) == -1) return;//Envoyer code d'erreur serveur
//     int fileSize = fileStat.st_size;

//     int fd = open(filename, O_RDONLY);
//     if (fd == -1) return;//Il faudra envoyer le bon code d'erreur 4xx/5xx si problème

//     // Charger fichier
//     char *fileContent = new char[fileSize];
//     read(fd, fileContent, fileSize);
//     close(fd);

//     // Construire réponse complète (headers + body)
//     const char *header =
//         "HTTP/1.1 200 OK\r\n"
//         "Content-Type: text/html\r\n"
//         "Connection: keep-alive\r\n\r\n";
    
//     // Ajouter : "Content-Length: " + std::to_string(fileSize) + "\r\n"

//     int responseSize = strlen(header) + fileSize;//Fonction interdite
//     char *response = new char[responseSize];
//     memcpy(response, header, strlen(header));//Fonction interdite
//     memcpy(response + strlen(header), fileContent, fileSize);//Fonction interdite

//     std::cout << "Server's response: " << response << std::endl;

//     send(clientSocket, response, responseSize, 0);

//     delete[] fileContent;
//     delete[] response;
// }