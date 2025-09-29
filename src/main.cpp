/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aisidore <aisidore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/11 19:32:12 by aisidore          #+#    #+#             */
/*   Updated: 2025/09/29 17:30:31 by aisidore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../webserv.hpp"

//Mon code : envoyer du texte dans un serveur qui me le renvoie directement (adresse IPv4)

//accept() attend l'arrivée d'un seul client. Si je veux plusieurs clients il va falloir
//soit faire du multi-threading soit utiliser select, poll, ou epoll...

//Ce qu'il faudrait c'est pouvoir faire des requêtes HTTP au serveur
//et qu'il renvoie la ressource (texte, img ...) ou alors poster des choses
//dans le serveur.

//ATTENTION le recasting doit se faire en mode C++
int main()
{
    //1) Créer socket

	// créé un point de communication retourne son FD.
    // int socket(int domain, int type, int protocol);
	// AF_INET : protocole de communication défini dans <sys/socket.h>.
	//C'est le protocole IPv4, compréhensible par des kernel Linux.
	// SOCK_STREAM : le type de socket ici est défini comme two-way et séquentiel.
	// 0: il n'existe qu'un type de protocole pour ce type de socket donc on met 0.
	int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        std::cerr << "Can't create a socket! Quitting" << std::endl;
        return -1;
    }
 
    //2) Connecter l'adresse IP locale et notre port au socket

    // struct addrinfo de <netdb.h>, permet de spécifier les critères pour retourner une adresse compatible
    // à notre socket. getaddrinfo() retournera toutes les adresses compatibles dans res sous forme de liste
    //chainée.
    // struct addrinfo
    // {
    //     int              ai_flags;     // options : AI_PASSIVE, AI_CANONNAME…
    //     int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
    //     int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM…
    //     int              ai_protocol;  // IPPROTO_TCP, IPPROTO_UDP…
    //     socklen_t        ai_addrlen;   // taille de ai_addr
    //     struct sockaddr *ai_addr;      // pointeur vers l’adresse (sockaddr_in ou sockaddr_in6)
    //     char            *ai_canonname; // nom canonique (facultatif)
    //     struct addrinfo *ai_next;      // pointeur vers l’élément suivant de la liste
    // };
    struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints)); //Les champs non définis contiennent des valeurs aléatoires et peuvent faire échouer getaddrinfo
    hints.ai_family = AF_INET;// IPv4
    hints.ai_socktype = SOCK_STREAM;// TCP
    hints.ai_flags = AI_PASSIVE; // adresse pour un serveur qui écoute

    // NULL = 0.0.0.0:54000, le serveur accepte les connexions sur toutes les IP à condition que le port soit 54000
    int status = getaddrinfo(NULL, "54000", &hints, &res);
	if (status != 0) {
		std::cerr << "getaddrinfo error: " << gai_strerror(status) << "\n";
		return -1;
	}

	//connecte le socket à l’adresse/port donné
    // bind(listening, (sockaddr*)&hint, sizeof(hint));//connecte le socket à l’adresse/port donné
    // if(bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1)
    if(bind(listening, res->ai_addr, res->ai_addrlen) == -1)
	{
		std::cerr << "Can't bind socket to IP/Port" << std::endl;
		return -2;
	}
 
	//Le socket peut maintenant recevoir des demandes de connexion avec accept
	if (listen(listening, SOMAXCONN) == -1)
	{
		std::cerr << "Socket can't listen" << std::endl;
		return -3;		
	}
	//SOMAXCONN : on demande au système de créer la file d'attente la plus longue possible si plusieurs
	//clients se connectent en même temps.
 
    freeaddrinfo(res);

    // 3) Attendre une connexion

    sockaddr_in client;//Structure qui décrit un client qui va se connecter
    socklen_t clientSize = sizeof(client);//socklen_t est un type qui mesure la taille d'un sockaddr_in

	//Le programme est bloqué ici, dans l'attente d'un client.
	//listening est 'duppliqué' pour créer un nouveau socket (client), et reste disponible pour de futurs
	//connexions. accept retourne le FD ne ce nouveau socket client.
	//(sockaddr*) : le type attendu par accept() qui peut contenir une IP IPv4 ou IPv6 (sockaddr_in est propre
	//à IPv4)
	//clientSize : car IPv4 et IPv6 ne font pas la même taille

    int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
	if (clientSocket == -1)
	{
		std::cerr << "Problem with client connecting" << std::endl;
		return -4;		
	}
 
    char host[NI_MAXHOST];      //Adresse du client
    char service[NI_MAXSERV];   //Port du client
 
    memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);

    // Pour une IPv4 je peux faire par exemple :
    unsigned char *ip = (unsigned char *)&client.sin_addr.s_addr;
    std::cout << (int)ip[0] << '.'
            << (int)ip[1] << '.'
            << (int)ip[2] << '.'
            << (int)ip[3]
            << " connected on port " << ntohs(client.sin_port) << std::endl;

    // Fermeture de listening, dans notre exemple on attendait qu'un seul client
    close(listening);
 
    // Récupère et echo le message envoyé par le client
    char buf[4096];
 
    while (true)
    {
        memset(buf, 0, 4096);
 
        // recv bloque jusqu’à ce que le client envoie quelque chose ou ferme la connexion.
		// si le client envoie de la data, recv retourne le nombre d'octets reçus.
        int bytesReceived = recv(clientSocket, buf, 4096, 0);
        if (bytesReceived == -1)
        {
            std::cerr << "Error in recv(). Quitting" << std::endl;
            break;
        }
 
        if (bytesReceived == 0)
        {
            std::cout << "Client disconnected " << std::endl;
            break;
        }
 
        std::cout << std::string(buf, 0, bytesReceived) << std::endl;
 
        // Echo message back to client
		// bytesReceived + 1 inclut le caractère nul final
        send(clientSocket, buf, bytesReceived + 1, 0);
    }
 
    // Close the socket
    close(clientSocket);
 
    return 0;
}
