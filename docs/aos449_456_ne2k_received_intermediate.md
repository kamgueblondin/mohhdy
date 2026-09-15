# AOS-449 à AOS-456 — validation NE2000 de l’intermédiaire reçu

`ne2k_tls_client_poll_received_chain` relie le parsing de la liste TLS `Certificate` à la politique X.509 à trois certificats. L’appelant fournit uniquement l’ancre de confiance, le hostname, l’instant UTC et les workspaces habituels ; l’orchestrateur emploie le premier intermédiaire reçu et parsé dans `client->handshake`.

| Condition | Résultat |
|---|---|
| Un premier intermédiaire est reçu et son X.509 est parsé | La politique `leaf → intermédiaire → ancre` est appliquée avant le flight X25519. |
| Aucun intermédiaire n’est reçu | La façade automatique refuse la politique à chaîne au moment de la validation d’identité. |
| Le DER intermédiaire échoue au parsing | Le handshake ne marque pas la feuille comme valide et le polling effectue son rollback transactionnel. |
| Client nul | La façade rejette immédiatement l’appel. |

La nouvelle façade délègue au polling à chaîne existant. Elle ne copie aucun DER et n’ajoute aucun état caché : les références du handshake, les buffers TCP/TLS, les workspaces RSA/X25519 et les limites de retransmission restent caller-owned.

> Cette implémentation ne choisit que le premier intermédiaire de la liste TLS. Elle ne résout pas les chaînes multiples, les cross-signatures, les listes de profondeur arbitraire, la révocation ou la découverte d’ancre.
