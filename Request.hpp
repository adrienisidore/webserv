#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "./webserv.hpp"

class Request
{
    public:
        Request(const char client_request);//la chaîne de caractère qu'envoie le client
        ~Request();

        //Ici on formalise notre instance pour savoir quelle donnée aller chercher

    private:
        std::string method;//GET, PUT ...
        Request();//une requête vide n'est pas possible
        Request(const Request &copy);//Inutile
        Request			&operator=(const Request &rhs);//Inutile

};

#endif