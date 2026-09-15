# MOHHDY Foundation — Incrément 84 : accès borné aux rôles de couche

**État :** implémenté sur la branche de travail du lot 84.

## Objectif

Le lot 83 prépare un contexte de forward contenant un `gpt2_gguf_layer_t`. Le lot 84 ajoute `gpt2_gguf_layer_get`, un accesseur borné qui transforme un rôle GPT-2 de couche en entrée du tableau, exige le bit correspondant dans `present_mask` et copie le descripteur vers un buffer caller-owned.

L’accesseur refuse les rôles globaux, les rôles hors de l’intervalle des dix familles de couche et les entrées absentes. Cette étape sépare la résolution GGUF du futur code d’exécution et évite au forward de manipuler directement un index de tableau non contrôlé.

## Contrat

| Cas | Résultat |
| --- | --- |
| rôle de couche présent | copie du descripteur, retour `0` |
| rôle global | retour `-1` |
| rôle hors intervalle | retour `-1` |
| bit absent du masque | retour `-8` |
| pointeur nul | retour `-1` |

La validation dimensionnelle sémantique complète reste différée: les fixtures actuelles couvrent surtout le raccord d’indexation et les bornes, tandis que le contrat legacy fournit les dimensions attendues pour un prochain lot.

## Validation

Le test GGUF vérifie l’accès au rôle `ffn_down`, le rejet d’un rôle global, le rejet d’un masque vide et le rejet d’une couche absente. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**. Aucun buffer global ou allocation dynamique n’est introduit.

## Suite

Le prochain incrément peut valider les formes et types par rôle à partir de `gpt2_config_t` (`channels`, `num_heads`, `num_layers`) avant de connecter les lectures paginées FAT16 aux kernels quantifiés.
