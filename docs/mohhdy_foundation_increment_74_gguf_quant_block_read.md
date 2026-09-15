# MOHHDY Foundation — Incrément 74 : lecture vérifiée des blocs quantifiés GGUF

**État :** implémenté sur la branche de travail du lot 74.

## Objectif

Le lot 74 ajoute `gpt2_gguf_read_quant_block_fat16`. Cette primitive reçoit un tenseur déjà indexé, vérifie qu’il appartient aux familles Q3_K, Q4_K ou Q6_K, calcule l’offset du super-bloc demandé et lit exactement son enveloppe binaire depuis FAT16.

Chaque super-bloc représente 256 valeurs et possède une taille connue par le kernel: 110 octets pour Q3_K, 144 octets pour Q4_K et 210 octets pour Q6_K. L’API refuse un type non supporté, un index au-delà de `byte_size`, une capacité insuffisante ou un dépassement arithmétique avant l’accès disque. Elle ne décode pas encore les échelles et sous-blocs; le résultat est une fenêtre binaire prête à être remise au kernel quantifié.

| Type | Valeurs par super-bloc | Taille contrôlée |
| --- | ---: | ---: |
| Q3_K | 256 | 110 octets |
| Q4_K | 256 | 144 octets |
| Q6_K | 256 | 210 octets |

## Validation

La fixture FAT16 du lot 70 est chargée puis indexée. Le test retrouve `output.weight` en Q4_K, lit son premier super-bloc de 144 octets depuis `GPT2.GGU`, vérifie le nombre retourné et rejette une capacité d’un seul octet. La suite `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**.

## Limites et suite

La primitive ne convertit pas le bloc en valeurs FP32 et ne lance pas de produit scalaire. Le prochain jalon pourra connecter cette lecture aux kernels Q4_K/Q6_K en imposant un buffer d’activation caller-owned et des contrôles de dimensions, sans modifier l’ABI syscall ni charger un checkpoint complet.
