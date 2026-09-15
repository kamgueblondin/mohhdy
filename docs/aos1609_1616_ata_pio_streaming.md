# AOS-1609 à AOS-1616 — transferts ATA PIO groupés pour le runtime GGUF

> **Statut : livré et mesuré.** Ce macro-lot réduit l’overhead de copie des secteurs ATA PIO sans modifier l’ABI, les formats FAT16/GGUF ou les buffers alloués statiquement.

## Objectif

Le forward GPT-2 GGUF Q3_K réel lit le modèle depuis le disque FAT16. Le pilote ATA envoyait auparavant 256 instructions `inw` ou `outw` depuis une boucle C pour chaque secteur de 512 octets. Sous QEMU TCG, cette boucle multiplie le coût d’émulation des accès port I/O sur les nombreux secteurs des matrices quantifiées.

| Chemin | Avant | Après |
|---|---|---|
| Lecture ATA PIO d’un secteur | 256 appels C autour de `inw` | une instruction x86 répétée `rep insw` |
| Écriture ATA PIO d’un secteur | 256 appels C autour de `outw` | une instruction x86 répétée `rep outsw` |
| Buffer intermédiaire | aucun | aucun |
| Contrôle DRQ/BSY | un contrôle par secteur | inchangé |

Le pilote conserve la sélection LBA28, la commande PIO, les limites de 1 à 256 secteurs et le contrôle d’état après chaque secteur. Seule la copie contiguë des 256 mots est confiée aux primitives matérielles x86 répétées. `cld` précède chaque primitive afin de garantir une progression ascendante indépendamment du drapeau de direction antérieur.

## Contrat de sûreté

Les buffers restent fournis par l’appelant et la consommation est exactement de 512 octets par secteur. Le code ne crée ni cache ni allocation dynamique ; il avance simplement le pointeur de sortie ou d’entrée de 512 octets après chaque transfert PIO terminé.

> **Invariant :** un échec DRQ ou BSY conserve le même code d’erreur que le pilote précédent. Aucune commande suivante n’est envoyée après cet échec.

## Mesure QEMU TCG

Le smoke GGUF exécute `ai bonjour` sur le modèle `GPT2.GGU`, refuse le repli de compatibilité puis lance `ai-continue`. Les mesures ne constituent pas un benchmark matériel : elles varient avec l’hôte et l’émulation TCG. Elles établissent toutefois une comparaison du même scénario de premier token.

| Mesure QEMU TCG | Référence AOS-1601…1608 | Avec `rep insw` / `rep outsw` | Écart |
|---|---:|---:|---:|
| Premier token Q3_K réel | 529,67 s | 508,94 s | −20,73 s (−3,9 %) |
| Continuation `ai-continue` | 175,05 s | 182,27 s | variation TCG ; non retenue comme gain |

Le premier token est amélioré de manière mesurable. La suite reste dominée par la projection de vocabulaire et les produits scalaires Q3_K ; la continuation ne permet pas encore de conclure à un gain stable sur un seul échantillon.

## Validation

| Vérification | Résultat |
|---|---|
| Build freestanding i386 | Réussi avec les contraintes SSE2 existantes. |
| Smoke QEMU standard | Réussi : cœur, shell, persistance overlay, tâches et `exec`. |
| Smoke QEMU GGUF réel | Réussi : premier token puis continuation, sans repli. |
| Allocation dynamique | Aucune fonction `kmalloc`, `malloc`, `calloc` ou `realloc` ajoutée. |

La suite complète a validé **456 tests sur 456**, sans échec ni test ignoré, avant publication de la pull request.

## Suite

Le prochain axe doit porter sur les produits scalaires Q3_K et la projection de sortie, qui représente 50 257 lignes par token. Les transformations devront préserver le top-k complet, l’équivalence RNG, le workspace statique et une mesure QEMU explicite avant livraison.

## Références

[1]: ../kernel/ata.c "Pilote ATA PIO LBA28 et transferts répétés"
[2]: ../tests/scripts/ci_qemu_gguf_local_smoke.py "Smoke QEMU de génération GGUF réelle"
[3]: aos1601_1608_gguf_real_runtime.md "Référence d’exécution Q3_K réelle"

Le changement est implémenté dans le pilote ATA [1], mesuré avec le smoke QEMU [2] et comparé à la référence fonctionnelle précédente [3].
