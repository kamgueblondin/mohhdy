# Rapport d'optimisation GPT-2 - cache KV et stabilité

**Auteur : Manus AI**
**Révision : 13 août 2026**

## Mise à jour du dépôt

Le dépôt local a été récupéré depuis `origin/master` jusqu'au commit `41e902c`. Les modifications locales GPT-2 ont été préservées sur la branche `manus/gpt2-kv-cache`, puis rebasées sur cette version distante. Les nouveaux appels système et commandes du shell ajoutés en amont, notamment `append`, `touch`, `test`, `getpid` et `rc`, ont été fusionnés avec le service de génération GPT-2.

## Correctifs de stabilité

Le signalement d'un gel après la première réponse a été reproduit et analysé. Le journal montre que le shell imprime immédiatement un nouveau prompt après la réponse GPT-2 ; le test initial avait un défaut de synchronisation, car il attendait un second prompt alors que celui-ci se trouvait déjà dans la même lecture de sortie que la réponse. Le test corrigé envoie ensuite `rc` et confirme que le shell l'exécute après une génération réelle.

| Vérification | Résultat |
|---|---|
| Réponse GPT-2 réelle | Réponse générée sous le préfixe `[GPT-2 local]` |
| Reprise du shell après réponse | `rc` accepté après la génération |
| Test de récupération QEMU | Réussi |
| Suite de non-régression après rebase | **121/121 tests réussis** |

## Cache KV implémenté

Le moteur stocke désormais, pour chaque couche et chaque position du contexte, les vecteurs **clé** et **valeur** produits par l'attention. Lorsqu'un nouveau jeton est généré, seules ses projections, son MLP et son attention vers les clés/valeurs déjà calculées sont exécutés. Le modèle ne recalcule donc plus les couches de tous les jetons précédents à chaque étape de génération.

| Élément | Avant | Après |
|---|---|---|
| Calcul lors de 4 jetons générés | Recalcul du contexte complet à chaque jeton | Un seul nouveau jeton traversant les couches à chaque étape |
| Mémoire supplémentaire | Aucune | Cache KV FP32 : environ 4,5 Mio pour GPT-2 124M, contexte 64 |
| Génération | Sélection top-k, sans mémoire d'attention | Sélection top-k, cache KV persistant tant que le préfixe est identique |
| Stabilité du shell | Test de reprise absent | Test réel : réponse puis commande `rc` réussie |

Le noyau est désormais compilé avec `-O3`, SSE2, réalignement explicite de pile (`-mstackrealign`) et un état SSE activé au démarrage. Le réalignement évite les défauts de protection générale déclenchés par des instructions SSE alignées dans certains chemins noyau, notamment la copie récursive d'overlay. Le mode graphique QEMU utilise un processeur virtuel Pentium III compatible SSE2.

## Mesures observées

Les mesures sont prises dans l'environnement de test, où QEMU fonctionne sans accélération KVM.

| Configuration | Latence `ai hello` jusqu'à 4 jetons | Commentaire |
|---|---:|---|
| Cache KV initial sans vectorisation SSE2 | 88,835 s | Mesure de référence après le cache KV |
| Cache KV + `-O3` + SSE2 et pile réalignée, CPU QEMU Pentium III | **7,693 s** | Mesure finale via `make gpt2-tests` |
| Objectif demandé | < 1 s | **Non atteint dans cet environnement** |

Le cache KV, la vectorisation et l'alignement de pile réduisent donc la latence mesurée d'un facteur d'environ **11,5**. La dernière mesure est `LATENCY_SECONDS=7.693` et a produit la sortie locale `[GPT-2 local]  to the in,`. L'objectif inférieur à une seconde reste irréaliste dans cet environnement pour GPT-2 124M en poids FP32 : chaque jeton doit encore exécuter les projections d'attention, les couches MLP et la projection sur le vocabulaire entier ; l'émulation QEMU ne bénéficie pas de KVM dans le bac à sable.

> Une latence inférieure à une seconde exige au minimum un binaire natif ou KVM sur CPU moderne, ainsi qu'un moteur quantifié (INT8 ou INT4), des kernels SIMD dédiés et idéalement un modèle plus petit ou une accélération matérielle. Le cache KV traite la partie "contexte déjà vu" ; il ne supprime pas le coût intrinsèque des couches du nouveau jeton.

## Console graphique active

La console graphique actuellement publiée utilise le cache KV et SSE2. Elle reste temporaire et est accessible à l'adresse fournie dans le message de résultat. Le premier échange réel est stable ; après la réponse, le prompt du shell est rendu et accepte de nouvelles commandes.
