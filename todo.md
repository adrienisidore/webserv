
TESTS:

    [ ] Checker les leaks en toute circonstances
    [ ] checker qu'on a pas incorpore des fonctions interdites (A la fin du projet)

    ?[ ] checker que les directives sont au bon endroit (qu'il n'y a pas de directives propre a server/location dans global par ex)
    [ ] parsingAutoConfig : faire une liste de toutes les directives qu'on gere et faire une loop dans config pour les verifier : rtout ce qu'on gere on accepte n'importe pas quelle valeur

    [ ] Rendre la page d'accueil connecte au reste

    [ ] tests avec differentes fichiers de config
    [ ] verifier leaks avec top ou htop
    [X] siege
    [ ] fichiers lourds
    [X] CGI boucle infinie (non bloquant pour autres clienbts + )


Feuille de correcion : https://wormav.github.io/42_eval/Cursus/Webserv/index.html


CORRECTION:

- Savoir expliquer difference entre epoll() poll() select() ...


ADRI:

- Attention affichage character non ACII (tout ecrire en anglais)
- ajouter directive error_pages si le user souhaite afficher une page specifique pour un code specifique
- multiple CGI a faire
- Pouvoir gerer plusieurs cgi differents en meme temps (.php et .py par ex)
- Ameliorer page d'accueil : permettre au user de POST, GET ou DELETE des elements

GASTON:

[x] Gerer cas ou Content-length ne correspond pas a la bonne taille du POST envoye
[x] Gerer .php (error 500)
- tester les POST avec les CGI (error 403), notamment avec post_test.py (a demander a Alix)
    -> pour l'instant CGI s'execute mais ne reconnait pas le body 
- tester commande "siege -c255 http://localhost:8080/" avec 1 ou plusieurs serveurs dans config (error) 
