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
//Step 1 : pouvoir connecter plusieurs clients 
//Step 2 : enregistrer du texte dans le serveur (PUT)

//NB : accept() attend l'arrivée d'un seul client. Si je veux plusieurs clients il va falloir
//soit faire du multi-threading soit utiliser select, poll, ou epoll...

int main()
{
                                        //1) Créer socket, çàd un point de communication identifiable par son fd(int) : int socket(int domain, int type, int protocol);

	//• AF_INET (famille de domaine acceptée) : le socket intéragit avec des IPv4
	//• SOCK_STREAM (type de socket) : les données envoyées avec send() arrivent dans le même ordre avec recv(), les erreurs sont gérées par le protocole TCP
	//• 0 (quel protocole utilisé, 0 est celui par défaut): AF_INET + SOCK_STREAM → TCP par défaut
	int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        std::cerr << "Can't create a socket! Quitting" << std::endl;
        return (-1);
    }
 
                                        //2) Connecter le couple IP/port au socket

    // struct addrinfo de <netdb.h>, permet de spécifier les critères pour retourner une adresse compatible
    // à notre socket. getaddrinfo() retournera toutes les adresses compatibles dans *res sous forme de liste
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

    struct addrinfo hints;// structure que tu remplis pour indiquer tes critères (type de socket, famille d’adresses…)
    struct addrinfo *res;// pointeur qui recevra la liste chaînée des adresses compatibles renvoyées par getaddrinfo
	memset(&hints, 0, sizeof(hints)); //Les champs non définis contiennent des valeurs aléatoires et peuvent faire échouer getaddrinfo, donc on met tout à 0
    hints.ai_family = AF_INET;// On veut les adresses IPv4
    hints.ai_socktype = SOCK_STREAM;// On veut les adresses compatibles avec le protocole TCP
    hints.ai_flags = AI_PASSIVE; //les adresses retournées seront utilisées pour un serveur qui écoute (bind())

    // NULL + AI_PASSIVE → getaddrinfo retourne l’adresse 0.0.0.0, çàd retourne dans *res toutes les adresses locales valides pour écouter sur le port 54000
    // Définition port : programme auquel je dois envoyer la donnée reçu via l'adresse IP
    int status = getaddrinfo(NULL, "54000", &hints, &res);
	if (status != 0) {
		std::cerr << "getaddrinfo error: " << gai_strerror(status) << "\n";
		return (-2);
	}

	//connecte le socket au couple adresse/port donné, en effet ai_addr contient :
    // sin_addr → 0.0.0.0 (toutes les interfaces)
    // sin_port → 54000 (converti en network byte order)
    if(bind(listening, res->ai_addr, res->ai_addrlen) == -1)
	{
		std::cerr << "Can't bind socket to IP/Port" << std::endl;
		return (-3);
	}
 
	//Le socket peut maintenant recevoir une demande de connexion avec accept
	if (listen(listening, SOMAXCONN) == -1)
	{
		std::cerr << "Socket can't listen" << std::endl;
		return (-4);		
	}
	//SOMAXCONN : on demande au système de créer la file d'attente la plus longue possible si plusieurs
	//clients se connectent en même temps.
 
    freeaddrinfo(res);

                                        // 3) Attendre une connexion

    sockaddr_in client;//Structure qui décrit un client qui va se connecter
    socklen_t clientSize = sizeof(client);//socklen_t est un type qui mesure la taille d'un sockaddr_in

	//Le programme est bloqué ici, dans l'attente d'un client.
	//listening est 'duppliqué' pour créer un nouveau socket (client), et reste disponible pour de futurs
	//connexions. accept retourne le FD de ce nouveau socket client.
	//(sockaddr*) : le type attendu par accept() qui peut contenir une IP IPv4 ou IPv6 (sockaddr_in est propre
	//à IPv4)
	//clientSize : car IPv4 et IPv6 ne font pas la même taille
    int clientSocket = accept(listening, reinterpret_cast<sockaddr*>(&client), &clientSize);
	if (clientSocket == -1)
	{
		std::cerr << "Problem with client connecting" << std::endl;
		return (-5);		
	}

    // Pour une IPv4 je peux faire par exemple :
    unsigned char *ip = reinterpret_cast<unsigned char *>(&client.sin_addr.s_addr);
    std::cout << static_cast<int>(ip[0]) << '.'
            << static_cast<int>(ip[1]) << '.'
            << static_cast<int>(ip[2]) << '.'
            << static_cast<int>(ip[3])
            << " connected on port " << ntohs(client.sin_port) << std::endl;

    // Fermeture de listening, dans notre exemple on attendait qu'un seul client
    close(listening);
 
    // Récupère la donnée octet par octet
    char buf[4096];//4096 arbitraire, si le message est plus grand il faut faire plusieurs appels successifs de recv
 
    while (true)
    {
        memset(buf, 0, 4096);
 
        // recv bloque jusqu’à ce que le client envoie quelque chose ou ferme la connexion.
		// si le client envoie de la data, recv retourne le nombre d'octets reçus.
        // Si ce nombre est > 4096 il faut rappeler recv (faire une fonction dédiée).
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

        // * * * Ici on peut essayer de sauvegarder ce qu'il y a dans buff sur le serveur ? * * * //
        std::cout << std::string(buf, 0, bytesReceived) << std::endl;
        // On renvoie le message reçu au client, pour voir si les octets sont reçus/envoyés dans le bon ordre ("bytesReceived + 1" sert à inclure le \0)
        send(clientSocket, buf, bytesReceived + 1, 0);
    }
 
    close(clientSocket);

    return (0);
}
