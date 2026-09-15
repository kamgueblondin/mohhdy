# Référence externe — layouts GGML K-quants

## Sources consultées

[1] [llama.cpp — `ggml-quants.h`](https://raw.githubusercontent.com/ggml-org/llama.cpp/master/ggml/src/ggml-quants.h), consulté le 16 août 2026. Le fichier expose les décodeurs `dequantize_row_q3_K`, `dequantize_row_q4_K` et `dequantize_row_q6_K`.

[2] [llama.cpp — `ggml-quants.c`](https://raw.githubusercontent.com/ggml-org/llama.cpp/master/ggml/src/ggml-quants.c), consulté le 16 août 2026. Les fonctions de référence indiquent `QK_K = 256`, le décodage Q3_K avec `hmask[32]`, `qs[64]`, `scales[12]`, `d` (110 octets), le décodage Q4_K avec `d`, `dmin`, `scales[12]`, `qs[128]` (144 octets) et le décodage Q6_K avec `ql[128]`, `qh[64]`, `scales[16]`, `d` (210 octets). L’helper Q4_K `get_scale_min_k4` utilise les quatre premiers octets pour les scales/minima bas et le packing 6 bits des indices suivants.

## Notes de compatibilité

Le code MOHHDY transpose ces formules en fonctions freestanding `gpt2_q3_k_dot_f32`, `gpt2_q4_k_dot_f32` et `gpt2_q6_k_dot_f32`, sans libc ni allocation. Les tests synthétiques utilisent des super-blocs unitaires et vérifient le produit attendu. Le runtime GPT-2 complet reste encore spécialisé dans le checkpoint FP32 historique ; ce lot ajoute les kernels et le comptage structural GGUF, mais ne prétend pas encore charger une table de tenseurs GGUF complète dans `gpt2_infer`.
