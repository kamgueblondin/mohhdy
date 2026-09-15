# AOS-705 à AOS-712 — Entropie matérielle RDRAND pour TLS

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** génération noyau des matériaux secrets TLS par RDRAND x86, sans fallback faible

## Objectif

Ce macro-lot supprime un prérequis bloquant du polling TLS : le noyau peut désormais générer les 32 octets de `client_random` et les 32 octets de clé privée X25519 à partir de l’instruction matérielle RDRAND. Ces valeurs restent exclusivement dans les buffers statiques du noyau et ne traversent aucune ABI Ring 3.

La disponibilité RDRAND est détectée par `CPUID.01H:ECX.RDRAND[bit 30]` au boot. Le statut LLM expose uniquement cette capacité dans le bit 2 ; il ne publie jamais les octets générés, une empreinte, une longueur ni une clé.

| Élément | Contrat |
|---|---|
| Source d’aléa | RDRAND matériel x86, détecté via CPUID. |
| Taille générée | 32 octets de random TLS et 32 octets de clé X25519. |
| Nombre de tentatives | Au plus 10 par mot RDRAND de 32 bits. |
| Absence ou échec RDRAND | `OS_LLM_TLS_ENTROPY_UNAVAILABLE`, phase inchangée. |
| Échec bootstrap ultérieur | Les matériaux générés sont effacés avant le retour d’erreur. |
| Fallback RTC/TSC/compteur | Interdit. |

## Intégration transactionnelle

`kernel_llm_acquire_start` valide d’abord la requête, la disponibilité NE2000 et la phase `IDLE`. Il remplit ensuite les matériaux TLS avant toute acquisition DHCP, DNS, ARP ou SYN. Si CPUID ne signale pas RDRAND, si l’instruction échoue dix fois pour un mot, ou si l’initialisation et le bootstrap échouent après génération, les buffers `client_random` et clé privée sont écrasés et l’état LLM ne progresse pas.

> Le RTC reste une source d’heure UTC pour la période de validité X.509 ; il ne devient jamais une source d’entropie TLS.

Le champ interne `boot_llm_tls_entropy_ready` est distinct du statut de matériaux TLS entièrement prêts : l’émission du ClientHello continue d’exiger une ancre X.509 de production en plus des valeurs RDRAND. Ainsi, ce lot retire le hasard faible sans affaiblir la vérification de serveur.

## Diagnostic utilisateur

`ai-runtime` ajoute la ligne `Entropie TLS RDRAND`. Elle indique seulement `disponible (materiel)` ou `indisponible`. Le CPU QEMU de validation ne présente pas RDRAND : le smoke confirme donc explicitement le diagnostic `indisponible` et l’absence de chemin de secours logiciel.

| Bit de `SYS_LLM_SESSION_STATUS` | Signification |
|---:|---|
| 0 | NE2000 prêt. |
| 1 | Bail DHCP présent. |
| 2 | Capacité RDRAND matérielle détectée. |
| 8..15 | Phase LLM. |

## Validation locale

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi ; RDRAND absent diagnostiqué sans fallback. |
| Smoke QEMU NE2000 | Réussi ; carte prête, phase `IDLE`, bail absent. |
| Suite complète | Réussie : **384/384** tests. |
| `git diff --check` | Propre avant commit. |

## Limites connues

RDRAND satisfait seulement le prérequis d’aléa local. Le handshake réel demeure bloqué sans ancre X.509 de production configurée, et la pile actuelle accepte les chaînes RSA SHA-256 requises par son implémentation. La prise en charge de chaînes ECDSA de fournisseurs publics, le provisioning d’ancre, les credentials OpenAI, fermeture/annulation, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-673 à AOS-680 — syscall de polling TLS LLM contrôlé](aos673_680_llm_poll_tls_syscall.md)

[2] [AOS-353 à AOS-360 — politique temporelle dans l’orchestrateur NE2000](aos353_360_ne2k_tls_time_policy.md)

[3] [AOS-697 à AOS-704 — syscall de réarmement de session LLM](aos697_704_llm_session_reset_syscall.md)
