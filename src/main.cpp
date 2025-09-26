/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aisidore <aisidore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/11 19:32:12 by aisidore          #+#    #+#             */
/*   Updated: 2025/09/16 11:34:49 by aisidore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../webserv.hpp"

//Mon code : envoyer du texte dans un serveur qui me le renvoie directement (adresse IPv4)

//Ce qu'il faudrait c'est pouvoir faire des requêtes HTTP au serveur
//et qu'il renvoie la ressource (texte, img ...) ou alors poster des choses
//dans le serveur.

int main()
{
    //1) Créer socket

	// créé un point de communication retourne son FD.
    // int socket(int domain, int type, int protocol);
	// AF_INET : protocole de communication défini dans <sys/socket.h>.
	//C'est le protocole IPv4, compréhensible par des kernel Linux.
	// SOCK_STREAM : le type de socket ici est défini comme two-way et séquentiel.
	// 0: il n'existe qu'un typede protocole pour ce type de socket donc on met 0.
	int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        std::cerr << "Can't create a socket! Quitting" << std::endl;
        return -1;
    }
 
    //2) Connecter l'adresse IP locale et notre port au socket

	//Structure <netinet/in.h> qui décrit une adresse IPv4
	// struct sockaddr_in
	// {
    // sa_family_t    sin_family; // Famille d’adresses, toujours AF_INET pour IPv4
    // in_port_t      sin_port;   // Port, en ordre réseau (par exemple htons(80))
    // struct in_addr sin_addr;   // Adresse IP (struct in_addr)
    // char           sin_zero[8];// Remplissage inutile, garder à 0
	// };
	//Pour renseigner l'adrrsse IP en chaîne de caractère on fait sin_addr.s_addr = inet_addr("203.0.113.5");
	//ou on utilise inet_pton
	// struct in_addr {in_addr_t s_addr; // adresse IPv4 sur 32 bits (ordre réseau) };
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
	//HOWTO 1
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);//"0.0.0.0" : le serveur écoute sur toutes les IP de la machine
 
	//A PROTEGER
    bind(listening, (sockaddr*)&hint, sizeof(hint));//connecte le socket à l’adresse/port donné
 
	//A PROTEGER
    listen(listening, SOMAXCONN);//Le socket peut maintenant recevoir des demandes de connexion avec accept
	//SOMAXCONN : on demande au système de créer la file d'attente la plus longue possible si plusieurs
	//clients se connectent en même temps.
 
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
 
    char host[NI_MAXHOST];      //Adresse du client
    char service[NI_MAXSERV];   //Port du client
 
    memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);
 
	//HOWTO 2
	//ICI ON AFFICHE JUSTE LES DONNEES DU CLIENT, CA N'EST PAS UNE ETAPE POUR CONNECTER
	//LE SERVEUR AU CLIENT
	//getnameinfo convertit l’adresse du client (sockaddr) en nom d’hôte et port lisibles
	//(sockaddr* : comme plus haut, getnameinfo prend comme type sockaddr (IPv4 ou IPv6)
    if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
    {
        std::cout << host << " connected on port " << service << std::endl;
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);//convertit l’IP binaire en chaîne lisible
        std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
		//ntohs converti en entier lisible par l’OS, qu'il soit en big-endian ou little-endian
    }
 
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
