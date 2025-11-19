[ ] checker qu'on a pas incorpore des fonctions interdites
PARSING (GASTON):

    [ ] faire une liste de toutes les directives qu'on gere et faire une loop dans config pour les verifier (est ce qu'on ignore ou pas)

RESPONSE (ADRI):

	[x] finir buildEnv et finir de launchexecve (pour le cas de POST)

	[ ] redirection directive -> vers une nouvelle location. ATTENTION AUX REDIRECTIONS INFINIES
	se baser sur "return" pour gerer ca, et savoir ou on doit checker les redirections dans le code

    [x] checkAllowedMethods()
	[ ] si la location correspond a la fois a CGI et redi, on fait quoi ?

SERVERMONITOR / TCPCONNECTION (GASTON):

    [x] CGI verifier EOF et gerer le POST -> Inpipe
    [x] autoindex on -> list directories
    [ ] s'assurer que le path va jusq'ua '?'
    [ ] debug monitoring cgi, connection_end()...
    [ ] implement TIMEOUT directives 
    [ ] TIMEOUT pour response et CGI






BONUS:

    [ ] gestion des cookies -> mdp + login dans un fichier

TESTS:

    [ ] leaks avec top ou htop
    [ ] siege
    [ ] fichiers lourds
    [ ] CGI boucle infinie (non bloquant pour autres clienbts + )
    [x] Se connecter avec localhost ne fonctionne pas car webserv ne reconnait que 127.0.0.1 (fichier config)

