# AOS-175 — Parsing ServerHello TLS caller-owned

AOS-175 ajoute `net_tls_server_hello_parse`, une vue sans copie du ServerHello TLS minimal. Le parseur vérifie le type Handshake, la longueur 24 bits, la version TLS 1.2, le random de 32 octets, le session ID borné, la suite cryptographique et la compression nulle.

Les pointeurs retournés désignent le buffer fourni par l’appelant. Aucun état global, buffer interne ou mécanisme d’allocation n’est utilisé. Les messages comportant des extensions ou une structure différente du profil minimal sont volontairement rejetés à ce stade afin de ne pas publier une vue partiellement interprétée.

| Élément | Statut |
|---|---|
| Vue ServerHello sans copie | Implémentée. |
| Contrôles de longueur et version | Implémentés. |
| Session ID, cipher suite et compression | Validés. |
| Extensions TLS et négociation complète | Non implémentées. |
| Dérivation de clés, chiffrement et X.509 | Non implémentés. |
| HTTP et appels LLM en ligne | Non fonctionnels de bout en bout. |
