#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "webserv.hpp"

//formalize a proper response
class Response : public HTTPcontent {
    public:
        Response();
        virtual ~Response();

		int 			hub();

		void			copyFrom(HTTPcontent& other);

		CGI _cgi;

    private:
		void			checkPermissions();
		void			buildPath();

};

//PARAGRAPHE UTILE POUR ENVOYER EFFICACEMENT UNE REPONSE AVEC poll() :

// When poll() says the file descriptor is ready for reading/writing,
// the generateResponse() function will be called again,
// recognize (based on the previously saved state) that this is not the first time
// it is being called, and continue where it left off last time. Every time it returns,
// the main server loop checks if the response is ready,
// so that it can be sent to the client.

#endif
