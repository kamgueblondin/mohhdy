# Plan du Lot 25 - Mohhdy OS

## Objectif
Extension Majeure du VFS Multi-Repertoires, Moteur TLS/Handshake Ring 3, Bus Multi-Agents PromptProgram & Resilience P2P/Raft Avancee (US-032 / US-045 / US-061 / US-078 / US-118).

## Tableau des Taches et Statuts

| Tache | Description | Statut |
|---|---|---|
| 1. VFS Multi-Repertoires & Operations Arborescentes | Etendre le systeme VFS pour supporter la creation recursive de sous-repertoires (ramfs_mkdir_p), le listage recursif (ramfs_list_recursive), la copie de repertoires (ramfs_cp), le deplacement inter-repertoires (ramfs_mv) et la suppression recursive (ramfs_rm_r / rm -r) sous controle de capacite mutate. (US-032 / US-033) | Termine |
| 2. Web Runtime, TLS Ring 3 & Pipeline Multi-Agents | Implementer la verification de chaine de certificats X.509 multi-niveaux et l'etat de handshake TLS Ring 3 (webrt_tls_verify_cert_chain, webrt_tls_handshake_step), le gestionnaire d'evenements DOM et hooks asynchrones (webrt_dom_add_event_listener, webrt_dom_dispatch_event), et le chainage multi-agents PromptProgram avec transmission de contexte de session serialise (pm_context_*, pm_pipe_exec). (US-041 / US-045 / US-061) | Termine |
| 3. Reseau P2P Mesh, Reconciliations & Raft Avance | Implementer la reconciliation automatique de journal Raft post-partition reseau (p2p_raft_reconcile_partition), la compression d'en-tetes gRPC-Wire (grpcwire_compress_header), et l'ajustement dynamique du Lease de Leader sous pics de RTT EWMA / pertes de paquets (p2p_raft_update_lease). (US-070 / US-078) | Termine |
| 4. Diagnostic de Production & Auto-Correction | Implementer l'auto-correction automatique des workers stalles ou bloques (prod_autofix_stalled_workers), la consolidation des regles du modele de risque de maintenance predictive et le controle d'integrite FNV-1a. (US-118 / US-120) | Termine |
| 5. Documentation, Tests & Alignement des Commandes | Creation de docs/lot25_plan.md, mise a jour ASCII de docs/ETAT_REEL.md, docs/mohhdy_execution_and_roadmap.md et docs/todo.md. Verification de l'alignement du registre des commandes via extract_guest_commands.py --check. | Termine |
