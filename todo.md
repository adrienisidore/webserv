
PARSING (GASTON):

    [ ] faire une liste de toutes les directives qu'on gere et faire une loop dans config pour les verifier

RESPONSE (ADRI):

	[x] coder _error_ : 400, 403, 404, 405, 409, 411, 413, 414, 500, 501
	[ ] un CGI peut etre enclenche depuis n'importe quel type de requete ?
	[ ] redirection directive -> vers une nouvelle location. ATTENTION AUX REDIRECTIONS INFINIES
	[ ] send response (dans TCPConnection)

SERVERMONITOR / TCPCONNECTION (GASTON):

    [ ] bien verifier le status ERROR
    [ ] gerer directive keep-alive
    [ ] TIMEOUT pour response et CGI
    [ ] Segfault avec ctrlC (connection close)






BONUS:

    [ ] gestion des cookies -> mdp + login dans un fichier

TESTS:

    [ ] leaks avec top ou htop
    [ ] siege
    [ ] fichiers lourds
    [ ] CGI boucle infinie (non bloquant pour autres clienbts + )
