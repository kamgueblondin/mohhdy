# AOS-1593 à AOS-1600 — session GGUF locale persistante

> **Statut : livré et validé localement.** Ce macro-lot permet de poursuivre une génération GPT-2 quantifiée stockée sur FAT16, un jeton à la fois, sans recalculer le prompt ni réinitialiser le cache KV entre deux commandes shell.

## Objectif et périmètre

Le runtime GGUF local produisait déjà un premier jeton depuis `GPT2.GGU` après la sélection de `gpt2.gguf`. Ce lot introduit une **session de génération persistante** dans le noyau : `ai <question>` initialise son contexte et émet le premier jeton, puis `ai-continue` demande les jetons suivants. Le traitement reste volontairement coopératif : une commande ne calcule qu’un jeton, ce qui évite de monopoliser le noyau i386 pendant une séquence complète.

| Élément | Contrat livré |
|---|---|
| Sélection | `ai-provider local` et `ai-model use gpt2.gguf` sélectionnent le chemin GGUF local. |
| Initialisation | `ai <question>` tokenise le prompt, initialise l’état de session et génère le premier jeton. |
| Reprise | `ai-continue` génère exactement un jeton supplémentaire à partir de l’état conservé. |
| Arrêt normal | Le token de fin GPT-2 ou la saturation du contexte termine la session et retourne une sortie vide. |
| Portée | Une unique session GGUF globale est conservée en mémoire noyau ; elle est volatile et réinitialisée au boot ou lors d’un nouveau prompt GGUF. |

Le chemin concerne uniquement l’inférence **locale GGUF**. Il ne modifie ni les fournisseurs réseau OpenAI/Ollama, ni les sessions HTTP/SSE, ni le runtime GPT-2 FP32 historique.

## ABI Ring 3 ↔ Ring 0

L’ABI système ajoute `SYS_GPT2_GGUF_CONTINUE = 110` et fixe `MAX_SYSCALLS` à `111`. Le dispatcher transmet le buffer de sortie situé dans `ECX` et sa capacité dans `EDX` à `sys_gpt2_gguf_continue`. Le wrapper Ring 3 exécute l’interruption `0x80` avec cette convention ; la commande shell ne reçoit aucun argument.

| Valeur de retour | Signification |
|---|---|
| `> 0` | Nombre d’octets du fragment de jeton écrit dans le buffer et terminé par NUL. |
| `0` | Fin normale : token EOT rencontré ou contexte déjà arrivé à sa limite. |
| `-1` | Buffer absent ou capacité inférieure à deux octets. |
| `-6` | Aucune session GGUF active : l’utilisateur doit d’abord appeler `ai <question>`. |
| `-4` | Échec de décodage du token généré. |
| `-30 + rc` | Propagation bornée d’une erreur du runtime GGUF sous-jacent. |

> L’absence de session est un état attendu, non un crash : le shell l’affiche comme un diagnostic et conserve sa réactivité.

## État statique et absence d’allocation dynamique

L’état de session est entièrement contenu dans le noyau : le tableau `gguf_session_tokens[64]`, son compteur, le nombre de jetons du prompt, l’état RNG et le drapeau actif. Il n’utilise aucun `kmalloc`, `malloc`, `calloc` ni `realloc`. Les poids restent lus depuis FAT16 et l’inférence conserve le cache KV statique déjà présent dans le backend GGUF.

| Ressource | Limite ou comportement |
|---|---|
| Historique de tokens | 64 positions au total, prompt et jetons générés inclus. |
| Prompt | Encodage borné à 64 tokens ; un prompt non compatible ou un runtime non prêt est rejeté. |
| Sortie par appel | Le texte décodé est tronqué proprement à `max - 1` et toujours terminé par NUL. |
| RNG | Graine déterministe dérivée du prompt, sauvegardée entre les commandes `ai-continue`. |
| Cache KV | Réutilisé par `gpt2_gguf_generate_next_sampled` à partir de l’historique de tokens conservé. |
| Réinitialisation | `syscall_init()` remet les compteurs, la graine et le drapeau de session à zéro. |

Un nouveau `ai <question>` sur le profil GGUF invalide la session précédente avant l’initialisation du nouveau prompt. Lorsque le modèle produit EOT ou que les 64 positions sont atteintes, le noyau désactive la session, écrit une chaîne vide et retourne `0`.

## Parcours utilisateur

Après avoir démarré l’image de déploiement GGUF, la séquence attendue est la suivante :

```text
ai-provider local
ai-model use gpt2.gguf
ai bonjour
ai-continue
ai-continue
```

La première commande `ai` crée la session et affiche le premier fragment. Chaque `ai-continue` produit le fragment suivant. Lorsque la génération est terminée, le shell affiche `(fin de sequence)`. Une reprise avant toute initialisation produit le diagnostic `session indisponible (lancez d'abord ai <question>)`.

## Validation et limites

La suite complète exécute **455 tests réussis sur 455**. Le vecteur `test_sys_gpt2_gguf_continue_requires_session` vérifie que l’ABI de reprise refuse proprement une session absente (`-6`) et ne modifie pas la sortie. Le mock Unity du dispatcher couvre également l’emplacement ABI 110, tandis que la construction i386 et le smoke QEMU standard garantissent l’intégration du noyau et du shell.

| Vérification | Objectif |
|---|---|
| `make test-all` | Non-régression Unity, utilisateurs, robustesse et ABI : 455/455. |
| `make all` | Construction freestanding i386 avec le dispatcher et le shell modifiés. |
| `make qemu-smoke` | Smoke QEMU standard : boot, shell, overlay, tâches et interactions existantes. |
| Recherche d’allocations | Confirme l’absence des allocations dynamiques interdites dans les fichiers modifiés. |

La session n’est pas multi-utilisateur, n’est pas persistée sur disque et ne fournit pas encore d’API d’annulation, de configuration de longueur ni de génération par lot. La latence du premier jeton dépend toujours du forward quantifié sur FAT16 ; les optimisations de lecture et de préchargement restent le prochain axe de travail.

## Références

[1]: ../kernel/syscall/syscall.c "État de session GGUF, génération et dispatcher ABI"
[2]: ../include/os_syscalls.h "Numéros de syscalls partagés"
[3]: ../userspace/shell.c "Wrapper Ring 3 et commandes ai / ai-continue"
[4]: ../tests/unit/kernel/test_syscall.c "Vecteur Unity de refus sans session"
[5]: ../tests/framework/kernel_mocks.c "Dispatcher simulé Unity"

Les contrats et limites décrits ci-dessus sont implémentés dans le dispatcher noyau [1], l’ABI partagée [2] et le shell Ring 3 [3]. Le comportement de refus est couvert par les tests [4] [5].
