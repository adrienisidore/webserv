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
    // setsockopt() sert à configurer des options spécifiques sur un socket
    // listening → socket sur lequel tu veux appliquer l’option.
    // SOL_SOCKET → on modifie le socket et non pas le protocole qu'il utilise.
    // SO_REUSEADDR → option permettant de binder un socket sur une adresse/port déjà utilisés en TIME_WAIT,
    // et donc en cas d'arrêt brutal du serveur (Ctrl + C) on peut réutiliser listening
    // &opt → valeur à appliquer (ici 1 = activer).
    // sizeof(opt) → taille de la valeur.
    //NB: TIME_WAIT (qui dure 1 à 4 minutes) évite que des paquets d'une ancienne connexion soit attribuée
    //au niveau client du port. C'est une sécurité du protocole TCP.
    int opt = 1;
    setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
		std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
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
	//SOMAXCONN : on demande au système de créer la file d'attente la plus longue possible si le nb maximal de clients
	//connectés est atteint. Si la file d'attente est pleine alors les clients qui essaient de se connecter reçoivent : ECONNREFUSED
	if (listen(listening, SOMAXCONN) == -1)
	{
		std::cerr << "Socket can't listen" << std::endl;
		return (-4);		
	}
    freeaddrinfo(res);

                                        // 3) Préparer l'arrivée de plusieurs clients
    // Tableau pour poll : un pour "listening", deux pour "clients"
    // struct pollfd
    // {
    //     int   fd;       // le descripteur à surveiller (socket, fichier, etc.)
    //     short events;   // ce qu’on veut surveiller (ex: POLLIN, POLLOUT, POLLERR : erreur)
    //     short revents;  // ce qui s’est réellement passé (rempli par poll), qu'on test après l'appel
    // };
    struct pollfd fds[3];// Le nombre de client est défini arbitrairement. Le nombre max de clients qui peuvent
    //être connectés en même temps dépend de la mémoire de l'ordi.
    memset(fds, 0, sizeof(fds));
    fds[0].fd = listening;       // socket d’écoute
    fds[0].events = POLLIN;      // on surveille les connexions entrantes
    int nfds = 1; // Nombre de fd ouverts : au début seulement listening
    // Pour récupérer les données que le client envoie
    char buf[4096];//4096 arbitraire, si le message est plus grand il faut faire plusieurs appels successifs de recv


                                        // 4) Attendre une connexion

    while (true)
    {
            // poll() permet de gérer plusieurs clients (ceux dans fds) avec un seul thread et sans bloquer le serveur.
            // Donc recv() n'est jamais bloqué par un client, et s'enclenche pour le client qui est prêt.
            // Idem pour accept() : il s'enclenche uniquement si listening est prêt.
            int ready = poll(fds, nfds, -1);//-1 : attendre indéfiniment
            if (ready == -1)
            {
                std::cerr << "poll error" << std::endl;
                break;
            }


            //Listening a été sollicité par un client
            if (fds[0].revents & POLLIN)
            {
            sockaddr_in client;//Structure qui décrit un client qui va se connecter
            memset(&client, 0, sizeof(client));
            socklen_t clientSize = sizeof(client);////socklen_t est un type qui mesure la taille d'un sockaddr_in. Pourquoi créer une variable dédiée ?
            
            //Le programme est bloqué ici, dans l'attente d'un client.
            //listening est 'duppliqué' pour créer un nouveau socket (client), et reste disponible pour de futurs
            //connexions. accept retourne le FD de ce nouveau socket client.
            //(sockaddr*) : le type attendu par accept() qui peut contenir une IP IPv4 ou IPv6 (sockaddr_in est propre
            //à IPv4)
            //clientSize : car IPv4 et IPv6 ne font pas la même taille
            int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
            if (clientSocket == -1)
            {
                std::cerr << "Problem with client connecting" << std::endl;
                return (-5);		
            }

            // on accepte max 2 clients pour le moment
            //Si plus de 2 clients : MSG_DONTWAIT empêche recv de bloquer si aucun octet n’est dispo
            if (nfds < 3)
            {
                fds[nfds].fd = clientSocket;
                fds[nfds].events = POLLIN;
                nfds++;
                // Je peux afficher l'IPv4 du client qui vient de se connecter :
                unsigned char *ip = reinterpret_cast<unsigned char *>(&client.sin_addr.s_addr);
                std::cout << "New client "
                          << static_cast<int>(ip[0]) << '.'
                          << static_cast<int>(ip[1]) << '.'
                          << static_cast<int>(ip[2]) << '.'
                          << static_cast<int>(ip[3])
                          << " connected on port " << ntohs(client.sin_port) << std::endl;
            }
            else
            {
                std::cout << "Too many clients, last one has been disconnected" << std::endl;
                close(clientSocket);
            }
        }

        for (int i = 1; i < nfds; i++)
        {
            //Si le serveur reçoit des données d'un des sockets
            if (fds[i].revents & POLLIN)
            {
                memset(buf, 0, sizeof(buf));
                // recv bloque jusqu’à ce que le client envoie quelque chose ou ferme la connexion,
                // mais avec poll() ce n'est plus le cas.
                // Si le client envoie de la data, recv retourne le nombre d'octets reçus.
                // Si ce nombre est > 4096 il faut rappeler recv (faire une fonction dédiée).
                int bytesReceived = recv(fds[i].fd, buf, sizeof(buf), 0);

                if (bytesReceived <= 0)
                {
                    if (bytesReceived == -1)
                    {
                        std::cerr << "Error in recv() for client " << i << std::endl;
                    }
                    std::cout << "Client "  << i << " disconnected" << std::endl;;
                    close(fds[i].fd);
                    fds[i] = fds[nfds-1]; // supprime le client en compactant
                    nfds--;
                }
                else
                {
                    std::cout << "Client says: " << buf << std::endl;
                    // * * * Ici on peut essayer de sauvegarder ce qu'il y a dans buff sur le serveur ? * * * //

                    // On renvoie le message reçu au client, pour voir si les octets sont reçus/envoyés dans le bon ordre ("bytesReceived + 1" sert à inclure le \0)
                    send(fds[i].fd, buf, bytesReceived + 1, 0);
                }
            }
        }
        
    }
 
    close(listening);

    return (0);
}
