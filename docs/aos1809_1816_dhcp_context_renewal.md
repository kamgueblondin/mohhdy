# AOS-1809…1816 — Renouvellement DHCP dans le contexte réseau LLM

## Objet

Ce macro-lot relie le renouvellement DHCP existant au contexte réseau LLM caller-owned grâce à `ne2k_llm_network_context_dhcp_renew_if_due()`. La façade utilise le bail mémorisé dans `context.lease`, exécute le renouvellement sur une copie locale, puis publie le bail renouvelé seulement lorsque toute la transaction DHCP est terminée.

| Situation | Résultat |
|---|---|
| Bail valide, renouvellement non dû | Retour `0`, aucune mutation |
| Bail valide, renouvellement dû et ACK reçu | Retour `1`, nouveau bail publié |
| Émission ou ACK en échec | Erreur, bail conservé |
| Bail expiré ou invalide | Erreur, bail conservé |

## Contrat

La façade ne conserve aucun buffer ni état d’E/S. Les trames TX/RX restent caller-owned. Elle copie d’abord le bail du contexte, appelle `ne2k_dhcp_renew_if_due()` sur cette copie, puis ne remplace `context.lease` qu’en cas de succès complet.

> Le contexte réseau LLM peut ainsi renouveler son bail DHCP avant les connexions DNS/TCP/TLS et les reprises SSE sans exposer de demi-bail aux couches applicatives.

## Tests

Le test de contexte couvre l’attente avant l’échéance de renouvellement, un échec d’émission à échéance, l’expiration du bail et un pointeur de contexte nul. Chaque erreur vérifie l’égalité mémoire du bail conservé.

| Vérification | Résultat |
|---|---|
| Avant échéance | Sans mutation |
| Échec de renouvellement | Bail conservé |
| Bail expiré | Rejet sans mutation |
| Contexte nul | Rejet |
| Test NE2000 ciblé | **41/41 réussis** |

## Références

[1] [AOS-1481 à AOS-1488 — réacquisition DHCP](aos1481_1488_dhcp_reacquisition.md)  
[2] [AOS-609 à AOS-616 — bail DHCP, route et DNS](aos609_616_dhcp_route_dns.md)  
[3] [RFC 2131 — Dynamic Host Configuration Protocol](https://www.rfc-editor.org/rfc/rfc2131)
