#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "webserv.hpp"

//formalize a proper response
class Response : public HTTPcontent {
    public:
        Response();
        ~Response();

		//Execute tout le code, remplit le body si necessaire et renvoie l'outpipe si CGI (0 sinon)
		//A la fin de hub le body de response est pret a etre envoye au client
		void 			hub();

    private:
		void			checkPermissions();
		void			buildPath();//Fais appel a checkPermissions, si tout est ok on peut continuer

		// CGI _cgi;
		void			launchCGI();//2)  ca execute la methode ou le CGI
		//en remplissant si necessaire le _body : applyMethod() va a la fois faire un _cgi.copyFrom(*this) puis _cgi.launchExecve();
		// On cree un getter pour envoyer _outpipe[0] au pollfd : pollfd_wrapper(response._cgi.getOutPipe()) et l'ajouter dans les bonnes map etc


		void			copyFrom(HTTPcontent& other);


		//C'est TCPConnection qui renvoie la response
};

//PARAGRAPHE UTILE POUR ENVOYER EFFICACEMENT UNE REPONSE AVEC poll() :

// When poll() says the file descriptor is ready for reading/writing,
// the generateResponse() function will be called again,
// recognize (based on the previously saved state) that this is not the first time
// it is being called, and continue where it left off last time. Every time it returns,
// the main server loop checks if the response is ready,
// so that it can be sent to the client.

#endif
