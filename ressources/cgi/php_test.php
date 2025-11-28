<?php
header('Content-Type: text/plain');

echo "=== VARIABLES D'ENVIRONNEMENT ===\n";
foreach ($_SERVER as $key => $value) {
    echo "$key = $value\n";
}

#curl -i -X POST -d "param=value" http://localhost:8080/cgi-bin/php_test.php
echo "\n=== CONTENU DU BODY (si POST) ===\n";
$input = file_get_contents('php://input');
echo $input ? $input : "(vide)\n";
?>
