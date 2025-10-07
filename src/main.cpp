/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aisidore <aisidore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/11 19:32:12 by aisidore          #+#    #+#             */
/*   Updated: 2025/10/07 12:31:37 by aisidore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../webserv.hpp"

//NB : le serveur doit pouvoir délivrer des pages statiques (configurables)

//NB 2 : Il ne faut pas que l'utilisateur puisse changer les droits des fichiers enregistrés sur mon serveur,
// (via le terminal ou l'interface graphique)
// entre 2 lancement de ./webserv, comme ça si le serveur s'éteint et redémarre : on retrouve les ressources.

//A FAIRE * : Ctrl + C : on doit pouvoir sortir proprement (gestion de signaux ?)

//Que se passe t il si je lance plusieurs webserv sur le même port ? Sur un autre port ?
// Le sujet dit que le 1er serveur répondra aux requêtes destinées à aucun des autres serveurs lancés :
// The first server for a host:port will be the default for this host:port (meaning it
// will respond to all requests that do not belong to another server).

int main()
{
                                        //1) Créer socket, çàd un point de communication identifiable par son fd(int) : int socket(int domain, int type, int protocol);

	//• AF_INET (famille de domaine acceptée) : le socket intéragit avec des IPv4
	//• SOCK_STREAM (type de socket) : les données envoyées avec send() arrivent dans le même ordre avec recv(), les erreurs sont gérées par le protocole TCP.
	//Envoie des infor;qtions de facon continue
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
 
	//On affiche l'IP/Port auquel listening est connecte
	unsigned char *ip = reinterpret_cast<unsigned char *>(res->ai_addr);
	std::cout << "Listening connected to  "
			<< static_cast<int>(ip[0]) << '.'
			<< static_cast<int>(ip[1]) << '.'
			<< static_cast<int>(ip[2]) << '.'
			<< static_cast<int>(ip[3]) << std::endl;
			
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
    struct pollfd fds[10];// Le nombre maximal de client est défini arbitrairement. Le nombre max de clients qui peuvent
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
            
            //Si le serveur est prêt, il attend la connexion d'un client.
            //listening est 'duppliqué' pour créer un nouveau socket (client), et reste disponible pour de futurs
            //connexions. accept() retourne le FD de ce nouveau socket client.
            //(sockaddr*) : le type attendu par accept() qui peut contenir une IP IPv4 ou IPv6 (sockaddr_in est propre
            //à IPv4)
            //clientSize : car IPv4 et IPv6 ne font pas la même taille
            int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
            if (clientSocket == -1)
            {
                std::cerr << "Problem with client connecting" << std::endl;
                return (-5);		
            }



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
                    fds[i] = fds[nfds-1]; // On remplace le client parti par le dernier client de la liste (par ex fds[6] = fds[10])
                    nfds--; //On réduit le nombre de clients à surveiller de 1
                    i--; //Le client 10 qui a pris la place de 6 doit être surveillé à la prochane itération, sinon il est ignoré pendant 1 tour
                    continue;//On s'arrête là. C'est un peu useless dans la mesure où le reste du code n'est pas enclenché (c'est du "else")
                }
                else
                {
					//Client se deconnecte a cause du 404 Error ou parce qu'il a finit de telecharger la page ?
                    std::cout << "Client " << i << " says: " << buf << std::endl;
                    //Fonction interdite
                    if (strstr(buf, "GET /favicon.ico") != NULL)
                    {
                        const char* notFound =
                            "HTTP/1.1 404 Not Found\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 23\r\n"
                            "Connection: close\r\n\r\n"
                            "404 Not Found: favicon.ico";
                            std::cout << "Server's response: " << notFound << std::endl;
                            send(fds[i].fd, notFound, strlen(notFound), 0);
                    }
                    else
                    {
                        serverReply(fds[i].fd, "ServerInterface.html");
                    }
                    // serverReply(fds[i].fd, "ServerInterface.html");
                }
            }
        }
        
    }
 
    close(listening);

    return (0);
}
