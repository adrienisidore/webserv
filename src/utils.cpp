# include "webserv.hpp"


// Pour alleger response et incorporer findCgiProgramForPath dans Response.cpp et CGI.cpp

// Une fonction utilitaire qui, à partir de la directive cgi_handler et du chemin _path, renvoie le premier binaire compatible trouvé.
bool findCgiProgramForPath(const std::string &handlers, const std::string &path, std::string &outProgram)
{
    outProgram.clear();

    if (handlers.empty())
        return false;

    // extension du fichier demandé
    std::string::size_type dotPos = path.rfind('.');
    if (dotPos == std::string::npos)
        return false;
    std::string pathExt = path.substr(dotPos); // ex ".py"

    std::size_t start = 0;
    while (start < handlers.size()) {
        // extraire une ligne (séparée par '\n')
        std::size_t end = handlers.find('\n', start);
        if (end == std::string::npos)
            end = handlers.size();

        std::string line = handlers.substr(start, end - start);
        start = end + 1;

        // trim début
        std::size_t i = 0;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        if (i == line.size())
            continue;
        line = line.substr(i);

        // ext = jusqu'au 1er espace ou tab
        std::size_t sp = line.find_first_of(" \t");
        if (sp == std::string::npos)
            continue;

        std::string ext = line.substr(0, sp);

        // début du binaire (skip espaces)
        std::size_t j = line.find_first_not_of(" \t", sp);
        if (j == std::string::npos)
            continue;
        std::string prog = line.substr(j);

        // validation de l'extension
        bool ok =
            ext.size() >= 2 &&
            ext[0] == '.' &&
            ext.find('.', 1) == std::string::npos;

        if (!ok)
            continue;

        // PREMIÈRE extension compatible → on s'arrête
        if (ext == pathExt) {
            outProgram = prog;
            return true;
        }
    }

    return false;
}

//Recherche si error_page contient _code
bool findErrorPageForCode(const std::string &value,
                          int code,
                          std::string &outPath)
{
    outPath.clear();
    if (value.empty())
        return false;

    std::size_t start = 0;
    while (start < value.size()) {
        std::size_t end = value.find('\n', start);
        if (end == std::string::npos)
            end = value.size();

        std::string line = value.substr(start, end - start);
        start = end + 1;

        // trim début
        std::size_t i = 0;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        if (i == line.size())
            continue;
        line = line.substr(i);

        // parser "CODE URI"
        std::istringstream iss(line);
        int cfgCode;
        std::string uri;
        iss >> cfgCode >> uri;
        if (!iss || uri.empty())
            continue;

        if (cfgCode == code) {
            outPath = uri;
            return true;
        }
    }
    return false;
}
