# AOS-433 à AOS-440 — chaîne X.509 à intermédiaire dans NE2000/TLS

Le polling TLS NE2000 expose maintenant `ne2k_tls_client_poll_chain_two`. Cette interface accepte un certificat intermédiaire et une ancre caller-owned, tout en conservant tous les buffers, workspaces RSA/X25519, limites de retransmission et garanties transactionnelles du polling direct.

La logique de réception TLS est factorisée dans une implémentation interne. Lorsque le certificat serveur est disponible, elle applique soit la politique directe historique, soit `x509_certificate_tls_identity_validate_two` pour la chaîne `leaf → intermédiaire → ancre`. Un échec restaure le client TLS, la connexion TCP, le compteur de consommation et la longueur du flight comme l’API historique.

| API | Chaîne validée | Usage |
|---|---|---|
| `ne2k_tls_client_poll` | feuille → ancre | Compatibilité avec les intégrations existantes. |
| `ne2k_tls_client_poll_chain_two` | feuille → intermédiaire → ancre | Serveur dont la feuille est signée par l’intermédiaire caller-owned. |

Les tests NE2000 confirment le comportement de polling vide de la nouvelle façade et le rejet immédiat d’un intermédiaire nul. Les vérifications cryptographiques de la chaîne et des contraintes CA sont couvertes par les vecteurs X.509 dédiés.

> Le handshake TLS actuel conserve une seule vue du certificat feuille reçu dans le message `Certificate`. Il ne parse ni ne sélectionne automatiquement une liste TLS d’intermédiaires. L’appelant doit donc fournir l’intermédiaire de confiance correspondant ; une collecte et une sélection automatique de la chaîne restent à implémenter.
