#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "webserv.hpp"

//formalize a proper response
class Response : public HTTPcontent {
    public:
        Response();
        virtual ~Response();

		int 			fetch();
		void			execute();
		void			_post_();
		void			_delete_();
		void			_get_();
		void			_error_();

		void			copyFrom(HTTPcontent& other);
		void			createFromCgi(CGI &cgi);


		bool			keep_alive() const;

		CGI 			_cgi;
		bool 			_autoindex;

    private:

		std::string		try_multiple_indexes(std::vector<std::string> indexes);
		bool			is_cgi();
		void			build_valid_response_get(std::string body);
		std::string		get_autoindex();

		void			checkPermissions(std::string path, bool is_cgi);

		void 			checkRedir();

		void			checkAllowedMethods();
		void			buildPath();




};

#endif
