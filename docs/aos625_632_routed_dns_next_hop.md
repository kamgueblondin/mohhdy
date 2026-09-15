# AOS-625 à AOS-632 — UDP et DNS routés par prochain saut

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** NE2000, ARP, IPv4/UDP, DNS, préparation du bootstrap LLM hors sous-réseau

## Objectif

Ce macro-lot raccorde la sélection de prochain saut DHCP au transport Ethernet. Lorsqu’un DNS est distant, la trame Ethernet doit être adressée à la passerelle résolue par ARP, tandis que le paquet IPv4/UDP doit conserver l’adresse du serveur DNS comme destination logique.

> L’adresse résolue par ARP et l’adresse de destination IPv4 ne sont désormais plus nécessairement identiques.

## API ajoutées

| API | Adresse résolue par ARP | Adresse dans l’en-tête IPv4 |
|---|---|---|
| `ne2k_tx_udp_resolve` | `target_ipv4` | `target_ipv4` |
| `ne2k_tx_udp_via` | `next_hop_ipv4` | `destination_ipv4` |
| `ne2k_dns_query_via` | `next_hop_ip` | `dns_ip` |

`ne2k_tx_udp_via` réemploie `ne2k_arp_resolve` et le cache ARP caller-owned pour le prochain saut. Après résolution, elle construit la trame UDP avec la MAC associée au prochain saut et la destination IPv4 réellement visée. `ne2k_dns_query` conserve son comportement historique en déléguant à la variante routée avec `dns_ip` utilisé pour les deux rôles.

## Garanties de mémoire et de transaction

Les buffers de requête ARP, RX, TX et payload restent fournis par l’appelant. Aucune allocation dynamique, copie persistante de pointeur ou état caché de route n’est ajouté. Un échec ARP, de cache ou de construction UDP est retourné avant publication de trame valide ; les données de session LLM ne sont pas modifiées par cette primitive de transport.

## Test principal

Le nouveau test NE2000 prépare un cache ARP avec la passerelle `10.0.2.2 → 52:54:00:00:00:02`, puis émet un UDP vers le DNS distant `1.1.1.1` via cette passerelle. Il vérifie simultanément :

| Élément vérifié | Valeur attendue |
|---|---|
| MAC Ethernet destination | `52:54:00:00:00:02` (passerelle) |
| IPv4 destination | `1.1.1.1` (DNS distant) |
| Résultat de l’émission | succès, sans allocation dynamique |

## Validation locale

| Vérification | Résultat |
|---|---|
| Test NE2000 ciblé | **17/17** réussis. |
| Suite complète | **380/380** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi. |
| Smoke QEMU NE2000 | Réussi. |

## Limites connues

Le prochain saut est maintenant disponible pour DNS/UDP, mais le bootstrap LLM complet doit encore sélectionner puis transmettre le prochain saut pour le DNS et la connexion TCP de l’hôte résolu. Le routage TCP par passerelle, le service LLM noyau, les syscalls d’émission/polling, les identifiants hors image, TLS live, timeout/retry, fermeture de session, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-617 à AOS-624 — masque DHCP et prochain saut réseau](aos617_624_dhcp_next_hop.md)  
[2] [AOS-609 à AOS-616 — routeur et DNS dans le bail DHCP](aos609_616_dhcp_route_dns.md)  
[3] [AOS-537 à AOS-544 — orchestrateur LLM DNS vers ClientHello](aos537_544_llm_connection_orchestrator.md)
