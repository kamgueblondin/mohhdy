# AOS-1025 à AOS-1032 — ingestion SSE transactionnelle

`net_llm_sse_response_feed_transactional` enveloppe l’ingestion HTTP chunked puis SSE existante. Il sauvegarde les métadonnées caller-owned de `net_llm_sse_response_t` et la valeur initiale de `text_length`, délègue au décodeur déjà validé, puis restaure ces métadonnées lorsque le feed retourne une erreur.

Les pointeurs et les capacités des buffers restent inchangés. Aucun buffer dynamique n’est créé et aucun secret ou payload n’est conservé par le wrapper. Les octets éventuellement écrits dans un buffer caller-owned sont considérés comme des données de travail ; la restauration de la longueur rend le prochain feed déterministe et permet au caller de réutiliser la zone.

La façade ne transforme pas les retours positifs : `1` signifie qu’aucun événement complet n’est encore disponible, `0` signale une progression ou une fin SSE, et les codes négatifs restent ceux du chemin existant. Elle protège ainsi l’état HTTP/SSE contre une erreur de framing, de fournisseur, de capacité ou de statut HTTP.

> Une erreur de décodage ne publie pas de nouvel état applicatif partiel.

| Élément | Garantie |
|---|---|
| État HTTP chunked | Restauré sur erreur |
| État SSE et `Last-Event-ID` | Restauré sur erreur |
| `decoded_consumed` | Restauré sur erreur |
| `text_length` | Restauré sur erreur |
| Buffers et secrets | Caller-owned, aucune copie dynamique |
| Blocage / allocation | Aucun |

Validation locale : **413/413 tests verts**, `git diff --check` propre.

Auteur : **Manus AI**

