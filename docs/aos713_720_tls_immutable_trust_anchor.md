# AOS-713 à AOS-720 — Ancre X.509 RSA immuable pour TLS

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** ancre de confiance X.509 compilée dans le noyau, parsing au boot et aucun chargement de CA depuis Ring 3

## Objectif

Ce macro-lot complète le prérequis de confiance du handshake TLS. Le noyau embarque l’ancre auto-signée **ISRG Root X1** dans `kernel/tls_trust_anchor.h`, sous forme DER immuable. Au boot, il parse le DER et valide la présence d’une clé publique RSA avant de publier l’état de confiance. L’ancre est compilée avec l’image et ne peut être ajoutée, supprimée ou remplacée par un syscall, une commande shell, un fichier Ring 3 ou un téléchargement réseau.

L’artefact intégré contient 1 391 octets et son SHA-256 est `96bcec06264976f37460779acf28c5a7cfe8a3c0aae11a8ffcee05c0bddf08c6`. La page officielle Let’s Encrypt décrit ISRG Root X1 comme une racine RSA 4096 auto-signée, avec une confiance annoncée jusqu’au 4 juin 2030. [1]

| Élément | Contrat |
|---|---|
| Format intégré | DER, tableau C constant, 1 391 octets. |
| Ancre | ISRG Root X1, RSA 4096, signature SHA-256 RSA. [1] |
| Vérification au boot | `x509_certificate_parse` puis `x509_rsa_public_key_validate`. |
| Bit de statut | Bit 3 de `SYS_LLM_SESSION_STATUS` : ancre X.509 noyau valide. |
| Surface Ring 3 | Aucune écriture, aucun chemin, aucun pointeur ou contenu DER accepté. |
| Échec de parsing | Confiance non prête ; le polling TLS conserve son refus sécurisé. |

## Cohérence avec les matériaux TLS

La confiance est distincte de l’entropie. Lors de `SYS_LLM_ACQUIRE_START`, les 64 octets secrets TLS sont générés par RDRAND dans le noyau. La variable interne de préparation TLS devient vraie seulement lorsque RDRAND a réussi **et** que l’ancre compilée a été parsée et validée. Toute erreur de bootstrap efface les matériaux RDRAND et désactive l’état de préparation.

> L’ancre n’est pas une autorisation de contourner la validation : hostname, temps RTC, signatures de chaîne et Finished TLS restent vérifiés par les couches existantes.

| Bit de `SYS_LLM_SESSION_STATUS` | Signification |
|---:|---|
| 0 | NE2000 prêt. |
| 1 | Bail DHCP présent. |
| 2 | Capacité RDRAND matérielle détectée. |
| 3 | Ancre ISRG Root X1 compilée et validée par le noyau. |
| 8..15 | Phase LLM. |

`ai-runtime` expose uniquement la phrase `Ancre X.509 noyau : ISRG Root X1 validee`. Il ne révèle ni le DER, ni la clé publique, ni une empreinte mutable, et ne fournit aucun mécanisme pour modifier l’ensemble de confiance.

## Compatibilité et limites de chaîne

La pile actuelle valide les signatures RSA SHA-256. Elle ne prend pas encore en charge ECDSA ni une profondeur arbitraire d’intermédiaires. La source officielle Let’s Encrypt indique que ses intermédiaires RSA actifs YR1/YR2 peuvent chaîner via Root YR puis ISRG Root X1, tandis que ses branches ECDSA utilisent d’autres racines ou intermédiaires. [1] Cette ancre prépare donc une confiance de production RSA immuable, mais ne constitue pas une promesse de compatibilité immédiate avec chaque endpoint public ou chaîne présentée.

| Cas | État après ce lot |
|---|---|
| Serveur RSA SHA-256 avec chaîne compatible au validateur | Précondition d’ancre satisfaite. |
| Chaîne ECDSA | Refusée par la pile actuelle. |
| Chaîne nécessitant plus d’intermédiaires que le validateur supporte | Refusée proprement. |
| CA ajoutée dynamiquement depuis Ring 3 | Impossible par conception. |

## Validation locale

Le smoke QEMU fournisseur confirme le parsing de l’ancre au boot et le diagnostic `ISRG Root X1 validee`. Il vérifie également que le CPU QEMU sans RDRAND reste diagnostiqué comme indisponible, sans fallback. Le smoke NE2000 confirme l’initialisation de la carte et l’état LLM initial.

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi ; ancre validée au boot. |
| Smoke QEMU NE2000 | Réussi ; carte prête, phase `IDLE`, bail absent. |
| Suite complète | Réussie : **384/384** tests. |
| `git diff --check` | Propre avant commit. |

## Limites connues

Ce lot ne fournit pas ECDSA, une chaîne d’intermédiaires de profondeur arbitraire, révocation, une seconde racine de confiance ou un store modifiable à l’exécution. Les appels LLM réels attendent encore la compatibilité de chaîne serveur avec les algorithmes implémentés, ainsi qu’un CPU avec RDRAND. OpenAI requiert en outre un credential interne au noyau. Fermeture/annulation, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [Let’s Encrypt — Chains of Trust, mis à jour le 8 juillet 2026](https://letsencrypt.org/certificates/)

[2] [AOS-705 à AOS-712 — entropie matérielle RDRAND pour TLS](aos705_712_tls_rdrand_entropy.md)

[3] [AOS-673 à AOS-680 — syscall de polling TLS LLM contrôlé](aos673_680_llm_poll_tls_syscall.md)
