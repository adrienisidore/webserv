
PARSING (GASTON):

    [ ] faire une liste de toutes les directives qu'on gere et faire une loop dans config pour les verifier (est ce qu'on ignore ou pas)

RESPONSE (ADRI):

	[ ] finir buildEnv et finir de launchexecve (pour le cas de POST)
	[ ] redirection directive -> vers une nouvelle location. ATTENTION AUX REDIRECTIONS INFINIES
    [ ] checkAllowedMethods()
    
    [ ] autoindex on -> list directories

SERVERMONITOR / TCPCONNECTION (GASTON):

    [x] fermer la connection quand ferme browser -> firefox forces keep-alive on tabs
    [ ] nettoyer tout le caca que j'ai fait 
    [ ] CGI verifier EOF et gerer le POST 
    [ ] TIMEOUT pour response et CGI






BONUS:

    [ ] gestion des cookies -> mdp + login dans un fichier

TESTS:

    [ ] leaks avec top ou htop
    [ ] siege
    [ ] fichiers lourds
    [ ] CGI boucle infinie (non bloquant pour autres clienbts + )
    [x] Se connecter avec localhost ne fonctionne pas car webserv ne reconnait que 127.0.0.1 (fichier config)

