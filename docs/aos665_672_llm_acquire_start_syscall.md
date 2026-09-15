# AOS-665 à AOS-672 — Syscall contrôlé de démarrage DHCP vers LLM

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** démarrage noyau DHCP→DNS→SYN depuis le shell, avec une requête ABI bornée et sans secret

## Objectif

Ce macro-lot rend accessible depuis Ring 3 le démarrage contrôlé d’une session LLM réseau. Le syscall `SYS_LLM_ACQUIRE_START` délègue au contexte noyau persistant `boot_llm_network`, puis à la façade transactionnelle `ne2k_llm_network_context_acquire_start_dhcp`. Le chemin effectue donc DHCP, sélection de route, DNS A, ARP et SYN uniquement lorsque les préconditions sont satisfaites.

La requête Ring 3 est une structure POD fixe. Elle ne contient aucun pointeur, token, clé, mot de passe, adresse IPv4, routeur, DNS ni résultat réseau.

```c
typedef struct {
    char hostname[OS_LLM_HOSTNAME_MAX];
    uint32_t xid;
    uint32_t local_sequence;
    uint16_t dns_id;
    uint16_t dhcp_attempts;
    uint16_t dns_attempts;
    uint16_t arp_attempts;
    uint16_t local_port;
    uint16_t remote_port;
} os_llm_acquire_start_request_t;
```

## Contrat ABI

| Élément | Valeur | Rôle |
|---|---:|---|
| Numéro de syscall | `91` | `SYS_LLM_ACQUIRE_START` ; `EBX` pointe vers la requête POD. |
| Taille maximale du hostname | `96` octets | Le nom doit être NUL-terminé et composé de lettres, chiffres, `.` ou `-`. |
| Budget maximal | `8` tentatives | Valeur appliquée indépendamment aux étapes DHCP, DNS et ARP. |
| Port local et distant | `1..65535` | Valeurs fournies par Ring 3, validées dans le noyau. |
| Retour `0` | Succès | Bail, connexion et phase `SYN_SENT` sont publiés ensemble. |
| Retour négatif | Rejet ou échec | Aucun détail sensible n’est retourné et le contexte n’est pas publié partiellement. |

La commande shell associée est `ai-acquire <hostname> [port]`. Elle construit une requête locale avec les valeurs par défaut suivantes : port local `49152`, port distant `443`, deux tentatives DHCP/DNS/ARP, ainsi que des identifiants DHCP, DNS et de séquence non secrets. Un port non numérique, nul ou supérieur à `65535` est refusé dans le shell.

## Contrôle noyau et sûreté

Le noyau conserve l’objet réseau, le descripteur d’E/S NE2000, le cache ARP et cinq buffers Ethernet fixes. Il n’effectue aucune allocation dynamique. Avant toute émission, il valide le hostname, les budgets, les ports, la disponibilité de la carte et la phase `IDLE`.

| Condition | Code retourné | Effet sur le contexte LLM |
|---|---:|---|
| Requête absente ou invalide | `OS_LLM_ACQUIRE_BAD_REQUEST` | Aucun changement. |
| NE2000 indisponible | `OS_LLM_ACQUIRE_UNAVAILABLE` | Aucun changement. |
| Phase différente de `IDLE` | `OS_LLM_ACQUIRE_IN_PROGRESS` | Aucun changement. |
| Échec DHCP, DNS, ARP ou SYN | `OS_LLM_ACQUIRE_FAILED` | Aucun changement publié ; la façade réseau reste transactionnelle. |
| Succès complet | `0` | Publication atomique du bail, de la connexion TCP et de `SYN_SENT`. |

> Le syscall ne reçoit ni ne conserve d’identifiant fournisseur. Les secrets nécessaires à une requête HTTP authentifiée restent explicitement hors de cette interface et ne sont jamais intégrés à l’image de boot.

## Validation

Le smoke QEMU fournisseur déclenche d’abord un rejet de port (`0`), puis une requête valide sans NE2000. Il vérifie que cette seconde demande retourne une indisponibilité explicite et que le statut reste `IDLE` avec bail DHCP absent. Le smoke QEMU NE2000 vérifie en parallèle que la présence de la carte seule ne publie aucun bail.

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi ; validation de port, rejet sans NE2000 et absence de publication. |
| Smoke QEMU NE2000 | Réussi ; carte prête, phase `IDLE`, bail absent. |
| Tests transactionnels NE2000 existants | Préservent les états sentinelles sur garde ou échec de bootstrap. |

## Limites connues

Le macro-lot s’arrête après l’émission du SYN. Le polling SYN-ACK et le handshake TLS restent à exposer par syscall ; l’émission HTTP authentifiée, le polling de texte ou SSE, le provisionnement d’identifiants, la fermeture, les délais/reprises, les outils, le multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-657 à AOS-664 — contexte réseau LLM dans le plan de contrôle noyau](aos657_664_llm_kernel_network_context.md)

[2] [AOS-649 à AOS-656 — contexte réseau LLM caller-owned](aos649_656_llm_network_context.md)

[3] [AOS-641 à AOS-648 — façade unifiée DHCP vers session LLM](aos641_648_llm_dhcp_unified_start.md)
