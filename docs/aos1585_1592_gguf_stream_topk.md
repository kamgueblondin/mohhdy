# AOS-1585…1592 — Projection GGUF top-k en flux

**Statut : livré et mesuré.** Le runtime GPT-2 GGUF local n’écrit plus un tableau complet de logits avant l’échantillonnage. La tête quantifiée FAT16 offre chaque logit directement à un accumulateur top-k caller-owned de huit candidats, qui applique la même pénalité de répétition, le même bannissement du dernier token généré et le même tirage déterministe que le chemin historique.

> **Invariant d’équivalence.** À logits, historique généré et état RNG identiques, le token choisi par la projection en flux est identique à celui choisi après matérialisation complète du vocabulaire.

## Livraison

| Composant | Modification | Effet |
|---|---|---|
| `gpt2_sample` | État top-k explicite avec initialisation, offre de logit et finalisation RNG | La politique d’échantillonnage est partagée par les deux chemins. |
| Loader GGUF | Projection de tête `gpt2_gguf_forward_output_top_k_fat16` | Chaque ligne quantifiée est lue, projetée puis insérée immédiatement parmi les huit candidats. |
| Pas de génération | Implémentation commune vers logits ou top-k en flux | Les embeddings, blocs, normalisation et cache KV restent strictement communs. |
| Backend persistant | Suppression du tableau statique de 50 257 floats ; conservation du dernier top-k compact associé au cache KV | Environ 201 KiB de mémoire statique de logits ne sont plus requis par le runtime local. |
| Cache KV | Invalidations explicites lorsqu’un historique généré rend une réutilisation de top-k ambiguë | Aucun candidat associé à un préfixe incompatible n’est réutilisé. |

## Contrat d’échantillonnage

L’accumulateur stocke huit paires `(token, score)` ordonnées. Lorsqu’un logit est proposé, il applique d’abord le bannissement du dernier token généré, puis la pénalité de fréquence sur les tokens déjà générés. La finalisation effectue ensuite la température et le tirage du même générateur linéaire que le chemin précédent.

| Ressource | Avant | Après |
|---|---:|---:|
| Logits persistants du backend GGUF | 50 257 floats | Aucun |
| Accumulateur de sortie | implicite dans l’échantillonneur après projection | 8 IDs + 8 scores, caller-owned |
| Lectures de tête FAT16 | séquentielles | inchangées et séquentielles |
| Politique top-k, température et RNG | historique | identique |

## Validation

Le vecteur `test_streams_output_top_k_equivalently_from_fat16` utilise la fixture GGUF quantifiée existante, projette d’abord les logits complets puis la variante en flux et vérifie l’identité du token et de l’état RNG avec un historique non vide. Les tests historiques de bannissement, pénalité et déterminisme restent verts après factorisation de l’échantillonneur.

Le smoke `make qemu-gguf-smoke` confirme le boot, la sélection `ai-model use gpt2.gguf` et le retour local sur le modèle Q3_K réel. La mesure observée pour `ai bonjour` est **16,42 s** sous QEMU TCG dans cette exécution, après une mesure de **17,03 s** pour le lot de cache sectoriel. Cette variation inclut l’émulation complète ; elle ne constitue pas une promesse de latence matérielle.

## Limites

La projection de chaque ligne de la tête reste nécessaire pour préserver l’échantillonnage sur tout le vocabulaire. Cette livraison réduit donc la mémoire d’état et les écritures de logits, mais elle ne transforme pas le coût algorithmique de la projection Q6_K. Les prochaines pistes restent la vectorisation SSE2 des kernels, une politique de cache de pages de poids statique et une génération coopérative sans blocage prolongé du shell.
