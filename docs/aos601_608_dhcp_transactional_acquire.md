# AOS-601 à AOS-608 — Acquisition DHCP transactionnelle NE2000

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** NE2000, DHCPv4, bail caller-owned, prérequis bootstrap LLM

## Objectif

Ce macro-lot transforme les primitives DHCP déjà présentes en une acquisition complète et bornée : `DISCOVER → OFFER → REQUEST → ACK`. L’API reste entièrement caller-owned et ne publie un bail IPv4 qu’après validation de l’ACK final.

> Un échec de transmission, de polling, de filtrage UDP, de XID ou de parsing ne modifie jamais le bail fourni par l’appelant.

## API ajoutées

| API | Rôle | Publication |
|---|---|---|
| `ne2k_dhcp_poll_ack` | Polling borné d’un datagramme UDP DHCP ACK `67 → 68`. | Copie le bail seulement lorsque `net_dhcp_parse_ack` réussit. |
| `ne2k_dhcp_acquire` | Orchestration `DISCOVER → OFFER → REQUEST → ACK`. | Copie le bail local vers le bail appelant seulement après ACK. |

`ne2k_dhcp_acquire` reçoit deux buffers distincts : un buffer TX pour DISCOVER et REQUEST, et un buffer RX pour OFFER et ACK. Les capacités sont explicites, l’attente RX est bornée par `poll_attempts`, et aucune allocation dynamique n’est effectuée.

## Flux transactionnel

| Étape | Primitive | Échec retourné par l’orchestrateur |
|---|---|---:|
| Validation des pointeurs et bornes | Garde locale | `-1` |
| Diffusion DISCOVER | `ne2k_dhcp_discover` | `-2` |
| Attente OFFER validée | `ne2k_dhcp_poll_offer` | `-3` |
| Émission REQUEST | `ne2k_dhcp_request` | `-4` |
| Attente ACK validé | `ne2k_dhcp_poll_ack` | `-5` |
| Publication du bail | Affectation finale | `0` |

Le polling ACK filtre les paquets sur UDP source `67`, destination `68` et taille DHCP minimale. `net_dhcp_parse_ack` vérifie ensuite le XID et les options DHCP avant de produire un `net_dhcp_lease_t` local. L’état appelant est inchangé sur toute erreur, y compris en cas de RX vide ou de datagramme non conforme.

## Sécurité et mémoire

Les adresses IPv4 obtenues et les buffers restent la propriété de l’appelant. Le pilote ne conserve aucun pointeur vers les buffers TX/RX, et aucun secret fournisseur, clé TLS ou identité LLM ne transite dans le protocole DHCP. Le flux est compatible avec le prérequis de MAC PROM validée au boot livré par AOS-593 à AOS-600.

## Tests et validation locale

Le test NE2000 ajouté initialise un bail sentinelle, invoque les gardes de `ne2k_dhcp_acquire` et `ne2k_dhcp_poll_ack` avec des paramètres invalides, puis vérifie que tous les octets sentinelles restent inchangés. Les codecs DHCP existants continuent de vérifier OFFER, REQUEST et ACK valides.

| Vérification | Résultat |
|---|---|
| Test NE2000 ciblé | **16/16** réussis. |
| Suite complète | **379/379** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi. |
| Smoke QEMU NE2000 | Réussi. |

## Limites connues

Cette API n’attache pas encore un bail à un contexte noyau persistant ni ne configure automatiquement une route, une passerelle, un DNS ou un endpoint LLM. Le bootstrap DNS/TLS existe sous forme d’API caller-owned ; son raccordement à un service noyau, les syscalls d’émission/polling contrôlés, les identifiants hors image, la validation TLS live, timeout/retry, fermeture TCP/TLS, historique conversationnel, tool calls, multimodal et Unicode complet restent à réaliser.

## Références

[1] [AOS-121 — codec DHCP caller-owned](aos121_dhcp_codec.md)  
[2] [AOS-145 — offre DHCP et état de bail caller-owned](aos145_dhcp_offer_lease.md)  
[3] [AOS-593 à AOS-600 — identité Ethernet requise par la session LLM](aos593_600_llm_network_identity.md)
