# Harness de latence GGUF sous QEMU KVM (Multiboot)

## Objet

Le benchmark TCG ([aos2053_2060_gguf_qemu_latency_benchmark.md](aos2053_2060_gguf_qemu_latency_benchmark.md))
quantifie la latence GGUF locale sous `accel=tcg` (~48,7 s / ~22,8 s). La
tranche 3 du plan de suite exige une **campagne séparée** sur une plateforme de
référence matérielle ou **QEMU KVM**, sans réouvrir l'axe TCG.

Ce lot livre le **harness de mesure** guest Multiboot (`-kernel` / `-initrd`)
sous `accel=kvm`, le même contrat JSON que le benchmark TCG, des seuils
explicites, et un **skip documenté** lorsque `/dev/kvm` est absent ou
inaccessible (typique CI sans nested virt).

| Élément | Contrat |
|---|---|
| Scénario | Boot Multiboot, `ai-model use gpt2.gguf`, `ai bonjour`, puis `ai-continue`. |
| Horloge | Temps monotone hôte avant chaque commande Ring 3 ; boot et sélection exclus. |
| Accélérateur | `-machine type=pc,accel=kvm` uniquement (pas TCG). |
| Répétitions | `GGUF_BENCH_RUNS` (1..9, défaut 3). |
| Artefact | `test_logs/gguf-qemu-kvm-latency.json` (schéma v1 + bloc `thresholds`). |
| Skip | Code 0 + `GGUF_KVM_SKIP=1` si KVM inutilisable ou artefacts manquants. |
| Exigence stricte | `GGUF_KVM_REQUIRE=1` transforme le skip en échec (hôte de référence). |

## Seuils et critères d'acceptation

Référence TCG (ne pas confondre avec KVM) :

| Phase | Médiane TCG documentée |
|---|---:|
| Premier token `ai bonjour` | **48,739 s** |
| Continuation `ai-continue` | **22,781 s** |

Critères de sortie (PLAN tranche 3 / priorité 3 `mohhdy_us.md`) :

1. Campagne reproductible sous KVM (ou matériel), **séparée** des chiffres TCG.
2. Rapport min / médiane / max / dispersion, commandes isolées du boot.
3. **Ne pas** déclarer une latence inférieure à **1,0 s** sans campagne native ou KVM mesurée (`sub_second_claim_allowed` dans le JSON).
4. `make qemu-gguf-smoke` et `make test-all` restent verts ; pas de régression du chemin Q3_K réel.
5. En CI sans KVM : `make gguf-kvm-benchmark` doit **skipper** proprement (exit 0), tandis que `make gguf-kvm-benchmark-check` reste obligatoire et vert.

L'optimisation supplémentaire du runtime GGUF reste ouverte tant qu'aucune
campagne KVM avec poids déployés n'a produit un rapport JSON sur un hôte
référence.

## Utilisation

```sh
# Protocole + probe KVM + argv Multiboot (sans QEMU de génération).
make -s gguf-kvm-benchmark-check

# Campagne KVM (skip exit 0 si /dev/kvm inutilisable ou disque GGUF absent).
make -s gguf-kvm-benchmark

# Hôte de référence : échouer si KVM ou artefacts manquent.
GGUF_KVM_REQUIRE=1 GGUF_BENCH_RUNS=3 make -s gguf-kvm-benchmark
```

Prérequis campagne réelle : `models/gpt2-Q3_K_M.gguf` (hors Git),
`make gguf-disk`, noyau Multiboot construit, `/dev/kvm` accessible au
utilisateur (groupe `kvm` ou équivalent).

## Validation locale de ce lot

| Niveau | Vérification | Résultat attendu |
|---|---|---|
| Protocole | `make -s gguf-kvm-benchmark-check` | Réussi (synthèse, probe, argv). |
| Skip KVM | `make -s gguf-kvm-benchmark` sans droit `/dev/kvm` | `GGUF_KVM_SKIP=1`, exit 0. |
| Hygiène | `git diff --check` | Réussi. |

## Références

[1] [Benchmark TCG](../tests/scripts/benchmark_qemu_gguf_latency.py)

[2] [Benchmark KVM](../tests/scripts/benchmark_qemu_gguf_kvm_latency.py)

[3] [Contrôle protocole KVM](../tests/scripts/test_benchmark_qemu_gguf_kvm_latency.py)

[4] [Plan tranche 3](PLAN_SUITE_IMPLEMENTATION.md)

[5] [AOS-2053…2060 TCG](aos2053_2060_gguf_qemu_latency_benchmark.md)
