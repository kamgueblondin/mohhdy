# Plan du Lot 26 - Mohhdy OS

## Objectif
Extension Avancee du VFS Virtuel Multi-Secteurs, Moteur JS/DOM Event Loop Ring 3, Reseau P2P Raft Multi-Regions & Supervision Predictive Auto-Adaptative (US-033 / US-043 / US-062 / US-079 / US-119).

## Tableau des Taches et Statuts

| Tache | Description | Statut |
|---|---|---|
| 1. VFS Quotas, Droits d'Acces, Permutation Atomique & Verrouillage | Implementer le controle de capacite VFS (vfs_quota_set, vfs_quota_check), la gestion d'acces ACL (vfs_access_grant, vfs_access_control), l'echange atomique de fichiers (vfs_atomic_swap) et le verrouillage de fichiers (vfs_flock) dans userspace/ramfs.c, userspace/ramfs.h, kernel/fs/fat32.c et userspace/osui_runtime.c. | Termine |
| 2. Web Runtime, JS Event Loop & Bus Multi-Agents Async | Implementer la gestion d'evenements temporises DOM (webrt_timer_schedule, webrt_event_loop_tick), la verification KeyUsage/EKU X.509 (webrt_x509_verify_key_usage) et statut OCSP simule (webrt_ocsp_check_status) dans webrt.c/webrt.h. Etendre le pipeline PromptProgram avec le bus multi-agents (pm_bus_init, pm_bus_publish, pm_bus_subscribe, pm_bus_dequeue) dans promptmessage.c/promptmessage.h. | Termine |
| 3. Reseau P2P Mesh Multi-Regions, Raft Snapshot Streaming & Dynamic QoS | Implementer le streaming d'instantanes Raft decoupe par trames (raft_stream_snapshot_chunk), le routage multi-regions/clustering hierarchique (p2p_region_set), et le controle de debit BBR-like (p2p_qos_bbr_step) dans p2p.c/p2p.h. | Termine |
| 4. Diagnostic Production, KPI Metier & Auto-Healing | Etendre prod_perf_monitor_eval et prod_support_bundle_text pour l'auto-redemarrage de services (auto_restarts), implementer le calculateur KPI metier (prod_biz_kpi_eval), et renforcer la validation FNV-1a sur la memoire de persistance dans prod.c/prod.h/shell_prod.c/shell_persist.c. | Termine |
| 5. Documentation, Tests Unitaires & Alignement | Creation de docs/lot26_plan.md et mise a jour ASCII de docs/ETAT_REEL.md, docs/mohhdy_execution_and_roadmap.md et docs/todo.md. Verification de l'alignement du registre des commandes via extract_guest_commands.py --check. | Termine |
