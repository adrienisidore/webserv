
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
- [x] ajouter directive error_pages si le user souhaite afficher une page specifique pour un code specifique, pouvoir en gerer plusieurs
	Taper "<html><body>File error</body></html>" pour savoir ce qu'on renvoie si le fichier
	specifie dans error_page n'est pas ouvrable. De preference il faudrait checker les permissions avant d'essayer d'ouvrir ?

 [ ] A PARSER directives geree :
 	return : 3xx location_presente dans serverConfig
	autoindex : on ou off
	cgi_handler : .xxx binary
	allow_methods : GET HEAD POST DELETE
	error_page : code_derreur_gere chemin_autorise

  [x] ATTENTION : appliquer is_valid_path(std::string filename) dans loadFile pour eviter
  de pouvoir entrer des chemins interdits

  [x] comprendre pourquoi quand on specifie une page html inexistante "inde.html" au lieu de "index.html" : taper "ICIIIII buildPath 2" dans la barre de recherche

  [ ] c'est peut etre les messages de debug qui font buguer siege (pas le temps de gerer les co et afficher les messages)

- [ ] Tester si on peut gerer plusieurs cgi differents en meme temps (.php et .py par ex) : j'ai modifie is_cgi et buildArgv (+ heritage) : tester directement dans le code car on
peut pas afficher de message depuis le processus enfant (afficher program)

----

- Ameliorer page d'accueil : permettre au user de POST, GET ou DELETE des elements (mettre a jour index.html avec IA)

GASTON:

[x] Gerer cas ou Content-length ne correspond pas a la bonne taille du POST envoye
[x] Gerer .php (error 500)
[x] tester les POST avec les CGI (error 403), notamment avec post_test.py (a demander a Alix)
[x] inpipe[1] is not in poll() for CGIS -> very large body could fail
[x] tester commande "siege -c255 http://localhost:8080/" avec 1 ou plusieurs serveurs dans config (error)
[x] mauvaise erreur (500) si GET fichier .py qui n'existe pas
[x] verifier comportement plusieurs cgis 
[x] refaire les fichiers .html en anglais