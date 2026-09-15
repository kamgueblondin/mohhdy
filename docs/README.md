# Documentation MOHHDY

## Pour démarrer

Depuis la racine du dépôt : `make deps` (script [`scripts/bootstrap-dev.sh`](../scripts/bootstrap-dev.sh)), puis `make all` et `make test-all`. Détail des paquets et du GPT-2 optionnel : [../README.md](../README.md).

## Lire en premier

1. [ETAT_REEL.md](ETAT_REEL.md) - **état actuel du code**, y compris GPT-2 local et limites vérifiées
2. [BILAN_MASTER.md](BILAN_MASTER.md) - bilan de `origin/master` au 13 septembre 2026 (`9078d5f`)
3. [vocabulaire.md](vocabulaire.md) - termes du hobby OS (pas une identité Linux)
4. [../US/mohhdy_us.md](../US/mohhdy_us.md) - user stories du prototype (fait + suite, dont FAT16 mutate 8.3 et réseau local)
5. [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md) - prochaines tranches AOS 0-4, puis track Agent Support
6. [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md) - Agent Support (`ASSIST-xxx`) ; runtime HTTP `agent/`
7. [aos_fat_volume.md](aos_fat_volume.md) - volume FAT16 (lecture + create/remove/rename 8.3 racine) et FAT32 lecture VFS ; pas ext2
8. [mohhdy_foundation_increment_01_ipc.md](mohhdy_foundation_increment_01_ipc.md) - IPC Foundation MOHHDY, limites et contrat QEMU
9. [aos020_gguf_quantization_design.md](aos020_gguf_quantization_design.md) - sonde GGUF v3 et quantification préparatoire
10. [aos025_network_stub.md](aos025_network_stub.md) - stub OpenAI initial ; voir ETAT_REEL pour `ai-acquire`
11. [gpt2_baremetal_deployment.md](gpt2_baremetal_deployment.md) - préparation des poids et construction d'une ISO autonome
12. [GUIDE_EXECUTION.md](GUIDE_EXECUTION.md) - lancement QEMU (console / GUI / nographic / NE2000)
13. [../README.md](../README.md) - compilation, tests, architecture des sources

Les increments Foundation 02 a 64 restent dans le tableau ci-dessous ; ils decrivent des tranches deja livrees, pas l'identite du systeme. Le nombre de tests Unity cite dans un increment est le **constat a la livraison** de cette tranche ; le chiffre courant est dans [ETAT_REEL.md](ETAT_REEL.md) (**489**).

Les autres fichiers de ce dossier sont conservés : rapports de debug, chronologie des correctifs clavier, spécifications d'étapes. Beaucoup décrivent un état **intermédiaire** (shell simulé dans le kernel, crash timer, clavier mort). Ils ne sont pas effacés.

## Documents à jour (comportement courant)

