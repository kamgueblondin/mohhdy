# AOS-176 — État minimal du handshake TLS caller-owned

AOS-176 ajoute `net_tls_handshake_t`, une structure d’état fournie par l’appelant. Elle suit les étapes `IDLE`, `CLIENT_HELLO_SENT` et `SERVER_HELLO_RECEIVED`. Le ServerHello ne peut être accepté qu’après notification explicite de l’émission du ClientHello.

À l’acceptation, la structure conserve la suite cryptographique négociée et une vue caller-owned du random serveur. Un ServerHello répété ou reçu dans un ordre invalide est rejeté. Aucun secret, état global, clé ou buffer interne n’est créé.

Cette étape formalise seulement l’ordre du handshake. Elle ne dérive pas de clés, ne calcule pas de Finished, ne chiffre pas les records et ne valide pas X.509. Le transport TLS vers HTTP et les appels LLM en ligne restent non fonctionnels de bout en bout.

| Élément | Statut |
|---|---|
| États caller-owned du handshake | Implémentés. |
| ClientHello requis avant ServerHello | Validé. |
| Suite et random serveur publiés sans copie | Implémenté. |
| Dérivation de clés et Finished | Non implémentés. |
| Chiffrement et X.509 | Non implémentés. |
| HTTP et appels LLM en ligne | Non fonctionnels de bout en bout. |
