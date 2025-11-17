#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "webserv.hpp"

//formalize a proper response
class Response : public HTTPcontent {
    public:
        Response();
        virtual ~Response();

		int 			fetch();
		void			_post_();
		void			_delete_();
		void			_get_();
		void			_error_();

		void			copyFrom(HTTPcontent& other);
		bool			keep_alive() const;

		CGI 			_cgi;
		bool 			_autoindex;

    private:

		std::string		try_multiple_indexes(std::vector<std::string> indexes);
		bool			is_cgi();
		void			build_valid_response_get(std::string body);
		std::string		get_autoindex();

		void			checkPermissions(std::string path);

		void			checkAllowedMethods();
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