| Fichier | Contenu |
|---|---|
| [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md) | Prochaines tranches : prototype AOS 0-4, puis track Agent Support |
| [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md) | Track Agent Support (`ASSIST-xxx`). Embed / sessions / admin / simulateur DOM / MCP demo / `/browser` / FS sandbox livrés (stub local, Playwright optionnel, pas US-031). Packaging 051-053 : scaffold |
| [assist050_docker_runtime.md](assist050_docker_runtime.md) | Image Docker / HTTP agent : build, `docker run`, fumée |
| [assist051_052_053_deploy.md](assist051_052_053_deploy.md) | Install PC, hyperviseur QEMU/cloud-init, mode hosted non-billing |
| [assist060_061_browser.md](assist060_061_browser.md) | Vue `/browser` et FS sandbox (ASSIST-060/061, pas US-031) |
| [assist_playwright_optional.md](assist_playwright_optional.md) | Profil Playwright / Chromium optionnel (pas US-031, hors slim) |
| [assist010_sessions_admin.md](assist010_sessions_admin.md) | Sessions visiteur, embed, admin `ADMIN_TOKEN`, snippet CSP |
| [assist013_origine_embed.md](assist013_origine_embed.md) | Refus d'origine document vs site déclaré (ASSIST-013) |
| [assist012_droits_handoff.md](assist012_droits_handoff.md) | KB locale, droits, escalade, handoff |
| [assist020_gestes_mcp.md](assist020_gestes_mcp.md) | Gestes simulateur, outils MCP, facture mock |
| [ETAT_REEL.md](ETAT_REEL.md) | État fonctionnel, GPT-2 local et limites vérifiées |
| [vocabulaire.md](vocabulaire.md) | Lexique du hobby OS ; distance volontaire d'une identité Linux |
| [../US/mohhdy_us.md](../US/mohhdy_us.md) | Backlog du prototype, AOS-001...026, FAT16 mutate, sockets, `ai-acquire` et TLS/HTTP local |
| [aos_fat_volume.md](aos_fat_volume.md) | FAT16 lecture + mutations 8.3 racine ; FAT32 lecture VFS ; LFN FAT16 hors perimetre |
| [aos025_network_stub.md](aos025_network_stub.md) | Stub OpenAI initial ; voir ETAT_REEL pour `ai-acquire` et TLS/HTTP local |
| [aos132_net_status_dynamic.md](aos132_net_status_dynamic.md) | `SYS_NET_STATUS` et smoke `qemu-ne2k-status` |
| [aos153_154_tcp_sequence_retransmit.md](aos153_154_tcp_sequence_retransmit.md) | Dernière tranche TCP caller-owned (séquence et retransmission bornée) |
| [mohhdy_foundation_increment_01_ipc.md](mohhdy_foundation_increment_01_ipc.md) | IPC Foundation entre tâches Ring 3, limites de la tranche et contrat QEMU |
| [mohhdy_foundation_increment_02_vfs_service.md](mohhdy_foundation_increment_02_vfs_service.md) | Médiateur VFS Ring 3, protocole de lecture et limites du backend noyau |
| [mohhdy_foundation_increment_03_service_registry.md](mohhdy_foundation_increment_03_service_registry.md) | Registre nommé, découverte `vfs` et absence de capabilities |
| [mohhdy_foundation_increment_04_service_lifecycle.md](mohhdy_foundation_increment_04_service_lifecycle.md) | Retrait propriétaire et nettoyage du registre à la terminaison |
| [mohhdy_foundation_increment_05_ipc_correlation.md](mohhdy_foundation_increment_05_ipc_correlation.md) | Identifiant de corrélation IPC, filtre VFS et limites non bloquantes |
| [mohhdy_foundation_increment_06_service_grant.md](mohhdy_foundation_increment_06_service_grant.md) | Transfert de propriété d'un nom, handoff et limites sans capabilities |
| [aos020_gguf_quantization_design.md](aos020_gguf_quantization_design.md) | Sonde GGUF v3, primitives Q8_0/K-quants et limites de l'inférence quantifiée |
| [gpt2_baremetal_deployment.md](gpt2_baremetal_deployment.md) | Préparation des artefacts, construction et démarrage d'une ISO GPT-2 hors ligne |
| [kv_cache_performance_report.md](kv_cache_performance_report.md) | Cache KV, SSE2, test de reprise du shell et mesures de latence |
| [baremetal_llm_architecture.md](baremetal_llm_architecture.md) | Architecture de référence et évolutions envisagées pour un LLM bare-metal |
| [GUIDE_EXECUTION.md](GUIDE_EXECUTION.md) | Modes QEMU et dépannage clavier |
| [CHANGELOG_v6.1.md](CHANGELOG_v6.1.md) | Notes de version 6.1 + correctif EOI |
| [todo.md](todo.md) | Suivi des tâches (historique + reste à faire) |
| [etape_7_shell_ia.md](etape_7_shell_ia.md) | Étape shell et ancien simulateur IA ; lecture historique |
| [analyse_logique_docs.md](analyse_logique_docs.md) | Logique des étapes 1-7 |
| [etapes_3_4_specifications.md](etapes_3_4_specifications.md) | Specs mémoire / initrd |
| [etapes_5_6_implementation.md](etapes_5_6_implementation.md) | Multitâche / userspace |
| [MOHHDY_v5_NOUVELLES_FONCTIONNALITES.md](MOHHDY_v5_NOUVELLES_FONCTIONNALITES.md) | Fonctionnalités v5 |

## Diagnostics et correctifs (historiques, contenu technique conservé)

Utile pour comprendre *pourquoi* un correctif a été tenté. La conclusion "toujours cassé" peut être fausse aujourd'hui.

### Architecture et userspace

- [ANALYSE_ARCHITECTURE.md](ANALYSE_ARCHITECTURE.md)
- [ANALYSE_PROBLEMES.md](ANALYSE_PROBLEMES.md)
- [ANALYSE_PROBLEMES_SHELL.md](ANALYSE_PROBLEMES_SHELL.md)
- [diagnostic_problemes.md](diagnostic_problemes.md)
- [DIAGNOSTIC_SHELL_IA_PROBLEMES.md](DIAGNOSTIC_SHELL_IA_PROBLEMES.md)
- [DIAGNOSTIC_COMPLET.md](DIAGNOSTIC_COMPLET.md)
- [CORRECTION_SHELL_V5.md](CORRECTION_SHELL_V5.md)
- [CORRECTION_STABILITE.md](CORRECTION_STABILITE.md)

