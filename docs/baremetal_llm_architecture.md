# Architecture LLM bare-metal - MOHHDY

## Objet

MOHHDY est un hobby OS **i386 32-bit** (BIOS/Multiboot, QEMU). Le chemin d'inférence livré est GPT-2 124M depuis l'initrd. Un profil GGUF conversationnel plus tardif (famille Qwen, machine plus large) reste une **cible d'architecture**, pas l'état courant : pas d'UEFI, pas d'inférence Qwen, pas 16 Gio requis. Voir [ETAT_REEL.md](ETAT_REEL.md).

Le terme « bare-metal » signifie ici qu'MOHHDY démarre depuis son support et fait l'inférence **dans son propre noyau**, sans OS préinstallé, sans service tiers et sans processus hôte. **Ollama n'est pas une voie retenue** : c'est un programme d'un autre système, pas un moteur embarquable dans ce noyau freestanding. L'API compatible OpenAI reste une référence d'interopérabilité pour un futur client réseau, pas une dépendance du chemin local [1] [2].

Le chemin déjà livré est GPT-2 124M (`llm.c` v3) depuis l'initrd. La sonde GGUF v3 et les primitives Q8_0 existent ; les kernels Q3_K/Q4_K/Q6_K manquent encore. Un GGUF trop gros pour l'initrd pourra plus tard vivre sur le **volume FAT** prévu, pas sur un système à inodes. Voir [aos_fat_volume.md](aos_fat_volume.md) et [ETAT_REEL.md](ETAT_REEL.md).

## Interface du shell

| Commande | Effet actuel |
|---|---|
| `ai-provider` | Affiche le fournisseur sélectionné. |
| `ai-provider local` | Sélectionne le chemin de modèle local. |
| `ai-provider openai` | Sélectionne le fournisseur en ligne et affiche clairement que le réseau et TLS restent à intégrer. |
| `ai-model list` | Affiche les modèles décrits dans le manifeste. |
| `ai-model use gpt2_124M.bin` | Profil local livré (checkpoint `llm.c` v3). |
| `ai-model use qwen2.5-1.5b-instruct-q4_0.gguf` | Profil déclaré dans le manifeste ; **non exécuté** (kernels GGUF absents). |
| `ai-runtime` | Affiche l'état réel du moteur local et du fournisseur en ligne. |

`ai <question>` appelle `SYS_GPT2_GENERATE` lorsque les poids GPT-2 sont dans l'initrd. Ce n'est plus un simulateur. Un profil GGUF mémorisé n'est pas exécuté tant que les kernels Q3_K/Q4_K/Q6_K manquent.

## Support de démarrage et manifeste

La construction de l'initrd crée `/models/models.manifest` avec le format GGUF et le profil local de référence. Le fichier de poids GGUF n'est pas inclus, car il doit être obtenu depuis une source autorisée par son détenteur puis placé dans le répertoire `/models` du support de démarrage. Cette séparation évite d'ajouter un binaire de modèle massif ou potentiellement sous licence restrictive au dépôt source.

| Élément | État | Rôle |
|---|---|---|
| Manifeste de modèles | Intégré | Déclare le format, le modèle par défaut et la famille du modèle. |
| Volume FAT sur disque IDE | À développer (AOS-026) | Lit un GGUF trop gros pour l'initrd depuis un volume FAT, sans toucher à l'overlay AIOV. |
| Analyseur GGUF et tokenizer Qwen | Partiel / à développer | Sonde GGUF v3 et BPE GPT-2 livrés ; tokenizer Qwen et exécution quantifiée absents. |
| Noyau tensoriel CPU et échantillonneur | GPT-2 FP32 livré ; GGUF quantifié à développer | GPT-2 : embeddings, attention, MLP, cache KV, top-k. Kernels Q3_K/Q4_K/Q6_K absents. |
| Allocation mémoire de grande capacité | Partiel | 1 Gio QEMU pour GPT-2 124M ; une cible 16 Gio/UEFI n'est pas le prototype actuel. |
| Pilote Ethernet, TCP/IP, DNS et TLS | À développer | Permet le fournisseur OpenAI sans transférer de secret dans l'image. |

## Sécurité des fournisseurs

Le fournisseur `local` n'exige aucun secret. Le fournisseur `openai` doit conserver sa clé **hors de l'image bootable et hors du dépôt Git**. Quand la pile réseau existera, l'implémentation devra préférer une saisie temporaire dans une zone mémoire effacée à la fin de session ou une configuration chiffrée par machine. Elle ne doit ni écrire la clé dans l'initrd, ni dans les journaux série, ni dans le manifeste de modèles.

## Étapes de réalisation restantes

La prochaine étape technique du moteur est d'implémenter les kernels GGUF Q3_K/Q4_K/Q6_K pour GPT-2, puis seulement une autre famille (tokenizer Qwen) si un profil le justifie. Un checkpoint trop gros pour l'initrd se lira depuis le volume FAT (AOS-026). La compatibilité universelle n'est pas réaliste.

## Références

[1] [Ollama - documentation d'installation (service d'un OS hôte, hors périmètre MOHHDY)](https://docs.ollama.com/linux)

[2] [Ollama - Compatibilité avec l'API OpenAI](https://docs.ollama.com/api/openai-compatibility)

[3] [llama.cpp - moteur d'inférence LLM C/C++](https://github.com/ggml-org/llama.cpp)
