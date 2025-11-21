
PARSING ADRI :
[ ] checker qu'on a pas incorpore des fonctions interdites

?[ ] parsingAutoConfig checker que les directives qu'on gere sont correctement parses (autoindex ne peut valeur que on ou off)
?[ ] checker que les directives sont au bon endroit (qu'il n'y a pas de directives propre a server/location dans global par ex)
[x] checker que les redirections fonctionnent
[x] Gerer les redirections infinies dans le parsing
[x] dans root et dans location_uri checker que ca mene pas vers des chemins interdits
?[ ] faire une liste de toutes les directives qu'on gere et faire une loop dans config pour les verifier (est ce qu'on ignore ou pas)

SERVERMONITOR / TCPCONNECTION (GASTON):

    [x] CGI verifier EOF et gerer le POST -> Inpipe
    [x] autoindex on -> list directories
    [x] debug monitoring cgi, connection_end()...
    [ ] implement TIMEOUT directives 
    [ ] TIMEOUT pour response et CGI
	[ ] s'assurer que le path va jusq'ua '?'

BONUS:

    [ ] gestion des cookies -> mdp + login dans un fichier

TESTS:

    [ ] leaks avec top ou htop
    [ ] siege
    [ ] fichiers lourds
    [ ] CGI boucle infinie (non bloquant pour autres clienbts + )
    [x] Se connecter avec localhost ne fonctionne pas car webserv ne reconnait que 127.0.0.1 (fichier config)