### Clavier PS/2 (plusieurs itérations)

- [CORRECTION_CLAVIER_FINALE.md](CORRECTION_CLAVIER_FINALE.md)
- [CORRECTION_CLAVIER_FINALE_V2.md](CORRECTION_CLAVIER_FINALE_V2.md)
- [CORRECTION_CLAVIER_ULTIME.md](CORRECTION_CLAVIER_ULTIME.md)
- [CORRECTION_CLAVIER_DEFINITIVE_v7.md](CORRECTION_CLAVIER_DEFINITIVE_v7.md)
- [CORRECTION_DEFINITIVE_CLAVIER_FINALE.md](CORRECTION_DEFINITIVE_CLAVIER_FINALE.md)
- [CORRECTION_AFFICHAGE_CLAVIER_V6.md](CORRECTION_AFFICHAGE_CLAVIER_V6.md)
- [CORRECTIONS_CLAVIER.md](CORRECTIONS_CLAVIER.md)
- [KEYBOARD_STABLE_FIX.md](KEYBOARD_STABLE_FIX.md)
- [KEYBOARD_ULTIMATE_FIX.md](KEYBOARD_ULTIMATE_FIX.md)
- [DIAGNOSTIC_CLAVIER_V3.md](DIAGNOSTIC_CLAVIER_V3.md)
- [DIAGNOSTIC_FINAL_CLAVIER_RESOLU.md](DIAGNOSTIC_FINAL_CLAVIER_RESOLU.md)
- [RAPPORT_CORRECTION_CLAVIER_FINALE.md](RAPPORT_CORRECTION_CLAVIER_FINALE.md)

Le blocage restant (EOI IRQ0 après `schedule()` / PIC qui masque IRQ1) est documenté dans [ETAT_REEL.md](ETAT_REEL.md), pas dans ces itérations.

### Rapports de campagne de tests / implémentation

- [RAPPORT_IMPLEMENTATION.md](RAPPORT_IMPLEMENTATION.md)
- [RAPPORT_CORRECTION_FINALE.md](RAPPORT_CORRECTION_FINALE.md)
- [RAPPORT_DIAGNOSTIC_APPROFONDI.md](RAPPORT_DIAGNOSTIC_APPROFONDI.md)
- [RAPPORT_TEST_FINAL_v7.md](RAPPORT_TEST_FINAL_v7.md)
- [RAPPORT_FINAL_V2.md](RAPPORT_FINAL_V2.md)
- [RAPPORT_FINAL_V4.md](RAPPORT_FINAL_V4.md)
- [rapport_tests_shell_ia.md](rapport_tests_shell_ia.md)
- [rapport_stabilite_modules.md](rapport_stabilite_modules.md)

Les captures QEMU et les exports Word ont été retirés du dépôt (la source reste les fichiers `.md`). Les scripts `tests/scripts/test_gpt2_shell_recovery.py` et `tests/scripts/benchmark_gpt2_kv_latency.py` sont les contrôles QEMU correspondants ; ils requièrent les poids locaux sous `models/`.

## User stories

- [../US/mohhdy_us.md](../US/mohhdy_us.md) - backlog du **prototype** (AOS-001...026 livres ; FAT16 mutate 8.3 ; `ai-acquire` local, pas client OpenAI public)
- [../US/README.md](../US/README.md) - trois couches : hobby OS, vision MOHHDY, track Agent Support
- [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md) - spec Agent Support (`ASSIST-xxx`) ; runtime HTTP `agent/`
- [assist050_docker_runtime.md](assist050_docker_runtime.md) - `docker run` et fumée de l'image agent
- [assist060_061_browser.md](assist060_061_browser.md) - vue navigateur d'instance et FS sandbox
- [assist010_sessions_admin.md](assist010_sessions_admin.md) - sessions, embed, admin, snippet CSP
- [assist013_origine_embed.md](assist013_origine_embed.md) - origine document vs site déclaré
- [../US/individual_us/INDEX.md](../US/individual_us/INDEX.md) - specs MOHHDY, chevauchements, IDs dupliqués

Les phases MOHHDY restent majoritairement des **specifications**. Le track Agent Support est une piste produit parallèle, pas une fonction du prototype i386. Les increments Foundation 01-64 (IPC, mediateur de chemins, registre, supervision de taches) sont compiles et testes ; ils ne transforment pas le noyau monolithique en microkernel et n'implementent pas les autres phases. FAT16 et FAT32 8.3 racine sont mutables via VFS ; le client OpenAI public reste hors perimetre.
