# AOS-020 — conception du jalon GGUF et quantification

## Constat de départ

Le moteur embarqué lit aujourd’hui un checkpoint **`llm.c v3`** propre à GPT-2 : en-tête fixe de 1 024 octets, suivi des poids FP32 dans un ordre implicite. Les paramètres sont adressés comme des tableaux de `float` par `gpt2_model.c` et `gpt2_infer.c`. Il ne peut donc pas charger un fichier GGUF sans nouveau chargeur, nouvelle table de tenseurs et kernels de produit matrice-vecteur adaptés.

## Sous-ensemble proposé

Le jalon initial doit rester compatible i386 freestanding et observable en QEMU. Il couvre :

| Élément | Jalon AOS-020 |
|---|---|
| Conteneur | Lecture et validation **GGUF v3** en little-endian |
| Architecture acceptée | Métadonnée `general.architecture = gpt2` |
| Types inspectés et kernels disponibles | F32, Q8_0, Q3_K, Q4_K et Q6_K ; autres types classés comme non supportés |
| Quantification | Blocs Q8_0 de 32 poids et super-blocs K-quants de 256 valeurs, avec échelles FP16/quantifiées selon le layout |
| Exécution | Kernels de dot-product Q8_0, Q3_K, Q4_K et Q6_K × activation FP32 ; chemin FP32 conservé comme référence |
| Validation | Test de parsing synthétique, test de rejet d’un fichier invalide, benchmark QEMU séparant chargement et génération |

Le format GGUF est auto-descriptif : il fournit le magic `GGUF`, la version, le nombre de tenseurs, les métadonnées et les offsets de données alignés. Sa métadonnée `general.architecture` connaît notamment la valeur `gpt2`. Le type `Q8_0` est un schéma historique par blocs de 32 poids dont la formule est `w = q × block_scale`. Ces propriétés permettent un lecteur réduit et sûr ; les kernels MOHHDY couvrent maintenant Q8_0, Q3_K, Q4_K et Q6_K sans prétendre supporter toutes les familles de modèles ni tous les types GGUF.[1] [2]

## Validation contre un GPT-2 GGUF réel

Un artefact public de référence, [`tensorblock/gpt2-GGUF`](https://huggingface.co/tensorblock/gpt2-GGUF), confirme que GPT-2 est distribué en GGUF avec des quantifications mixtes. L’inspection locale du fichier `gpt2-Q3_K_M.gguf` a relevé : GGUF v3, `general.architecture = gpt2`, 149 tenseurs, alignement implicite de 32 octets, et un mélange de F32, Q3_K, Q4_K et Q6_K. Les noms de tenseurs essentiels sont `token_embd.weight`, `position_embd.weight`, `output_norm.{weight,bias}`, `output.weight` et, pour chaque bloc, `blk.N.attn_{norm,qkv,output}.{weight,bias}` ainsi que `blk.N.ffn_{norm,up,down}.{weight,bias}`. Cette convention est cohérente avec la table de mapping de llama.cpp.[4] [5]

Les kernels Q3_K/Q4_K/Q6_K sont maintenant implémentés et comparés sur des super-blocs synthétiques, ce qui permet le calcul quantifié borné au niveau mathématique. Le premier noyau bare-metal ne prétend toutefois pas encore exécuter immédiatement ce fichier Q3_K_M : le chargeur GGUF ne conserve pas encore une table de tenseurs réutilisable par le forward GPT-2. La compatibilité structurale, les kernels et les cas de rejet restent testables sans charger un modèle de 94 Mio.

## Limite de performance

> La quantification réduit principalement le volume des poids et les accès mémoire. Elle ne garantit pas, à elle seule, une latence inférieure à une seconde sous QEMU TCG : les projections denses, la conversion des blocs et l’émulation restent coûteuses.

L’objectif réaliste du jalon est d’ajouter un chemin vérifiable, de mesurer la mémoire et la latence, puis de documenter honnêtement la différence avec le chemin FP32. Un objectif inférieur à une seconde doit être mesuré sur exécution native ou KVM ; il ne sera pas déclaré atteint sans résultat reproductible.

## Références

[1] [GGUF — spécification ggml](https://github.com/ggml-org/ggml/blob/master/docs/gguf.md)

[2] [Hugging Face — types de quantification GGUF](https://huggingface.co/docs/hub/en/gguf)

[3] [llama.cpp — guide de quantification](https://github.com/ggml-org/llama.cpp/blob/master/tools/quantize/README.md)

[4] [TensorBlock — GPT-2 GGUF](https://huggingface.co/tensorblock/gpt2-GGUF)

[5] [llama.cpp — mapping des noms de tenseurs](https://github.com/ggml-org/llama.cpp/blob/master/gguf-py/gguf/tensor_mapping.py)
