# AOS-1365 à AOS-1372 — Renouvellement DHCP live caller-owned

## Objectif

Ce macro-lot transforme l’état de bail DHCP déjà présent en mécanisme live borné. Lorsqu’un bail atteint son seuil de renouvellement, le pilote peut envoyer un DHCP REQUEST de renouvellement, attendre un ACK avec un nombre d’itérations borné et publier le nouveau bail seulement si toutes les validations réussissent.

## Contrat livré

| Primitive | Résultat |
|---|---|
| `net_dhcp_build_renew` | Produit un DHCP REQUEST avec l’adresse courante dans `ciaddr`, le XID fourni et une liste de paramètres masque/routeur/DNS. |
| `ne2k_dhcp_renew` | Émet le paquet de renouvellement depuis l’adresse courante vers le broadcast DHCP, sans allocation dynamique. |
| `ne2k_dhcp_renew_if_due` | Retourne `0` avant échéance, `1` après ACK validé, et une erreur négative sans modifier le bail à l’échec. |

L’échéance réutilise `net_dhcp_lease_renewal_due`, définie à la moitié de la durée du bail. Le chemin impose aussi que le bail soit toujours valide au moment de l’appel. Après ACK, `net_dhcp_lease_mark_acquired` actualise `acquired_tick`, ce qui redémarre la fenêtre de validité et de renouvellement.

> Le bail est copié dans un état local avant émission. Une erreur TX, un ACK absent ou invalide, ou une incohérence de durée laisse le bail caller-owned inchangé.

## Validation

Le test DHCP vérifie la longueur du packet, `ciaddr`, le type REQUEST, la liste de paramètres et le rejet d’un buffer trop petit. Le test NE2000 vérifie l’absence d’émission avant échéance, la conservation transactionnelle du bail en cas d’échec de transport et le rejet d’un bail expiré. La compilation i386 réussit, la suite noyau passe **36/36 tests** et la suite complète passe **432/432 tests**.

## Limites restantes

Le renouvellement envoyé ici est un REQUEST broadcast avec `ciaddr`, ce qui permet une récupération bornée sans ajouter de cache ARP ni tâche de fond. L’orchestrateur de réseau doit encore programmer périodiquement `ne2k_dhcp_renew_if_due` à partir de l’horloge et définir une stratégie après expiration, par exemple une réacquisition DHCP complète. Cette planification reste séparée afin de ne pas créer de timer implicite, de thread noyau ou d’allocation dynamique.

## Références

[1]: aos1353_1364_llm_socket_ne2k_bridge.md "Adaptateur LLM/socket et pont NE2000"
[2]: todo.md "Backlog MOHHDY"

[1] [2]
