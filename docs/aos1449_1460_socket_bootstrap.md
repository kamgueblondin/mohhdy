# AOS-1449 à AOS-1460 — Bootstrap DHCP/DNS/ARP/SYN vers session LLM socket

## Objectif

Ce macro-lot raccorde la phase amont du flux LLM à la session socket introduite précédemment. Les appels publics déclenchent désormais le bootstrap DNS/ARP/SYN tout en préparant un slot du registre socket statique, puis attachent ce slot à `ne2k_llm_socket_session_t` uniquement si l’intégralité du bootstrap aboutit.

> Une session LLM socket ne publie jamais un identifiant de slot partiellement initialisé : en cas d’erreur DHCP, DNS, ARP, SYN ou attachement, le slot créé est fermé et la session reste `IDLE` avec `socket_id == -1`.

## API livrée

| Primitive | Entrées réseau | Publication en cas de succès |
|---|---|---|
| `ne2k_llm_socket_session_start` | DNS, ARP et SYN depuis IPv4/DNS explicites. | Slot `SYN_SENT`, IPv4 résolue et session attachée. |
| `ne2k_llm_socket_session_start_dhcp` | DNS routé et SYN via le prochain saut du bail DHCP. | Même état socket, sans exposer de connexion TCP privée. |
| `ne2k_llm_socket_session_acquire_start_dhcp` | Acquisition DHCP puis bootstrap routé. | Bail et session publiés ensemble seulement après succès total. |

## Transaction et compatibilité

Le slot socket est ouvert avant le bootstrap afin que le registre possède les ports et la séquence qui correspondent au SYN envoyé. Les codecs DNS/ARP/SYN historiques sont exécutés dans une connexion de travail locale, non publiée. Après résolution et transmission réussies, l’attachement vérifie que le slot est bien en `SYN_SENT`, recopie l’IPv4 résolue et passe la session à cette même phase.

Un échec sur un chemin inférieur ferme immédiatement le slot. Ni le bail appelant, ni la session, ni ses quatre octets d’IPv4 ne sont publiés. Les anciennes façades à `net_tcp_connection_t` restent intactes : la nouvelle API est une voie socket parallèle et compatible qui retire progressivement le transport privé du chemin LLM.

## Validation

Le test Unity introduit force un échec DNS sans réponse et une variante DHCP avec bail invalide. Il confirme alors que la session demeure `IDLE`, que son identifiant reste `-1`, et que les quatre slots du registre peuvent toujours être ouverts puis fermés. Cette dernière vérification contrôle directement l’absence de fuite de capacité dans le rollback.

| Contrôle | Résultat |
|---|---|
| Compilation i386 `make all` | Réussie |
| Suite complète `make test-all` | 443/443 tests verts |
| `git diff --check` | Propre |
| Allocation dynamique dans les chemins modifiés | Aucune occurrence |

## Limites restantes

La session socket peut maintenant être préparée de DHCP/DNS/ARP/SYN à HTTP/SSE, mais le noyau conserve encore l’ancien contexte de connexion pour son orchestration boot actuelle. Le raccordement de `kernel.c` au nouveau contexte socket, la planification périodique du renouvellement DHCP, la reprise après expiration de bail, les délais/backoff et le client OpenAI effectif restent à livrer.

## Références

[1]: aos1437_1448_llm_socket_session.md "Session LLM unifiée sur API socket"
[2]: aos1425_1436_socket_llm_orchestrator.md "Orchestrateur LLM HTTP/SSE actif sur socket"
[3]: aos1365_1372_dhcp_live_renewal.md "Renouvellement DHCP live caller-owned"

[1] [2] [3]
