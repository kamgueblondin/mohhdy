# Incrément Foundation 67 — vue de tenseurs GGUF bornée

**État :** implémenté sur la branche `manus/mohhdy-foundation-gguf-runtime`.

## Objectif

L’incrément complète le parseur GGUF par une recherche de descripteur de tenseur. `gpt2_gguf_find_tensor` relit le conteneur validé, compare un nom exact, expose les dimensions, le type, l’offset relatif et la taille calculée, puis vérifie que la plage correspondante reste entièrement dans le blob.

La fonction couvre F32, F16, Q8_0, Q3_K, Q4_K et Q6_K. Pour les quantifications par blocs, elle exige une cardinalité compatible avec le bloc et calcule les tailles 34, 110, 144 et 210 octets par bloc selon le format. Aucun pointeur vers les données n’est retourné par l’API : l’appelant conserve le blob et peut dériver une adresse seulement après les contrôles de borne.

## Validation

La suite Unity atteint **257 tests réussis**. Le nouveau cas construit un tenseur Q4_K de 256 valeurs, vérifie son type, sa forme, sa taille de 144 octets et son offset nul, puis vérifie le nom absent et un offset hors plage. Les tests GGUF structurels, les kernels quantifiés, la robustesse et toutes les suites existantes restent verts.

## Limites

Cette vue est une primitive de chargement bornée, pas encore une table persistante de tous les tenseurs. `gpt2_model.c` et `gpt2_infer.c` restent spécialisés dans le checkpoint FP32 `llm.c v3`; la sélection d’un fichier GGUF pour un forward GPT-2 complet, le mapping des 149 tenseurs et la génération quantifiée sont donc réservés à la tranche suivante.

Le format GGUF est traité comme une donnée non fiable : les compteurs, dimensions, types, alignements, tailles de blocs et plages doivent tous être validés avant utilisation. La fonction ne fournit ni allocation, ni copie, ni persistance, ni support des types non reconnus.
