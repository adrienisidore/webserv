
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

---
- [ ] ajouter directive error_pages si le user souhaite afficher une page specifique pour un code specifique, pouvoir en gerer plusieurs
	: il faut d'abord checker si une page particuliere est souhaite pour une erreur, si oui (verifier qu'on a la permission de la lire) on l'affiche sinon on affiche la page par defaut
- [x] Pouvoir gerer plusieurs cgi differentsen meme temps (.php et .py par ex) : j'ai modifie is_cgi et buildArgv (+ heritage) : A TESTER
----

- Ameliorer page d'accueil : permettre au user de POST, GET ou DELETE des elements (mettre a jour index.html avec IA)

GASTON:

[x] Gerer cas ou Content-length ne correspond pas a la bonne taille du POST envoye
[x] Gerer .php (error 500)
[x] tester les POST avec les CGI (error 403), notamment avec post_test.py (a demander a Alix)
[x] inpipe[1] is not in poll() for CGIS -> very large body could fail
[x] tester commande "siege -c255 http://localhost:8080/" avec 1 ou plusieurs serveurs dans config (error)
[ ] pourquoi buildServerNameHost a une valeur de 'port' absolue ?
[ ] mauvaise erreur (500) si GET fichier .py qui n'existe pas

