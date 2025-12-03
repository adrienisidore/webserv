#include "webserv.hpp"

HTTPcontent::HTTPcontent() : _code(0), _protocol("HTTP/1.1") {}

void	HTTPcontent::reset(TCPConnection *connection) {
	_code = 0;
	_method.clear();
	_URI.clear();
	_headers.clear();
	_current_header.clear();
	_current_body.clear();
	_connection = connection;
}

void    HTTPcontent::setLocation(const ServerConfig & servconfig) {

    std::string best_match_path = "";
    int max_length = -1; // Use -1 to ensure any match (even "/") is longer

    typedef std::map<std::string, LocationConfig> LocationMap;
    typedef LocationMap::const_iterator           LocationIterator;

    const LocationMap& locations = servconfig.getLocations();

    // 1. Iterate through all locations
    for (LocationIterator it = locations.begin(); it != locations.end(); ++it) {
        
        const std::string& location_path = it->first;

        if (_URI.rfind(location_path, 0) == 0) {
            // rfind because we search backwards starting at position 0 (only 1 check)
            // It's a prefix match. Check if it's the longest one found so far.
            int current_length = static_cast<int>(location_path.length());
            if (current_length > max_length) {
                max_length = current_length;
                best_match_path = location_path;
            }
        }
    }

    // After checking all locations, get the config for the best match
	if (max_length == -1) {
		return setCode(404);
	}
	
	LocationIterator found = locations.find(best_match_path);
	
	if (found == locations.end()) {
		return setCode(404);
	}

	// Success! Assign the configuration from the found iterator's value
	_config = found->second;
}

void	HTTPcontent::setCode(const int & code) {_code = code;}

std::string	HTTPcontent::getMethod() const {return (_method);}

std::string	HTTPcontent::getURI() const {return (_URI);}

int	HTTPcontent::getCode() const {return (_code);}

std::map<std::string, std::string>	HTTPcontent::getHeaders() const {return (_headers);}

LocationConfig						HTTPcontent::getConfig() const { return (_config);}

std::string							HTTPcontent::getPath() const {return _path;}

HTTPcontent::~HTTPcontent() {}

void	HTTPcontent::append_to_header(char buff[BUFF_SIZE], int bytes) {_current_header.append(buff, (size_t)bytes);}

void	HTTPcontent::setCurrentBody(std::string str) {_current_body = str;}

std::string	HTTPcontent::getCurrentBody() const {return _current_body;}


std::string		HTTPcontent::getCurrentHeader() const {return _current_header;}


unsigned long	HTTPcontent::getContentLength() const {return _content_length;}

void	HTTPcontent::setContentLength(unsigned long len) {_content_length = len;}

void	HTTPcontent::append_to_body(char buff[BUFF_SIZE], int bytes) {_current_body.append(buff, (size_t)bytes);}
