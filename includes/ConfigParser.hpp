#ifndef CONFIG_PARSER
# define CONFIG_PARSER

#include "webserv.hpp"

class ConfigParser {

private:
    std::ifstream   _input;
    std::ofstream   _tempFile;
    std::string     _currentLine;
    size_t          _lineNumber;

    std::string readLine();
    void        writeTempLine(const std::string &line);
    void        parseDirective(const std::string &line, bool allowedContext);
    void        parseLocation(std::string &line);
    void        parseServer();

public:

    ConfigParser(const std::string &configFile, const std::string &tempFileName);
    ~ConfigParser();
    
    void        parseGlobal();

};

#endif