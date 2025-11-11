#include "webserv.hpp"
#include "ConfigParser.hpp"
#include "Exceptions.hpp"

// ---------------------------------------UTILS-----------------------------------

std::string toString(size_t num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
}

std::string trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(str[start]))
        start++;
    while (end > start && std::isspace(str[end - 1]))
        end--;
    
    return str.substr(start, end - start);
}

bool isEmptyOrComment(const std::string& line) {
    std::string trimmed = trim(line);
    return trimmed.empty() || trimmed[0] == '#';
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        std::string trimmed = trim(token);
        if (!trimmed.empty())
            tokens.push_back(trimmed);
    }
    
    return tokens;
}


// ---------------------------------------METHODS-----------------------------------


ConfigParser::ConfigParser(const std::string &configFile, const std::string &tempFileName): _lineNumber(0) {
    _input.open(configFile.c_str());
    if (!_input.is_open()) {
        throw ParsingException("Cannot open config file: " + configFile);
    }
    
    _tempFile.open(tempFileName.c_str());
    if (!_tempFile.is_open()) {
        _input.close();
        throw ParsingException("Cannot create temporary file: " + tempFileName);
    }
}

ConfigParser::~ConfigParser() {
    if (_input.is_open())
        _input.close();
    if (_tempFile.is_open())
        _tempFile.close();
}

std::string ConfigParser::readLine() {
    _lineNumber++;
    if (std::getline(_input, _currentLine)) {
        return trim(_currentLine);
    }
    return "";
}

void    ConfigParser::writeTempLine(const std::string& line) {
    _tempFile << line << std::endl;
}

void    ConfigParser::parseDirective(const std::string& line, bool allowedContext) {
    if (!allowedContext)
        throw ParsingException("Directive not allowed in this context at line " + toString(_lineNumber));
    
    // Check if line ends with semicolon
    if (line[line.length() - 1] != ';')
        throw ParsingException("Missing semicolon at line " + toString(_lineNumber));
    
    // Remove semicolon and split
    std::string directive = line.substr(0, line.length() - 1);
    std::vector<std::string> parts = split(directive, ' ');
    
    if (parts.empty())
        throw ParsingException("Empty directive at line " + toString(_lineNumber));
    
    // Write to temp file (name + concatenated values)
    std::string tempLine = parts[0];
    for (size_t i = 1; i < parts.size(); i++) {
        tempLine += " " + parts[i];
    }
    writeTempLine(tempLine + ";");
}

void    ConfigParser::parseLocation(std::string &line) {
    
    // Expected format: location /uri {
    std::vector<std::string> parts = split(line, ' ');
    
    if (parts.size() < 2)
        throw ParsingException("Invalid location directive at line " + toString(_lineNumber));
    
    if (parts[0] != "location")
        throw ParsingException("Expected 'location' at line " + toString(_lineNumber));
    
    std::string uri = parts[1];
    
    // Check for opening brace
    if (parts.size() == 2) {
        line = readLine();
        if (line != "{")
            throw ParsingException("Expected '{' after location at line " + toString(_lineNumber));
    } else if (parts.size() == 3 && parts[2] == "{") {
        // Opening brace on same line
    } else {
        throw ParsingException("Invalid location syntax at line " + toString(_lineNumber));
    }
    
    writeTempLine("location " + uri);
    
    // Parse location body
    while (true) {
        line = readLine();
        
        if (line.empty() && _input.eof())
            throw ParsingException("Unexpected EOF in location block");
    
        if (isEmptyOrComment(line))
            continue;
        
        if (line == "}")
            break;
        
        // Check for nested location (not allowed)
        if (line.find("location") == 0)
            throw ParsingException("Nested locations are not allowed at line " + toString(_lineNumber));
        
        parseDirective(line, true);
    }
}

void    ConfigParser::parseServer() {
    
    std::string line;

    // Expected format: server name {  or just "server {"
    std::vector<std::string> parts = split(_currentLine, ' ');
    
    if (parts.empty() || parts[0] != "server")
        throw ParsingException("Expected 'server' at line " + toString(_lineNumber));
    
    writeTempLine("server");
    
    // Find opening brace
    bool foundBrace = false;
    if (parts.size() > 1 && parts[parts.size() - 1] == "{") {
        foundBrace = true;
    } else {
        line = readLine();
        if (line == "{") {
            foundBrace = true;
        }
    }
    
    if (!foundBrace)
        throw ParsingException("Expected '{' after server at line " + toString(_lineNumber));
    
    bool hasListen = false;
    
    // Parse server body
    while (true) {
        line = readLine();
        
        if (line.empty() && _input.eof()) {
            throw ParsingException("Unexpected EOF in server block");
        }
        
        if (isEmptyOrComment(line))
            continue;
        
        if (line == "}") {
            break;
        }
        
        if (line.find("location") == 0) {
            parseLocation(line);
        } else if (line.find("server") == 0) {
            throw ParsingException("Nested servers are not allowed at line " + toString(_lineNumber));
        } else {
            // Check for mandatory directives
            if (line.find("listen") == 0) {
                hasListen = true;
            }
            parseDirective(line, true);
        }
    }
    
    // Validate mandatory directives
    if (!hasListen) {
        throw ParsingException("Missing mandatory 'listen' directive in server block");
    }
}


void    ConfigParser::parseGlobal() {

    while (true) {
            std::string line = readLine();
            
            if (line.empty() && _input.eof()) { // VERIFIER QUE EMPTY MARCHe
                break;
            }
            
            if (isEmptyOrComment(line))
                continue;
            
            if (line.find("server") != std::string::npos) {
                _currentLine = line;
                parseServer();
            } else {
                // Global directive
                parseDirective(line, true);
            }
    }
}

std::string generate_filename() {

    std::string default_filename = "./webserv_config.tmp";
    int i = 0;
    if (access((default_filename).c_str(), O_RDONLY) != 0)
        return default_filename;
    while (access((default_filename + ".tmp").c_str(), O_RDONLY) == 0) {
        default_filename = "./webserv_config(" + toString(++i) + ").tmp";
    }
    return default_filename;
}

std::string parseConfig(const std::string& configFile) {
    
    // Attention: que se passe-t-il si le fichier existe deja 
    std::string tempFile = generate_filename();
    
    // First pass: Syntactic validation
    ConfigParser parser(configFile, tempFile);
    parser.parseGlobal();
    
    // Second pass: Semantic validation and object creation
    return tempFile;
}
