#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "webserv.hpp"

//formalize a proper response
class Response : public HTTPcontent {
    public:
        Response();
        ~Response();

    private:
		void			buildPath();

		void			applyMethod();//2) Fais appel a checkPermissions, si tout est ok ca execute la methode ou le CGI,
		//en remplissant si necessaire le _body

		void			checkPermissions();
		void			cgi_handler();//fait appel a une instance CGI pour remplir son body

		void			copyFrom(HTTPcontent& other);

		//Quand le _body de response est rempli (si GET) alors on construit la reponse
		void 			buildResponse();//HTTP/1.1 200 OK	\r\n	[headers] Content-length etc...
		//C'est TCPConnection qui renvoie la response

};

//PARAGRAPHE UTILE POUR ENVOYER EFFICACEMENT UNE REONSE AVEC poll() :

// When poll() says the file descriptor is ready for reading/writing,
// the generateResponse() function will be called again,
// recognize (based on the previously saved state) that this is not the first time
// it is being called, and continue where it left off last time. Every time it returns,
// the main server loop checks if the response is ready,
// so that it can be sent to the client.

#endif
