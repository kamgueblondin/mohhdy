# AOS-1641 à AOS-1648 — clôture mesurée de l’axe de latence GGUF

## Objet

Ce lot clôt l’exploration des optimisations locales du runtime GPT-2 GGUF Q3_K lu depuis FAT16. La règle de décision est conservée : une optimisation ne peut être retenue que si son gain est **reproductible au-delà de la variabilité observée de QEMU TCG**, approximativement trois à cinq secondes pour le premier token et une à deux secondes pour la continuation. Le runtime reste entièrement freestanding, i386 et sans allocation dynamique.

## État retenu

| Composant | État retenu | Justification |
|---|---|---|
| Transferts ATA PIO | `rep insw` / `rep outsw` | Gain mesuré et chemin matériel inchangé. |
| FAT16 | Fenêtre caller-owned de 8 Kio sur 16 secteurs, conservée entre clusters ; cache FAT isolé | Réduit fortement les lectures physiques et conserve les contrats d’erreur. |
| Q3_K | Décodage sans branche | Équivalence fonctionnelle et réduction déterministe du contrôle conditionnel. |
| Cache KV et session locale | Résidents, bornés à 64 positions | Évite le recalcul du contexte sans allocation dynamique. |

Les deux essais complémentaires suivants ont été exécutés sur le vrai disque `GPT2.GGU`, ont franchi les tests unitaires applicables, puis ont été retirés car aucune amélioration robuste n’a été observée.

| Essai retiré | Empreinte additionnelle | Mesure QEMU TCG | Décision |
|---|---:|---:|---|
| Cache paresseux des huit vecteurs constants de chaque couche | Environ 474 Kio statiques | 50,08 s puis 21,88 s | Rejeté : pas de gain, budget mémoire inutile. |
| Spécialisation Q3_K de la ligne top-k pour éviter le dispatch par super-bloc | Aucune | 49,94 s puis 21,43 s | Rejeté : variation contradictoire et inférieure au seuil de confiance. |

Après retrait des deux essais, le smoke propre a produit **48,59 s** pour le premier token et **23,08 s** pour `ai-continue`. Cette mesure est cohérente avec la variabilité déjà documentée : elle ne constitue pas une régression ni une promesse de performance matérielle.

> Le forward d’un token GPT-2 124M contient environ 84 934 656 multiplications dans les douze couches et 38 597 376 dans la projection de vocabulaire, soit environ 123 532 032 multiplications. Les micro-optimisations de normalisation, de copies de petits vecteurs ou de dispatch n’ont donc pas une marge crédible de plusieurs secondes sous QEMU TCG.

## Validation de clôture

La suite complète a été relancée après tous les retraits. Elle valide **457 tests sur 457** avec succès. Le dépôt a été vérifié propre et synchronisé avec `origin/master`. Le smoke GGUF réel valide toujours l’initialisation du modèle FAT16, le premier token de `ai` et l’avancement coopératif par `ai-continue`.

## Limites et suite

Cette clôture ne prétend pas épuiser les optimisations possibles sur matériel réel. Une mesure sur une cible i386 physique, ou un profiler capable d’isoler ATA, décodage Q3_K et calcul scalaire hors bruit d’émulation, serait nécessaire avant de réouvrir cet axe. La priorité de livraison bascule donc vers les fonctionnalités encore explicitement ouvertes, en commençant par la recherche FAT16 par nom long pour les opérations de lecture.

## Références internes

- [État réel du projet](ETAT_REEL.md)
- [Backlog de livraison](todo.md)
- [Mesures de la fenêtre FAT16 inter-clusters](aos1625_1632_fat16_intercluster_window.md)
- [Décodage Q3_K sans branche et variabilité QEMU](aos1633_1640_q3k_branchless_decode.md)
- [Contrat LFN FAT16 existant](aos1193_fat16_lfn.md)
