# AOS-321 à AOS-328 — Authorization Bearer caller-owned

`net_http_build_post_json_bearer` construit un POST JSON HTTP/1.1 comprenant `Authorization: Bearer <token>`. Le token est une entrée caller-owned consommée uniquement pendant la construction : il n’est ajouté à aucune structure TCP, TLS, NE2000 ou contexte persistant.

Le constructeur applique les mêmes limites que le framing HTTP existant : chemin commençant par `/`, texte ASCII imprimable sans contrôle, capacité de buffer bornée, `Content-Type: application/json`, `Content-Length` décimal et `Connection: close`. Un token vide, un token comportant un caractère interdit ou un buffer trop petit provoque un échec sans longueur publiée.

Le test unitaire vérifie le plaintext exact, le `Content-Length`, l’absence de token vide et le rejet d’une capacité insuffisante. Le token de test est fictif et ne doit jamais être remplacé par une clé réelle dans les sources, documents ou images de boot.

> L’ajout de ce header ne stocke ni ne gère de secrets. La fourniture sécurisée, rotation, révocation et effacement d’une clé API restent de la responsabilité de la couche appelante.

Les formats OpenAI/Ollama, le chiffrement de persistance, l’authentification mutuelle, le proxy, HTTP/2, streaming SSE/chunked et le client HTTPS de production complet restent hors périmètre.
