# MOHHDY Foundation — Incrément 110 : préparation runtime GGUF

**État :** implémenté et testé.

## Objectif

Le lot 110 ajoute `gpt2_gguf_runtime_prepare`, première couche d’intégration entre l’index GGUF persistant et le runtime d’inférence. La primitive construit une table caller-owned de rôles GPT-2 pour chaque couche, puis valide les axes et les tailles physiques des tenseurs avant de publier l’état `ready`.

## Contrat mémoire

La table de couches, le buffer de construction de noms et la structure runtime appartiennent à l’appelant. Aucune allocation n’est effectuée. La préparation échoue si la table est trop courte, si le nombre de couches ou de canaux est nul, si le buffer de noms est insuffisant ou si une couche ne respecte pas les invariants GPT-2 quantifiés.

## Portée

Cette étape rend la table de tenseurs exploitable et persistante pour le futur forward GGUF. Elle ne remplace pas encore le moteur FP32 historique dans `gpt2_infer.c`; cette substitution nécessite le chargement des embeddings, positions et logits GGUF ainsi que l’orchestration complète des couches.

## Validation

Les tests couvrent le rejet des dépendances nulles et d’une capacité de table nulle. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.
