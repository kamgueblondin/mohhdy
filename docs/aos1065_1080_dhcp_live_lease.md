# AOS-1065 à AOS-1080 — bail DHCP live borné

Le client DHCP extrait désormais l’option 51 (`IP address lease time`) lors de la validation d’un ACK. La durée est conservée dans `net_dhcp_lease_t` avec le tick d’acquisition caller-owned.

`net_dhcp_lease_mark_acquired` marque un bail valide sans consulter d’horloge globale. `net_dhcp_lease_is_valid_at` vérifie l’expiration par soustraction non signée, ce qui conserve le comportement attendu lors du wraparound d’un compteur 32 bits. `net_dhcp_lease_renewal_due` signale le seuil de renouvellement à mi-bail, sans bloquer ni émettre directement un paquet.

Le parseur reste transactionnel : il construit un bail temporaire et ne publie l’état que si le message DHCP est un ACK cohérent. Une option 51 mal formée est rejetée avec restauration du bail précédent. Les champs existants de routeur, DNS, masque et next-hop restent inchangés.

> Le bail DHCP fournit une politique d’échéance ; la boucle réseau décide quand lancer le renouvellement.

| Élément | Garantie |
|---|---|
| Option DHCP | 51, durée en secondes |
| Horloge | Tick fourni par l’appelant |
| Expiration | Contrôle non bloquant, wraparound sûr |
| Renouvellement | Signalé à 50 % du bail |
| Erreur de parsing | Bail précédent préservé |
| Mémoire | Aucun buffer dynamique |

Validation locale : **414/414 tests verts**, incluant la durée, le seuil de renouvellement et l’expiration.

Auteur : **Manus AI**

