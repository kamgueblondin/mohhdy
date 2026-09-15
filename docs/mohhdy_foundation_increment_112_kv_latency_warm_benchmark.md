# MOHHDY Foundation — Incrément 112 : benchmark latence froide et chaude

**État :** implémenté et vérifié syntaxiquement.

## Objectif

Le benchmark `tests/scripts/benchmark_gpt2_kv_latency.py` mesure désormais deux exécutions successives du même prompt : la latence froide, qui initialise le chemin de génération, puis la latence chaude, qui réutilise le cache KV de la session.

## Sortie

Le script émet `COLD_LATENCY_SECONDS`, `WARM_LATENCY_SECONDS` et `KV_REUSE=1`. Il conserve également la première ligne de réponse locale GPT-2 afin de vérifier que la mesure correspond bien à une génération réelle et non à un simple boot QEMU.

## Portée

Cette évolution ne prétend pas atteindre l’objectif inférieur à une seconde sous QEMU TCG. Elle rend la comparaison reproductible et sépare explicitement le coût d’amorçage du coût d’une requête chaude. Le benchmark complet nécessite les assets GPT-2 locaux et un environnement QEMU disponible.

## Validation

La syntaxe a été vérifiée par `python3 -m py_compile tests/scripts/benchmark_gpt2_kv_latency.py`, puis `git diff --check` a passé sans erreur.
