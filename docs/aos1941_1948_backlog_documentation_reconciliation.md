# AOS-1941…1948 — Réconciliation de la documentation historique du backlog

## Objet

Plusieurs documents de macro-lots anciens décrivaient des fonctionnalités comme futures parce qu’elles n’étaient pas disponibles à leur date de publication. Ces formulations restaient visibles après la livraison des lots successeurs et pouvaient faire apparaître à tort des axes déjà clôturés comme ouverts.

Ce lot conserve les contrats et résultats historiques, mais remplace les limites obsolètes par une note de succession reliant chaque domaine aux implémentations qui l’ont complété.

| Document historique | Réconciliation |
|---|---|
| AOS-729…736 bigint | ECDSA P-256, X.509 ECDSA et TLS ECDHE-ECDSA référencés vers AOS-745…808. |
| AOS-1289…1304 FAT32 | Publication, lecture, suppression et renommage LFN référencées vers AOS-1321 et AOS-1657…1752. |
| AOS-1373…1384 LLM socket | Orchestration réseau, polling SSE, reprise, retry et DHCP référencés vers AOS-649…664, AOS-1017…1032 et AOS-1769…1880. |

## Méthode

Les passages modifiés sont explicitement signalés comme des **notes historiques réconciliées**. Cette formulation empêche d’attribuer rétroactivement les fonctions au lot d’origine tout en maintenant un chemin de lecture exact vers les livraisons ultérieures. Les limites réellement hors périmètre, telles que la révocation et les chaînes X.509 non bornées, restent indiquées.

> L’index de backlog devient ainsi la source opérationnelle de l’état livré, tandis que chaque document de lot reste une archive technique fidèle de son propre périmètre.

## Validation

La réconciliation ne modifie aucun binaire, contrat C ni vecteur de test. `git diff --check` assure l’intégrité de format ; la suite complète reste exécutée avant publication du lot documentaire.
