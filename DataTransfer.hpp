#ifndef DATATRANSFER_HPP
# define DATATRANSFER_HPP

# include "./webserv.hpp"
# include "./Request.hpp"

class DataTransfer
{
    public:
        DataTransfer(const Request request);//On récupère une rquête et on va chercher la donnée correspondante
        ~DataTransfer();

        //Ici on formalise notre instance pour envoyer la bonne donnée (chemin), correctement (chunk)

    private:
        DataTransfer();//Inutile
        DataTransfer(const DataTransfer &copy);//Inutile
        DataTransfer			&operator=(const DataTransfer &rhs);//Inutile

};

#endif