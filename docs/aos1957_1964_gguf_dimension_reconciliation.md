# AOS-1957…1964 — Réconciliation de la validation dimensionnelle GGUF

## Objet

L’état historique du lot GGUF indiquait que la validation sémantique des dimensions restait à faire avant le branchement aux kernels quantifiés. Cette limite est désormais obsolète.

| Contrat actuel | Validation appliquée |
|---|---|
| `gpt2_gguf_generation_prepare` | Rangs, bornes 32 bits, dimensions `C/V/T`, cohérence embeddings/normes/sortie, types de tenseurs et tailles encodées. |
| `gpt2_gguf_validate_gpt2_layer_storage` | Axes des matrices de bloc GPT-2 et cohérence du stockage quantifié par couche. |
| `gpt2_gguf_generation_token_fat16` | Exécution caller-owned des étapes de génération avec kernels quantifiés et cache KV. |

> La note historique est conservée dans `ETAT_REEL.md`, mais elle est maintenant explicitement reliée aux préparations et kernels livrés ultérieurement.

## Garanties

La réconciliation ne change ni les formats GGUF ni les contrats de mémoire. Les buffers de noms, couches, espaces de travail et cache KV restent intégralement caller-owned ; aucune allocation dynamique n’est introduite.

## Validation

Le code concerné est couvert par les suites `test_gpt2_gguf`, `test_gpt2_gguf_infer`, `test_gpt2_gguf_bounds` et la suite globale `make -s test-all`.
