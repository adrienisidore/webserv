
PARSING ADRI :

Probleme de leak au niveau de connected_socket_end (supprimer cout)

[ ] peut on avoir un global sans directivre "root" ? Il y a un "root" par defaut ?
[ ] Checker les leaks en toute circonstances
[ ] checker qu'on a pas incorpore des fonctions interdites (A la fin du projet)
[ ] tester HEAD (PUT n'est pus supporte par le serveur)

?[ ] checker que les directives sont au bon endroit (qu'il n'y a pas de directives propre a server/location dans global par ex)
[ ] parsingAutoConfig : faire une liste de toutes les directives qu'on gere et faire une loop dans config pour les verifier : rtout ce qu'on gere on accepte n'importe pas quelle valeur


    
TESTS:

    [ ] tester les differentes methodes read()
        [x] chunked
        [ ] specific content-length

    [ ] tester POST sans CGI   
    [ ] tester DELETE
        => marche mais verifier que la reponse envoyee est toujours bonne

    [ ] leaks avec top ou htop
    [ ] siege
    [ ] fichiers lourds
    [ ] CGI boucle infinie (non bloquant pour autres clienbts + )
    [ ] test fichier de l'ecole

BONUS:

    ?[ ] gestion des cookies -> mdp + login dans un fichier


