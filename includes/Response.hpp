#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "./webserv.hpp"

class Response
{
    public:
        Response(const Request request);//On créé une réponse adaptée, et on envoie la donnée dans "Body"
        ~Response();

        //Ici on formalise notre instance pour permettre une réponse au client (code d'erreur etc...)

    private:
        Response();//Inutile
        Response(const Response &copy);//Inutile
        Response			&operator=(const Response &rhs);//Inutile

};

//Surcharge de l'operateur d'injection pour composer une reponse

#endif