
PARSING (GASTON):

    [ ] faire une liste de toutes les directives qu'on gere et faire une loop dans config pour les verifier (est ce qu'on ignore ou pas)

RESPONSE (ADRI):

    [x] mettre les headers appropries dans toutes les methodes response et error
    [ ] redirection directive -> vers une noubvelle location
    [x] send response

SERVERMONITOR / TCPCONNECTION (GASTON):

    [x] bien verifier le status ERROR
    [x] gerer directive keep-alive
    [ ] CGI verifier EOF et gerer le POST 

    [ ] TIMEOUT pour response et CGI
    [ ] Segfault avec ctrlC (connection close)






BONUS:

    [ ] gestion des cookies -> mdp + login dans un fichier

TESTS:

    [ ] leaks avec top ou htop
    [ ] siege
    [ ] fichiers lourds
    [ ] CGI boucle infinie (non bloquant pour autres clienbts + )
