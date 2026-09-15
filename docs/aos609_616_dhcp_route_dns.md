# AOS-609 à AOS-616 — Routeur et DNS dans le bail DHCP

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** codec DHCPv4, bail caller-owned, routeur, DNS, préparation bootstrap LLM

## Objectif

Ce macro-lot complète l’acquisition DHCP transactionnelle avec les paramètres réseau indispensables au bootstrap LLM dynamique. Le client DHCP demande désormais les paramètres IPv4, routeur et DNS ; un ACK peut publier le premier routeur et le premier DNS proposés, sans exposer ni stocker de données sensibles.

> Le bail DHCP reste inchangé lorsqu’un ACK est incomplet, invalide, tronqué ou porte une option routeur/DNS malformée.

## Demande de paramètres DHCP

`net_dhcp_build_discover` construit maintenant une liste de requête de paramètres DHCP (option `55`) qui demande :

| Paramètre | Code DHCP | Finalité |
|---|---:|---|
| Masque de sous-réseau | `1` | Préparation de la configuration IPv4. |
| Routeur | `3` | Passerelle pour joindre un DNS ou un hôte LLM hors sous-réseau. |
| DNS | `6` | Résolution de nom du fournisseur LLM. |

Le DISCOVER passe de 244 à 249 octets ; la capacité minimale est ajustée de façon cohérente.

## Bail DHCP étendu

`net_dhcp_lease_t` expose désormais les champs suivants, tous détenus par l’appelant :

| Champ | Signification |
|---|---|
| `router_valid`, `router_ipv4` | Premier routeur de l’option DHCP 3. |
| `dns_valid`, `dns_ipv4` | Premier serveur de l’option DHCP 6. |

Les options routeur et DNS acceptent une liste d’adresses IPv4. Le codec sélectionne la première adresse et exige une longueur positive, multiple de quatre. Une longueur de 1, 2, 3 ou toute autre valeur non multiple de quatre entraîne un échec sans publication du bail temporaire.

## Transactionnalité

Le parsing ACK utilise un `net_dhcp_lease_t` local intégralement initialisé. Le serveur, le routeur, le DNS, l’IPv4 et le XID sont écrits dans cette copie locale, puis publiés en une seule affectation uniquement après validation du type ACK et de l’identifiant serveur. `net_dhcp_lease_apply` et `net_dhcp_lease_clear` appliquent le même principe et réinitialisent les nouveaux champs.

Aucune allocation dynamique n’est réalisée. Les buffers DHCP, le bail et la suite de bootstrap DNS/TLS restent fournis par l’appelant.

## Tests et validation locale

Le vecteur DHCP couvre l’option de requête de paramètres dans DISCOVER, un ACK contenant routeur et deux DNS, l’extraction du premier DNS, puis une option routeur rendue volontairement malformée. Cette dernière vérifie que les indicateurs et adresses précédemment publiés restent inchangés.

| Vérification | Résultat |
|---|---|
| Test DHCP ciblé | réussi. |
| Test NE2000 ciblé | **16/16** réussis. |
| Suite complète | **379/379** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi. |
| Smoke QEMU NE2000 | Réussi. |

## Limites connues

Le bail enrichi n’est pas encore attaché à une interface noyau active : la configuration de route et de passerelle, le bootstrap DNS/TCP/TLS automatique à partir du bail, le service LLM noyau, les syscalls d’émission/polling, les identifiants hors image, le TLS live, timeout/retry, fermeture de session, outils, multimodal et Unicode complet restent à réaliser.

## Références

[1] [AOS-601 à AOS-608 — acquisition DHCP transactionnelle NE2000](aos601_608_dhcp_transactional_acquire.md)  
[2] [AOS-593 à AOS-600 — identité Ethernet requise par la session LLM](aos593_600_llm_network_identity.md)  
[3] [AOS-537 à AOS-544 — orchestrateur LLM DNS vers ClientHello](aos537_544_llm_connection_orchestrator.md)
