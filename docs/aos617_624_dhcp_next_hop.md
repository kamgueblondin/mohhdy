# AOS-617 à AOS-624 — Masque DHCP et prochain saut réseau

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** DHCPv4, masque de sous-réseau, routage IPv4 minimal, préparation ARP/DNS/LLM

## Objectif

Ce macro-lot complète le bail DHCP avec le masque de sous-réseau et fournit une sélection explicite du prochain saut IPv4. Cette étape est nécessaire avant de raccorder le bootstrap LLM à un DNS ou un fournisseur situé hors du réseau local : ARP doit résoudre la passerelle dans ce cas, et non l’adresse distante elle-même.

> Si une destination est dans le même sous-réseau, le prochain saut est la destination. Sinon, le prochain saut est le routeur DHCP. Aucun prochain saut n’est publié si le bail ou son masque sont invalides.

## Bail DHCP enrichi

| Champ | Source DHCP | Usage |
|---|---:|---|
| `subnet_valid`, `subnet_mask` | Option `1` | Déterminer l’appartenance au sous-réseau. |
| `router_valid`, `router_ipv4` | Option `3` | Passerelle d’un DNS ou hôte distant. |
| `dns_valid`, `dns_ipv4` | Option `6` | Résolution de nom fournisseur LLM. |

L’option de masque doit avoir exactement quatre octets. Comme les options routeur et DNS, une option de masque malformée provoque le rejet du paquet avant publication du bail local.

## API de routage

```c
int net_dhcp_lease_next_hop(const net_dhcp_lease_t* lease,
                            const uint8_t destination[4],
                            uint8_t next_hop[4]);
```

L’API exige un bail valide et un masque valide. Elle compare `lease->ipv4 & lease->subnet_mask` avec `destination & lease->subnet_mask` sur les quatre octets. Une destination locale est recopiée vers `next_hop`; une destination hors sous-réseau requiert `router_valid` et retourne `router_ipv4`. La sortie est préparée dans un buffer local : `next_hop` est inchangé lors de toute erreur.

| Cas | Résultat |
|---|---|
| Bail/mask invalide ou argument nul | `-1`, sortie inchangée. |
| Destination distante sans routeur | `-2`, sortie inchangée. |
| Destination locale | `0`, prochain saut = destination. |
| Destination distante avec routeur | `0`, prochain saut = routeur DHCP. |

## Tests et validation locale

Le vecteur ACK DHCP inclut un masque `/24`, un routeur `10.0.2.2` et deux DNS. Il vérifie d’abord une destination locale `10.0.2.99`, puis une destination distante `1.1.1.1` qui sélectionne le routeur. Le test invalide ensuite le masque pour confirmer que la sortie sentinelle reste inchangée. Les tests transactionnels NE2000 sont adaptés au bail étendu.

| Vérification | Résultat |
|---|---|
| Test DHCP ciblé | Réussi. |
| Test NE2000 ciblé | **16/16** réussis. |
| Suite complète | **379/379** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi. |
| Smoke QEMU NE2000 | Réussi. |

## Limites connues

Le prochain saut n’est pas encore branché au transport ARP/TCP du bootstrap LLM : `ne2k_dns_query`, la résolution de l’hôte LLM et l’émission SYN doivent encore recevoir et employer ce prochain saut. La configuration live d’interface, le service LLM noyau, les syscalls d’émission/polling, les identifiants hors image, TLS live, timeout/retry, fermeture de session, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-609 à AOS-616 — routeur et DNS dans le bail DHCP](aos609_616_dhcp_route_dns.md)  
[2] [AOS-601 à AOS-608 — acquisition DHCP transactionnelle NE2000](aos601_608_dhcp_transactional_acquire.md)  
[3] [AOS-537 à AOS-544 — orchestrateur LLM DNS vers ClientHello](aos537_544_llm_connection_orchestrator.md)
