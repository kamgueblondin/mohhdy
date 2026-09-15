# AOS-2053 à AOS-2060 — Quantification de la latence GGUF locale sous QEMU

## Objet

Le runtime GPT-2 GGUF local sur FAT16 disposait d’un smoke fonctionnel qui affichait deux durées, sans protocole de répétition ni artefact structuré. Ce macro-lot livre un **benchmark QEMU TCG reproductible** qui mesure séparément le premier token et la continuation d’une session locale, puis produit une synthèse JSON exploitable.

| Élément | Contrat livré |
|---|---|
| Scénario mesuré | Boot, sélection `ai-model use gpt2.gguf`, premier token `ai bonjour`, puis `ai-continue`. |
| Horloge | Temps monotone hôte, démarrant immédiatement avant l’injection de chaque commande Ring 3. Le boot et la sélection sont exclus. |
| Répétitions | `GGUF_BENCH_RUNS`, borné de 1 à 9 ; la valeur par défaut est 3. |
| Synthèse | Minimum, médiane, maximum, dispersion absolue et dispersion relative pour chacune des deux phases. |
| Artefact | `test_logs/gguf-qemu-latency.json`, avec les échantillons, les logs associés et le schéma versionné. |
| Variance | `GGUF_BENCH_MAX_SPREAD_RATIO` peut publier une alerte observationnelle ; elle ne bloque pas une mesure par défaut. |

> Le benchmark quantifie le comportement sous **QEMU TCG**. Il ne prédit ni une latence matérielle, ni un SLA de production, ni un gain universel entre deux hôtes.

## Référence mesurée

La campagne locale a exécuté trois itérations sur le même profil GPT-2 Q3_K, le même volume FAT16 de déploiement et la même configuration QEMU i386/Pentium III. Les valeurs ci-dessous proviennent du rapport JSON produit par le benchmark.

| Mesure | Échantillons (s) | Médiane (s) | Minimum (s) | Maximum (s) | Dispersion |
|---|---:|---:|---:|---:|---:|
| Premier token `ai bonjour` | 48,739 ; 49,639 ; 46,182 | **48,739** | 46,182 | 49,639 | 3,457 s ; 7,09 % |
| Continuation `ai-continue` | 22,482 ; 22,781 ; 23,233 | **22,781** | 22,482 | 23,233 | 0,751 s ; 3,30 % |

La continuation médiane est d’environ **53,3 % plus courte** que le premier token médian dans cet environnement. Cette différence est compatible avec la réutilisation de la session GGUF locale ; elle reste une observation de cette campagne, pas une garantie de performance.

## Utilisation

```sh
# Vérifie la synthèse statistique, sans modèle ni QEMU.
make -s gguf-benchmark-check

# Lance trois répétitions (valeur par défaut) et écrit le rapport JSON.
make -s gguf-benchmark

# Personnalise le nombre de répétitions et demande une alerte de dispersion.
GGUF_BENCH_RUNS=5 GGUF_BENCH_MAX_SPREAD_RATIO=0.25 make -s gguf-benchmark
```

Le benchmark est délibérément indépendant de la CI obligatoire : une campagne de plusieurs générations GGUF est coûteuse, alors que `qemu-gguf-smoke` conserve son rôle de validation fonctionnelle rapide du modèle et de `ai-continue`.

## Validation

| Niveau | Vérification | Résultat |
|---|---|---|
| Protocole | `make -s gguf-benchmark-check` vérifie médiane, bornes, dispersion et rejet de l’échantillon vide. | Réussi. |
| Mesure réelle | `GGUF_BENCH_RUNS=3 GGUF_BENCH_MAX_SPREAD_RATIO=1.0 make -s gguf-benchmark`. | Réussi ; alerte de variance inactive. |
| Suite complète | `make -s test-all`. | 483/483 réussis. |
| Noyau i386 | `make -s kernel-only`. | Réussi. |
| Hygiène | `git diff --check`. | Réussi. |

## Références

[1] [Benchmark QEMU GGUF](../tests/scripts/benchmark_qemu_gguf_latency.py)

[2] [Contrôle du protocole statistique](../tests/scripts/test_benchmark_qemu_gguf_latency.py)

[3] [Smoke GGUF fonctionnel existant](../tests/scripts/ci_qemu_gguf_local_smoke.py)

[4] [Clôture de latence GGUF antérieure](aos1641_1648_gguf_latency_closure.md)
