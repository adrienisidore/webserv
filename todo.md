
[x] gerer toutes les erreurs compilation

[ ] finir fonction hub()
    [ ] gerer les exceptions (delete cgi sockets, ...)

[ ] TIMEOUT pour response et CGI

[ ] put status to END after response is sent

[ ] gerer keep-alive

[ ] send-response

[ ] faire une liste de toutes les directives qu'on gere et faire une loop dans config pour les verifier

[ ] intergrer toutes les directives dans le code (a la place des values par defaut)

[ ] gestion des cookies -> mdp + login dans un fichier

[ ] tests:
    [ ] leaks avec top ou htop
    [ ] siege
    [ ] fichiers lourds
    [ ] CGI boucle infinie (non bloquant pour autres clienbts + )
