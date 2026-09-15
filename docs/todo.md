# TODO - Correction MOHHDY Shell Utilisateur

> **État réel et pilotage (26 août 2026).** Le shell utilisateur Ring 3 **se lance** et le clavier **répond** (correctif EOI IRQ0). Le périmètre courant validé, les limites et les prochaines tranches priorisées sont dans [ETAT_REEL.md](ETAT_REEL.md) ; la suite compte **505/505** tests et les sept contrats QEMU terminent localement en **23 min 30 s**. Les phases et incréments ci-dessous restent un **journal de livraison** : leurs compteurs et limites décrivent l’état de leur date, sauf lorsqu’une note les actualise explicitement.

## Phase 1: Récupération et configuration du projet ✅
- [x] Cloner le projet depuis GitHub
- [x] Examiner la structure du projet
- [x] Analyser le Makefile
- [x] Identifier les fichiers sources principaux

## Phase 2: Analyse du code existant ✅
- [x] Analyser le code du kernel principal
- [x] Examiner le code du shell userspace
- [x] Comprendre la gestion des tâches et processus
- [x] Analyser les appels système
- [x] Examiner les logs de debug existants

## Phase 3: Identification des problèmes ✅
- [x] Identifier pourquoi l'interface reste figée en mode utilisateur
- [x] Analyser les problèmes de gestion des processus
- [x] Vérifier les appels système et la communication kernel/userspace
- [x] Identifier les problèmes de redémarrage/crash

### Problèmes identifiés:
1. **Transition Kernel->Userspace manquante**: Le système reste en mode simulation au lieu de passer au shell utilisateur
2. **Gestion des interruptions clavier**: sys_gets() peut bloquer indéfiniment avec hlt
3. **Context switch incomplet**: Pas de switch_to_userspace() implémenté
4. **Configuration timer**: Conflits d'interruptions lors de la transition

## Phase 4: Tests et débogage avec make run ✅
- [x] Compiler et tester le système actuel
- [x] Analyser les logs en temps réel
- [x] Identifier les points de blocage
- [x] Tester différentes configurations

### Résultats des tests:
- ✅ **Compilation réussie** après installation de nasm, gcc-multilib et qemu
- ✅ **Système stable** - Plus de redémarrage en boucle
- ✅ **Timer fonctionnel** - Ticks réguliers à 100Hz
- ❌ **Mode simulation seulement** - Le système reste en mode kernel au lieu de passer au shell utilisateur *(constat d'alors)*
- 📝 **Point de blocage identifié**: Le code reste dans la boucle de simulation au lieu d'exécuter le shell utilisateur

*Mise à jour août 2026 :* le shell ELF userspace est lancé ; le "mode simulation seulement" ne s'applique plus. Voir phase 5.

## Phase 5: Correction et refactorisation ✅ (août 2026)
- [x] Corriger les problèmes identifiés (transition userspace via `jump_to_task`, EOI PIC avant `schedule()`)
- [x] Refactoriser le code si nécessaire (planification uniquement sur `g_reschedule_needed`)
- [x] Améliorer la stabilité du système (plus de reboot systématique après le timer)
- [x] Optimiser la communication kernel/userspace (syscalls GETS/EXEC fonctionnels)

### Reste ouvert (hors périmètre "shell qui démarre")
- [x] Commandes listées dans `help` branchées dans `execute_builtin_command` (overlay noyau + initrd ; `procsim.c` n'alimente plus `ps`/`kill`)
- [x] `ls` / `cat` / `ps` / `kill` / `uptime` / `mem` : syscalls noyau (initrd + overlay + `task.c` + PIT + PMM)
- [x] Overlay noyau RAM : `mkdir` / `rm` / `cp` (fichier et dossier via `SYS_COPY`) / `mv` (fichier et dossier via `SYS_RENAME`) / `write` / `append` (`SYS_APPEND`) / `touch` / `echo >` visibles par `ls`/`cat` (initrd toujours read-only)
- [x] Overlay persisté : snapshot AIOV V2 ATA PIO LBA28 sur disque IDE QEMU (`write` survit à un reboot) ; volume FAT16 lecture seule à partir du LBA 64
- [x] `spawn` / `yield` coopératifs (cadre syscall user) et préemption IRQ0 sûre entre tâches Ring 3
- [x] `exec` bloquant : parent `TASK_WAITING`, enfant reveille via `SYS_EXIT` (plus de `int $0x30` noyau)
- [x] AOS-020…026 : sonde GGUF, BPE UTF-8, contrats QEMU, overlay V2, IRQ0, stub OpenAI, FAT16 lecture seule
- [x] Kernels GGUF Q3_K/Q4_K/Q6_K, index, mapping, génération shell et échantillonnage local FAT16
- [x] Réduire la latence locale GGUF ; le forward Q3_K réel sans branche, le cache KV, les lectures FAT16 inter-clusters et la session locale coopérative sont validés. Les essais supplémentaires de cache paresseux de constantes et de spécialisation Q3_K top-k n’ayant pas dépassé la variabilité QEMU TCG, l’axe est clôturé avec mesures et critères dans [aos1641_1648_gguf_latency_closure.md](aos1641_1648_gguf_latency_closure.md).
- [x] FAT16/FAT32 via VFS : LFN à la racine et un sous-répertoire 8.3 avec `mkdir`, lecture, statut, pagination, écriture, renommage, suppression et `rmdir` vide sous capacité `mutate`, sans écrasement ni rejeu local après mutation incertaine — [aos_fat_volume.md](aos_fat_volume.md) ; second niveau, LFN enfant, renommage inter-répertoire et remplacement atomique restent ouverts ; pas ext2
- [x] Pilote NE2000 ISA et codecs ARP/IPv4/UDP/DHCP/DNS/TCP/TLS record (lots 113–154)
- [x] Validation page-par-page des pointeurs socket utilisateur dans le VMM
- [x] Reprise SSE fine avec `Last-Event-ID`, TLS/HTTP/SSE et fermeture `close_notify` authentifiés sur pair QEMU local contrôlé ; toute campagne OpenAI réelle reste suspendue jusqu’à un accord explicite et une clé API valide hors CI
- [x] AOS-1345…1352 : exposition TLS caller-owned par le registre socket (`send_tls`/`receive_tls`), sans allocation dynamique — [aos1345_1352_socket_tls_adapter.md](aos1345_1352_socket_tls_adapter.md)
- [x] AOS-1353…1364 : construction LLM Ollama/OpenAI sur socket TLS et pont de segment TCP vers TX NE2000 ; polling HTTP/SSE socket et orchestration complète encore ouverts — [aos1353_1364_llm_socket_ne2k_bridge.md](aos1353_1364_llm_socket_ne2k_bridge.md)
- [x] AOS-1365…1372 : renouvellement DHCP live caller-owned avec REQUEST `ciaddr`, ACK borné et publication transactionnelle ; planification périodique et réacquisition après expiration encore ouvertes — [aos1365_1372_dhcp_live_renewal.md](aos1365_1372_dhcp_live_renewal.md)
- [x] AOS-1373…1384 : réception HTTP et SSE LLM sur socket TLS avec rollback TCP/TLS/accumulateur ; l’orchestrateur de bout en bout reste ouvert — [aos1373_1384_llm_socket_http_sse.md](aos1373_1384_llm_socket_http_sse.md)
- [x] AOS-1385…1392 : injection de réception TCP NE2000 dans le registre socket statique ; orchestration active et commande `ai` encore ouvertes — [aos1385_1392_ne2k_socket_rx_bridge.md](aos1385_1392_ne2k_socket_rx_bridge.md)
- [x] AOS-1393…1400 : SYN actif construit par socket et émis via NE2000 ; handshake TLS et orchestration applicative encore ouverts — [aos1393_1400_socket_active_syn.md](aos1393_1400_socket_active_syn.md)
- [x] AOS-1401…1412 : ClientHello TLS après SYN-ACK via socket et NE2000, avec rollback socket/TLS — [aos1401_1412_socket_tls_clienthello.md](aos1401_1412_socket_tls_clienthello.md)
- [x] AOS-1413…1424 : polling TLS authentifié sur socket, ACK NE2000, validation X.509 directe/chaînée, flight X25519 et post-flight transactionnels — [aos1413_1424_socket_tls_poll.md](aos1413_1424_socket_tls_poll.md)
- [x] AOS-1425…1436 : émission LLM et polling HTTP/SSE actifs sur socket TLS via NE2000, avec rollback TCP/TLS/applicatif — [aos1425_1436_socket_llm_orchestrator.md](aos1425_1436_socket_llm_orchestrator.md)
- [x] AOS-1437…1448 : session LLM socket de SYN-ACK à HTTP/SSE, avec phases transactionnelles et réarmement TLS — [aos1437_1448_llm_socket_session.md](aos1437_1448_llm_socket_session.md)
- [x] AOS-1449…1460 : bootstrap DHCP/DNS/ARP/SYN transactionnel vers session LLM socket et libération du slot en rollback — [aos1449_1460_socket_bootstrap.md](aos1449_1460_socket_bootstrap.md)
- [x] AOS-1461…1472 : orchestrateur noyau DHCP→socket→TLS→HTTP/SSE, extraction LLM et FIN+ACK transactionnel — [aos1461_1472_kernel_socket_orchestrator.md](aos1461_1472_kernel_socket_orchestrator.md)
- [x] AOS-1473…1480 : maintenance DHCP différée hors IRQ0 et renouvellement transactionnel à l’entrée syscall — [aos1473_1480_dhcp_deferred_maintenance.md](aos1473_1480_dhcp_deferred_maintenance.md)
- [x] AOS-1481…1488 : réacquisition DHCP après expiration, fermeture socket et bootstrap transactionnel relancé hors IRQ0 — [aos1481_1488_dhcp_reacquisition.md](aos1481_1488_dhcp_reacquisition.md)
- [x] AOS-1489…1496 : backoff exponentiel borné de réacquisition DHCP, plafond de tentatives et remise à zéro transactionnelle — [aos1489_1496_dhcp_reacquire_backoff.md](aos1489_1496_dhcp_reacquire_backoff.md)
- [x] AOS-1497…1504 : conservation contrôlée de la configuration fournisseur lors d’une réacquisition automatique — [aos1497_1504_provider_recovery.md](aos1497_1504_provider_recovery.md)
- [x] AOS-1505…1512 : reprise applicative HTTP/SSE après réacquisition réseau et TLS complet — [aos1505_1512_application_recovery.md](aos1505_1512_application_recovery.md)
- [x] AOS-1513…1520 : reprise SSE fine sur `Last-Event-ID` après réacquisition réseau, TLS complet et rollback TCP/TLS — [aos1513_1520_sse_fine_resume.md](aos1513_1520_sse_fine_resume.md)
- [x] AOS-1521…1528 : activation contrôlée OpenAI depuis le shell, bearer masqué dans l’historique et diagnostics réseau réalignés — [aos1521_1528_openai_shell_activation.md](aos1521_1528_openai_shell_activation.md)
- [x] Réseau, TLS 1.2, HTTP/SSE et intégration OpenAI complète : support du bootstrap live DHCP, DNS A, TCP, TLS authentifié, POST HTTP JSON, flux SSE et credential OpenAI sécurisé hors boot image
- [x] AOS-1529…1536 : workspace GPT-2 statique, bornes 124M, suppression de `kmalloc` et test de capacité — [aos1529_1536_static_gpt2_workspace.md](aos1529_1536_static_gpt2_workspace.md)
- [x] AOS-1537…1544 : lecture GGUF dense F32/F16 et tête de logits Q3_K/Q4_K/Q6_K sur FAT16 caller-owned — [aos1537_1544_gguf_output_path.md](aos1537_1544_gguf_output_path.md)
- [x] AOS-1545…1552 : forward de bloc transformeur GPT-2 Q4_K sur FAT16, cache KV et workspace caller-owned — [aos1545_1552_gguf_transformer_block.md](aos1545_1552_gguf_transformer_block.md)
- [x] AOS-1553…1560 : préparation statique de génération GPT-2 GGUF, rôles globaux, couches contiguës et dimensions déduites — [aos1553_1560_gguf_generation_prepare.md](aos1553_1560_gguf_generation_prepare.md)
- [x] AOS-1561…1568 : exécution GPT-2 GGUF token-vers-logits sur FAT16, cache KV et workspace caller-owned — [aos1561_1568_gguf_token_logits.md](aos1561_1568_gguf_token_logits.md)
- [x] AOS-1569…1576 : runtime GPT-2 GGUF local, disque FAT16 de déploiement, shell, top-k et lectures séquentielles — [aos1569_1576_gguf_local_shell.md](aos1569_1576_gguf_local_shell.md)
- [x] AOS-1577…1584 : cache de curseur FAT16, pré-calcul Q4_K/Q6_K et smoke local chronométré — [aos1577_1584_gguf_fat16_latency.md](aos1577_1584_gguf_fat16_latency.md)
- [x] AOS-1585…1592 : projection GGUF top-k en flux, suppression du buffer de logits et équivalence RNG — [aos1585_1592_gguf_stream_topk.md](aos1585_1592_gguf_stream_topk.md)
- [x] AOS-1593…1600 : session GPT-2 GGUF locale persistante, continuation coopérative `ai-continue`, ABI 110 et refus sans session — [aos1593_1600_gguf_local_session.md](aos1593_1600_gguf_local_session.md)
- [x] AOS-1601…1608 : forward GGUF Q3_K réel, buffer MLP 4C, garde FAT16 profonde corrigée et smoke `ai`/`ai-continue` sans repli — [aos1601_1608_gguf_real_runtime.md](aos1601_1608_gguf_real_runtime.md)
- [x] AOS-1609…1616 : transferts ATA PIO `rep insw`/`rep outsw`, gain mesuré du premier token Q3_K et smoke QEMU sans régression — [aos1609_1616_ata_pio_streaming.md](aos1609_1616_ata_pio_streaming.md)
- [x] AOS-1617…1624 : fenêtre FAT16 caller-owned de 4 Kio, lecture ATA multi-secteurs et accélération QEMU du forward/continuation GGUF — [aos1617_1624_fat16_read_window.md](aos1617_1624_fat16_read_window.md)
- [x] AOS-1625…1632 : fenêtre FAT16 inter-clusters de 8 Kio, cache FAT isolé, invalidation après écriture et gain QEMU supplémentaire — [aos1625_1632_fat16_intercluster_window.md](aos1625_1632_fat16_intercluster_window.md)
- [x] AOS-1633…1640 : décodage Q3_K sans branche, équivalence quantifiée et gain QEMU de projection — [aos1633_1640_q3k_branchless_decode.md](aos1633_1640_q3k_branchless_decode.md)
- [x] AOS-1641…1648 : clôture mesurée de l’axe de latence GGUF, essais non concluants retirés et bascule vers les fonctionnalités FAT16/LFN — [aos1641_1648_gguf_latency_closure.md](aos1641_1648_gguf_latency_closure.md)
- [x] AOS-1649…1656 : recherche FAT16 par LFN ASCII validé pour lecture totale, lecture à offset et curseur, sans allocation dynamique — [aos1649_1656_fat16_lfn_read_lookup.md](aos1649_1656_fat16_lfn_read_lookup.md)
- [x] AOS-1657…1664 : suppression FAT32 par alias 8.3 ou LFN ASCII validé, marquage de séquence et libération de chaîne bornée — [aos1657_1664_fat32_lfn_unlink.md](aos1657_1664_fat32_lfn_unlink.md)
- [x] AOS-1665…1672 : montage VFS protégé `fat16/` en lecture/stat/listage, sans mutation ni allocation dynamique — [aos1665_1672_vfs_fat16_readonly_mount.md](aos1665_1672_vfs_fat16_readonly_mount.md)
- [x] AOS-1673…1680 : renommage FAT32 LFN validé sans déplacement de chaîne, borné à une cardinalité de séquence identique — [aos1673_1680_fat32_lfn_rename.md](aos1673_1680_fat32_lfn_rename.md)
- [x] AOS-1681…1688 : lecture FAT32 bornée par alias 8.3, parcours de chaîne contrôlé et prérequis VFS — [aos1681_1688_fat32_alias_read.md](aos1681_1688_fat32_alias_read.md)
- [x] AOS-1689…1696 : sélection ATA primaire maître/esclave, prérequis d’un volume FAT32 IDE distinct — [aos1689_1696_ata_multidrive.md](aos1689_1696_ata_multidrive.md)
- [x] AOS-1697…1704 : volume FAT32 secondaire statique au noyau, montage ATA esclave non bloquant — [aos1697_1704_fat32_secondary_kernel_mount.md](aos1697_1704_fat32_secondary_kernel_mount.md)
- [x] AOS-1705…1712 : image FAT32 ATA esclave reproductible et smoke QEMU multi-disque de montage réel — [aos1705_1712_fat32_secondary_image_smoke.md](aos1705_1712_fat32_secondary_image_smoke.md)
- [x] AOS-1713…1720 : syscalls FAT32 Ring 3 de lecture et listage de racine, sans mutation ni allocation dynamique — [aos1713_1720_fat32_read_syscalls.md](aos1713_1720_fat32_read_syscalls.md)
- [x] AOS-1721…1728 : montage VFS protégé `fat32/` en lecture/stat/listage, sans mutation ni allocation dynamique — [aos1721_1728_vfs_fat32_readonly_mount.md](aos1721_1728_vfs_fat32_readonly_mount.md)
- [x] AOS-1729…1736 : lecture FAT32 par alias ou LFN ASCII validé, ordinaux/checksum contrôlés et parcours de chaîne borné — [aos1729_1736_fat32_lfn_read.md](aos1729_1736_fat32_lfn_read.md)
- [x] AOS-1737…1744 : LFN FAT16/FAT32 UTF-8 BMP, sérialisation UTF-16LE, recherche/lecture/listage Unicode et zéro allocation dynamique — [aos1737_1744_fat_lfn_utf8.md](aos1737_1744_fat_lfn_utf8.md)
- [x] AOS-1745…1752 : LFN FAT16/FAT32 Unicode hors BMP par paires de surrogates UTF-16LE, lecture/listage UTF-8 complet et zéro allocation dynamique — [aos1745_1752_fat_lfn_unicode_full.md](aos1745_1752_fat_lfn_unicode_full.md)
- [x] AOS-1753…1760 : sélection transactionnelle de chaînes TLS bornées avec intermédiaires X.509, permutations contrôlées et zéro allocation dynamique — [aos1753_1760_tls_chain_selection.md](aos1753_1760_tls_chain_selection.md)
- [x] AOS-1761…1768 : ClientHello TLS 1.2 avec extensions SNI/ALPN ASCII bornées, compatibilité historique et zéro allocation dynamique — [aos1761_1768_tls_sni_alpn.md](aos1761_1768_tls_sni_alpn.md)
- [x] AOS-1769…1776 : contexte NE2000 de reprise SSE inter-session, checkpoint intègre, restauration transactionnelle et effacement explicite sans allocation dynamique — [aos1769_1776_sse_session_context.md](aos1769_1776_sse_session_context.md)
- [x] AOS-1777…1784 : tick SSE NE2000, programmation automatique de retry, attente non bloquante et reprise déléguée sans allocation dynamique — [aos1777_1784_ne2k_sse_event_tick.md](aos1777_1784_ne2k_sse_event_tick.md)
- [x] AOS-1785…1792 : tick SSE actif raccordé au contexte LLM, checkpoint automatique, publication transactionnelle et reprise persistante sans allocation dynamique — [aos1785_1792_sse_context_event_tick.md](aos1785_1792_sse_context_event_tick.md)
- [x] AOS-1793…1800 : rotation SSE Ollama/OpenAI après budget atteint, reset du scheduler, checkpoint sans reprise inter-fournisseur et zéro allocation dynamique — [aos1793_1800_sse_provider_rotation_context.md](aos1793_1800_sse_provider_rotation_context.md)
- [x] AOS-1801…1808 : jitter SSE borné dans le contexte LLM, publication transactionnelle de la graine et checkpoint persistant sans allocation dynamique — [aos1801_1808_sse_context_jitter.md](aos1801_1808_sse_context_jitter.md)
- [x] AOS-1809…1816 : renouvellement DHCP transactionnel attaché au contexte LLM, conservation du bail sur erreur et zéro allocation dynamique — [aos1809_1816_dhcp_context_renewal.md](aos1809_1816_dhcp_context_renewal.md)
- [x] AOS-1817…1824 : réconciliation DHCP du contexte LLM, purge transactionnelle du transport après changement IPv4 et conservation du checkpoint SSE — [aos1817_1824_dhcp_context_reconcile.md](aos1817_1824_dhcp_context_reconcile.md)
- [x] AOS-1825…1832 : décision SSE de reprise ou flux neuf dans le contexte LLM, avec gardes de phase et zéro allocation dynamique — [aos1825_1832_sse_resume_decision.md](aos1825_1832_sse_resume_decision.md)
- [x] AOS-1833…1840 : alerte TLS `close_notify` AES-GCM, type Alert et avancement transactionnel de séquence sans allocation dynamique — [aos1833_1840_tls_close_notify.md](aos1833_1840_tls_close_notify.md)
- [x] AOS-1841…1848 : émission NE2000 de `close_notify` TLS, rollback avant commit et séparation explicite du FIN TCP sans allocation dynamique — [aos1841_1848_ne2k_tls_close_notify.md](aos1841_1848_ne2k_tls_close_notify.md)
- [x] AOS-1849…1856 : annulation LLM avec `close_notify` avant FIN best-effort, snapshot socket/TLS et purge locale conservée sans allocation dynamique — [aos1849_1856_llm_cancel_tls_close.md](aos1849_1856_llm_cancel_tls_close.md)
- [x] AOS-1857…1864 : parsing strict de `close_notify` TLS distant, validation sans mutation et zéro allocation dynamique — [aos1857_1864_tls_peer_close_notify.md](aos1857_1864_tls_peer_close_notify.md)
- [x] AOS-1865…1872 : détection de `close_notify` TLS distant dans les pollers HTTP/SSE, ACK cohérent, transition LLM vers `RESPONSE_READY` et zéro allocation dynamique — [aos1865_1872_tls_peer_close_poller.md](aos1865_1872_tls_peer_close_poller.md)
- [x] AOS-1873…1880 : réponse TLS `close_notify` best-effort après fermeture distante, transition SSE terminale et zéro allocation dynamique — [aos1873_1880_tls_peer_close_reply.md](aos1873_1880_tls_peer_close_reply.md)
- [x] AOS-1881…1888 : isolation des tables VMM utilisateur, rollback de création Ring 3 et restitution PMM sans destruction des mappings noyau partagés — [aos1881_1888_user_vmm_isolation.md](aos1881_1888_user_vmm_isolation.md)
- [x] AOS-1889…1896 : réclamation différée des tâches Ring 3 terminées, destruction VMM hors pile active et libération de pile noyau sans allocation dynamique — [aos1889_1896_task_deferred_reaper.md](aos1889_1896_task_deferred_reaper.md)
- [x] AOS-1897…1904 : réclamation forcée des tâches Ring 3 détachées par `kill`, destruction VMM hors contexte actif et zéro allocation dynamique — [aos1897_1904_task_forced_reap.md](aos1897_1904_task_forced_reap.md)
- [x] AOS-1905…1912 : contrat public de retrait de tâche hors contexte actif, vérification de liste et réclamation Ring 3 sans allocation dynamique — [aos1905_1912_task_public_remove.md](aos1905_1912_task_public_remove.md)
- [x] AOS-1913…1924 : pools statiques bornés de tâches Ring 3, piles noyau et conteneurs VMM, rollback et zéro allocation de tas dans `task.c` — [aos1913_1924_static_task_vmm_pools.md](aos1913_1924_static_task_vmm_pools.md)
- [x] AOS-1925…1932 : tables VMM créées par le PMM, alignement matériel cohérent et zéro allocation de tas dans `vmm.c` — [aos1925_1932_vmm_pmm_tables.md](aos1925_1932_vmm_pmm_tables.md)
- [x] AOS-1933…1940 : contrat VMM sans tas, répertoires utilisateur statiques et restitution PMM des pages privées — [aos1933_1940_vmm_static_contract.md](aos1933_1940_vmm_static_contract.md)
- [x] AOS-1941…1948 : réconciliation des limites historiques, renvoi vers les macro-lots successeurs et index documentaire cohérent — [aos1941_1948_backlog_documentation_reconciliation.md](aos1941_1948_backlog_documentation_reconciliation.md)
- [x] AOS-1949…1956 : captures QEMU GUI portables, saisie contrôlée avec reprise et validation shell/IA/NE2000 reproductible — [aos1949_1956_portable_gui_captures.md](aos1949_1956_portable_gui_captures.md)
- [x] AOS-1957…1964 : réconciliation GGUF, validation `C/V/T` et axes de couches branchés aux kernels quantifiés caller-owned — [aos1957_1964_gguf_dimension_reconciliation.md](aos1957_1964_gguf_dimension_reconciliation.md)
- [x] AOS-1965…1972 : rollback ELF transactionnel après restauration du VMM actif et restitution PMM des mappings partiels — [aos1965_1972_elf_transactional_rollback.md](aos1965_1972_elf_transactional_rollback.md)
- [x] AOS-1973…1980 : réconciliation FAT32 LFN UTF-8 et runtime GGUF quantifié avec les capacités déjà couvertes par le code et les tests ; intégration FAT32 au VFS explicitement distincte — [aos1973_1980_fat32_gguf_reconciliation.md](aos1973_1980_fat32_gguf_reconciliation.md)
- [x] AOS-1981…1988 : smoke QEMU Ring 3 des builtins `sort`, `head` et `tail`, fixture multi-ligne, assertions d’ordre et rattachement au gate CI — [aos1981_1988_qemu_shell_file_builtins.md](aos1981_1988_qemu_shell_file_builtins.md)
- [x] AOS-1989…1996 : alias VFS dynamiques FAT16/FAT32, table statique préservant trois alias, lecture seule et contrat QEMU sur fixture FAT16 — [aos1989_1996_vfs_fat_dynamic_mounts.md](aos1989_1996_vfs_fat_dynamic_mounts.md)
- [x] AOS-1997…2004 : VFS QEMU multi-disque avec fixtures FAT16/FAT32, quatre alias dynamiques statiques et contrôles de lecture, listage et statut — [aos1997_2004_vfs_fat32_secondary.md](aos1997_2004_vfs_fat32_secondary.md)
- [x] AOS-2005…2012 : pagination de la source virtuelle `vfs-mounts`, inventaire des huit montages sans dépasser la borne IPC — [aos2005_2012_vfs_mounts_pagination.md](aos2005_2012_vfs_mounts_pagination.md)
- [x] AOS-2013…2020 : observation cohérente de `vfs-mounts` avec génération, détection d’état obsolète et validation QEMU — [aos2013_2020_vfs_mounts_observe.md](aos2013_2020_vfs_mounts_observe.md)
- [x] AOS-2021…2028 : pagination native des racines FAT16/FAT32 depuis le VFS, ABI Ring 3 dédiée, compatibilité des listes historiques et validation Unity/QEMU — [aos2021_2028_fat_vfs_root_pagination.md](aos2021_2028_fat_vfs_root_pagination.md)
- [x] AOS-2029…2036 : libération autonome des capacités backend VFS, PID dérivé de la tâche Ring 3, génération de révocation et contrat QEMU — [aos2029_2036_vfs_backend_self_release.md](aos2029_2036_vfs_backend_self_release.md)
- [x] AOS-2037…2044 : routeur IPC Ring 3 commun aux réponses VFS, corrélation par PID/type/requête, conservation des messages discordants et contrat QEMU intercalé — [aos2037_2044_ipc_deferred_response_router.md](aos2037_2044_ipc_deferred_response_router.md)
- [x] AOS-2045…2052 : table statique d’opérations backend VFS, dispatch de chemins externalisé, mutabilité overlay explicite et contrat QEMU d’alias dynamique — [aos2045_2052_vfs_path_backend_ops.md](aos2045_2052_vfs_path_backend_ops.md)
- [x] AOS-2053…2060 : benchmark GGUF QEMU répétable, rapport JSON de médiane/dispersion et campagne locale de référence sur premier token et continuation — [aos2053_2060_gguf_qemu_latency_benchmark.md](aos2053_2060_gguf_qemu_latency_benchmark.md)
- [x] AOS-2061…2068 : contrat QEMU isolé du bootstrap LLM NE2000, DHCP/OFFER/REQUEST/ACK, ARP, DNS A et SYN observés ; lecture PROM DMA, PAR strict et ordre RDC/TX régressés — [aos2061_2068_ne2k_qemu_controlled_bootstrap.md](aos2061_2068_ne2k_qemu_controlled_bootstrap.md)
- [x] AOS-2069…2075 : pair QEMU TCP contrôlé, SYN-ACK checksumé, voisin de prochain saut DHCP conservé pour TLS/HTTP, ClientHello observé et publication Ring 3 `TLS_STARTED` — [aos2069_2075_ne2k_qemu_synack_clienthello.md](aos2069_2075_ne2k_qemu_synack_clienthello.md)
- [x] AOS-2076…2080 : ServerHello TLS 1.2 minimal reçu du pair QEMU, parsing borné au paquet IPv4 et ACK TCP observé ; certificat, X25519 et TLS complet restent distincts — [aos2076_2080_ne2k_qemu_server_hello.md](aos2076_2080_ne2k_qemu_server_hello.md)
- [x] Lot TLS/HTTP local : ancre de test `example.com` (SAN aussi `api.example.test`), pair QEMU `0xc02f` + X25519 + RSA/SHA-256, `TLS_COMPLETE`, POST ollama et HTTP 200 JSON ; `make qemu-ne2k-tls-http` ; pas d'hote public ni d'OpenAI
- [x] Lot TLS/SSE local : POST `"stream":true`, HTTP chunked sans Content-Length, delta `SSE : ok` puis `[DONE]` ; `make qemu-ne2k-tls-sse` ; pas d'hote public ni d'OpenAI
- [x] Lot TLS/next local : `ai-next` rearme HTTP/SSE, second POST sur la meme session TLS ; `make qemu-ne2k-tls-next` ; pas d'hote public ni d'OpenAI
- [x] AOS-2081…2088 : worker VFS virtuel Ring 3, délégation asynchrone bornée de `vfs-info`, corrélation PID/request-id, repli local et contrat QEMU inter-processus — [aos2081_2088_vfs_ring3_virtual_worker.md](aos2081_2088_vfs_ring3_virtual_worker.md)
- [x] AOS-2089…2094 : instantané `vfs-stats` de 16 octets transmis au worker Ring 3 pour formatage borné, corrélation conservée et contrat QEMU sur les valeurs publiques — [aos2089_2094_vfs_ring3_stats_worker.md](aos2089_2094_vfs_ring3_stats_worker.md)
- [x] AOS-2095…2102 : entrées `vfs-mounts` formatées séquentiellement par worker Ring 3, agrégation VFS de 80 octets, troncature historique préservée et attente de lecture bornée — [aos2095_2102_vfs_ring3_mounts_worker.md](aos2095_2102_vfs_ring3_mounts_worker.md)
- [x] AOS-2103…2108 : redécouverte stricte du worker VFS, repli local vérifié après retrait du service et reprise de délégation après relance QEMU — [aos2103_2108_vfs_worker_resilience.md](aos2103_2108_vfs_worker_resilience.md)
- [x] AOS-2109…2114 : récupération bornée d’une transaction privée VFS active après disparition du worker, réponse locale corrélée et redélégation QEMU — [aos2109_2114_vfs_worker_inflight_recovery.md](aos2109_2114_vfs_worker_inflight_recovery.md)
- [x] AOS-2115…2120 : vue locale `vfs-worker`, PID publié/absence et compteur volatile des récupérations en vol, validés dans le contrat QEMU — [aos2115_2120_vfs_worker_health.md](aos2115_2120_vfs_worker_health.md)
- [x] AOS-2121…2126 : expiration coopérative d’un worker VFS vivant mais silencieux après huit tours du médiateur, repli local corrélé et compteur `timeouts` vérifiés sous QEMU — [aos2121_2126_vfs_worker_timeout.md](aos2121_2126_vfs_worker_timeout.md)
- [x] AOS-2127…2132 : pages publiques `vfs-mounts` formatées séquentiellement par le worker Ring 3, avec pagination et génération conservées par le médiateur — [aos2127_2132_vfs_mount_page_worker.md](aos2127_2132_vfs_mount_page_worker.md)
- [x] AOS-2133…2138 : pages observées `vfs-mounts` formatées séquentiellement par le worker Ring 3, avec contrôle de génération et réponse `stale` conservés par le médiateur — [aos2133_2138_vfs_mount_observe_worker.md](aos2133_2138_vfs_mount_observe_worker.md)
- [x] AOS-2139…2146 : création FAT16 8.3 à la racine via `vfs-write`, sous capacité backend `mutate`, writer ATA explicite, prévention d’écrasement et preuve QEMU de persistance — [aos2139_2146_vfs_fat16_write.md](aos2139_2146_vfs_fat16_write.md)
- [x] AOS-2147…2154 : suppression FAT16 8.3 racine via `vfs-remove`, syscall backend `mutate`, marqueur d’entrée supprimée, libération bornée des copies FAT et preuve QEMU de retrait persistant — [aos2147_2154_vfs_fat16_unlink.md](aos2147_2154_vfs_fat16_unlink.md)
- [x] AOS-2155…2162 : renommage FAT16 8.3 racine via `vfs-rename`, syscall backend `mutate`, collision refusée, chaîne FAT inchangée et preuve QEMU ancien nom absent/nouveau nom lisible — [aos2155_2162_vfs_fat16_rename.md](aos2155_2162_vfs_fat16_rename.md)
- [x] Contrats QEMU dans `tests/integration` (cœur, IRQ0, fournisseur, NE2000, acquisition LLM contrôlée jusqu’au ServerHello, IPC, VFS et vues virtuelles déléguées Ring 3, services)

## Phase 6: Tests finaux et soumission sur GitHub ✅ (août 2026)
- [x] **Constat historique de livraison :** 489 tests et les smokes alors disponibles. L’état courant est **505/505**, avec sept contrats `make integration-qemu` en **23 min 30 s**, les cycles VFS LFN FAT16/FAT32 et sous-répertoire 8.3, ainsi que les contrats TLS/HTTP/SSE/`close_notify`/`ai-next` locaux ; voir [ETAT_REEL.md](ETAT_REEL.md).
- [x] Validation du fonctionnement en mode utilisateur (QEMU GTK + `sendkey`)
- [x] Commit et push des corrections sur GitHub
- [x] Documentation des corrections apportées ([ETAT_REEL.md](ETAT_REEL.md))

## Problème identifié (historique - phases 1 à 4)
Le mode simulation fonctionnait parfaitement mais le passage au mode utilisateur échoue - l'interface reste figée. Besoin d'analyser les appels système et la gestion des processus.

**Statut 2026 :** ce blocage est levé. Le kernel pose `g_reschedule_needed` puis le timer appelle `schedule()` une fois vers le shell ELF.


## Mise à jour réseau TCP — août 2026

Les lots réseau AOS-132 à AOS-150 sont désormais intégrés progressivement et validés par CI. Le pilote NE2000 sait lire la PROM MAC, émettre en PIO, sonder la réception et traiter IRQ3 ; ARP statique et actif, DHCP et DNS A sont disponibles dans leurs chemins caller-owned. AOS-147 construit un SYN IPv4 TCP, AOS-148 expose les segments TCP reçus, AOS-149 valide strictement un SYN-ACK et AOS-150 construit le premier ACK avec un état `SYN_SENT`/`ESTABLISHED` appartenant à l’appelant.

| Domaine | Statut réel |
|---|---|
| Ethernet, ARP, DHCP, DNS A | Implémentés et couverts par tests Unity / smokes NIC. |
| TCP SYN, SYN-ACK, ACK, payload, séquence, retransmission bornée | Implémentés (AOS-147…154) ; 299 tests verts. |
| Timer RTO, congestion, fermeture TCP, API socket | Non implémentés. |
| Handshake TLS, X.509, HTTP, OpenAI | Non fonctionnels de bout en bout. |

Cette section remplace toute interprétation de la ligne générique « Pilote NIC, DHCP, DNS, TCP/TLS et client OpenAI effectif » : le pilote et les codecs jusqu’à AOS-154 sont livrés, tandis que le bail live, le handshake TLS et le client LLM restent hors périmètre.

Références détaillées : [AOS-149](aos149_tcp_synack_validation.md) et [AOS-150](aos150_tcp_first_ack.md).

### AOS-151 — émission du premier ACK via NE2000

AOS-151 est implémenté localement : `ne2k_tcp_ack` utilise le cache ARP caller-owned, construit Ethernet/IPv4/TCP, calcule les checksums et transmet via `ne2k_tx_submit`. Le test NE2000 valide les ports, séquences, ACK et adresses MAC. La validation complète et la PR restent à effectuer après la non-régression.

Le prochain lot logique est AOS-152 : codec TCP avec données caller-owned et émission bornée d’un segment ACK+payload. Cela ne constitue pas encore un client HTTP, TLS ou LLM fonctionnel.

### AOS-152 — données TCP caller-owned

AOS-152 ajoute le codec `ACK+payload` et `ne2k_tcp_data`, avec longueur IPv4 exacte, checksums TCP/IPv4 et contrôles de capacité. La suite locale atteint 297 tests verts. Le smoke `qemu-ai-provider` a échoué une première fois sur l’absence de sortie `Runtime IA bare-metal`, puis a réussi lors d’une relance immédiate ; le smoke `qemu-ne2k-status` est vert. Cette anomalie de smoke IA reste à surveiller séparément du chemin TCP.

### AOS-153/AOS-154 — séquences, ACK reçus et retransmission bornée

AOS-153 fait progresser explicitement `local_sequence` après envoi confirmé et `remote_sequence` après acceptation d’un payload en séquence. AOS-154 conserve une vue caller-owned du dernier payload et borne le nombre de retransmissions autorisées. Aucun timer, RTO, mécanisme de congestion ou stockage copié n’est introduit. La validation complète atteint désormais 299 tests verts, avec build i386 et deux smokes QEMU réussis.

### AOS-155 à AOS-158 — retransmission, ACK final et fermeture TCP

Les lots ajoutent la retransmission NE2000 caller-owned, la confirmation et purge du payload pending, le codec et l’émission FIN+ACK, puis les transitions FIN_WAIT_1, FIN_WAIT_2, CLOSE_WAIT et CLOSED. La validation complète atteint 301 tests verts, avec build i386, smoke IA et smoke NE2000 réussis. Les timers RTO, la congestion, TLS, HTTP et l’appel LLM en ligne restent hors périmètre fonctionnel.

### AOS-159 à AOS-162 — envoi suivi d’ACK, réception, fenêtre et checksums

AOS-159 ajoute la construction data suivie d’un pending caller-owned avant commit TX. AOS-160 ajoute la réception TCP NE2000 bornée avec copie vers le buffer appelant. AOS-161 ajoute la fenêtre de réception explicite. AOS-162 valide les checksums IPv4 et TCP en réception. Validation : 304 tests verts, build i386 réussi, smoke IA réussi et smoke NE2000 réussi.

### AOS-163 à AOS-165 — polling TCP et orchestration RX

AOS-163 ajoute `ne2k_tcp_poll`, AOS-164 formalise le retour non bloquant RX vide avec longueur nulle, et AOS-165 regroupe les contrôles de bornes, checksums, séquence et fenêtre dans le chemin de réception caller-owned. Le prochain incrément reste la génération d’un ACK automatique après payload accepté.

Validation AOS-163/AOS-165 : **305/305 tests verts**, build i386 réussi, `qemu-ai-provider` réussi et `qemu-ne2k-status` réussi.

### AOS-166/AOS-167 — polling TCP suivi d’ACK

`ne2k_tcp_poll_ack` reçoit et valide un payload TCP, le publie dans le buffer appelant puis émet son ACK via le cache ARP caller-owned. RX vide retourne `1` sans émission ; une erreur ARP/TX est propagée après acceptation afin que l’appelant puisse décider d’une nouvelle tentative. Documentation : `docs/aos166_167_tcp_poll_ack.md`.

Validation AOS-166/AOS-167 : **305/305 tests verts**, build i386 réussi, smoke `qemu-ai-provider` réussi et smoke `qemu-ne2k-status` réussi.

### AOS-168/AOS-169 — FIN→ACK et contrôle TCP

Le polling FIN caller-owned valide la trame, effectue la transition de fermeture et émet l’ACK via NE2000. Les ACK purs ne déclenchent pas de réponse automatique afin d’éviter les boucles. Le framing TLS 1.2 existe séparément, mais son handshake cryptographique et son raccordement TCP restent à implémenter.

Validation AOS-168/AOS-169 : **305/305 tests verts**, build i386 réussi, smoke `qemu-ai-provider` réussi et smoke `qemu-ne2k-status` réussi.

### AOS-170 — ClientHello TLS 1.2 minimal

Le codec TLS record construit maintenant un ClientHello minimal caller-owned avec random fourni, sans génération cryptographique ni négociation complète. SNI/ALPN, dérivation de clés, chiffrement, X.509, HTTP et appels LLM en ligne restent hors périmètre fonctionnel.

Validation AOS-170 : **306/306 tests verts**, build i386 réussi, smoke `qemu-ai-provider` réussi et smoke `qemu-ne2k-status` réussi.

### AOS-171 — composition TLS sur TCP

`net_tcp_connection_build_tls_record` construit un record TLS dans un buffer caller-owned puis l’encapsule dans un segment TCP pending, réutilisable pour commit et retransmission. Le raccordement ne fournit encore ni cryptographie TLS, ni certificat, ni HTTP ou appel LLM en ligne.

Validation AOS-171 : **307/307 tests verts**, après ajout de `net_tls_record.c` aux chemins de linkage TCP/NE2000 du harness ; build i386 réussi, smoke `qemu-ai-provider` réussi et smoke `qemu-ne2k-status` réussi.

### AOS-172/AOS-173 — parsing TLS depuis TCP

Le parsing stream TLS distingue les fragments incomplets et les records complets. La connexion TCP accepte un record complet uniquement lorsqu’il occupe exactement le payload reçu, puis avance le sequence distant ; les buffers d’assemblage restent caller-owned.

Validation AOS-172/AOS-173 : **309/309 tests verts**, build i386 réussi, smoke `qemu-ai-provider` réussi et smoke `qemu-ne2k-status` réussi.

### AOS-174 — accumulateur de fragments TLS

L’état d’assemblage TLS est désormais caller-owned : les fragments TCP sont bornés par la capacité fournie, le record incomplet reste non publié et le record complet est parsé sans allocation. Validation : **310/310 tests verts**, build i386 réussi, smoke IA réussi et smoke NE2000 réussi.

### AOS-175 — parsing ServerHello TLS

Le ServerHello TLS minimal est désormais parsé sans copie dans une vue caller-owned, avec contrôles de longueur, version, random, session ID, cipher suite et compression. Validation : **311/311 tests verts**, build i386 réussi, smoke IA réussi et smoke NE2000 réussi. Extensions TLS, dérivation de clés, chiffrement, X.509, HTTP et appels LLM en ligne restent hors périmètre.

### AOS-176 — état minimal du handshake TLS

Le handshake TLS caller-owned suit désormais l’ordre `IDLE → CLIENT_HELLO_SENT → SERVER_HELLO_RECEIVED`, conserve la suite et une vue du random serveur, et rejette les transitions invalides. Validation : **312/312 tests verts**, build i386 réussi, smoke IA réussi et smoke NE2000 réussi. La cryptographie, Finished, X.509, HTTP et les appels LLM en ligne restent hors périmètre.


### AOS-177 — Extensions ServerHello TLS

Le parseur `ServerHello` expose désormais le bloc d’extensions TLS via une vue caller-owned (`extensions` et `extensions_length`). Il accepte l’absence d’extensions ou un vecteur correctement borné, et rejette toute longueur incohérente sans `kmalloc`. Le faux échec de `test_net_tls_record` provenait d’un dépassement de pile dans le test, corrigé par un buffer dimensionné pour le cas d’extension.

Validation AOS-177 : **312/312 tests verts**, build i386 réussi, `qemu-ai-provider` réussi et `qemu-ne2k-status` réussi. Le décodage sémantique des extensions, X.509, la dérivation de clés, le chiffrement TLS, Finished, HTTP et les appels LLM sécurisés de bout en bout restent non implémentés. Référence : [aos177_tls_server_hello_extensions.md](aos177_tls_server_hello_extensions.md).


### AOS-178 — Parsing Certificate TLS 1.2

Le message `Certificate` TLS 1.2 est maintenant validé sans allocation dynamique. Le codec contrôle les longueurs 24 bits du corps, de la liste et de chaque certificat, publie la première chaîne via une vue caller-owned et parcourt les entrées supplémentaires. L’automate accepte Certificate uniquement après `SERVER_HELLO_RECEIVED` et passe à `CERTIFICATE_RECEIVED`.

Validation ciblée : **8/8 tests TLS verts**. La suite complète, le build i386 et les smokes QEMU restent à confirmer avant publication. La validation X.509, la dérivation de clés, le chiffrement TLS, Finished, HTTP et les appels LLM sécurisés de bout en bout restent non implémentés. Référence : [aos178_tls_certificate_parse.md](aos178_tls_certificate_parse.md).

### AOS-179 — ServerHelloDone TLS 1.2

Le message `ServerHelloDone` est désormais validé comme handshake de type 14 à corps vide. L’automate caller-owned l’accepte uniquement après `CERTIFICATE_RECEIVED`, passe à `SERVER_HELLO_DONE_RECEIVED` et rejette les transitions hors ordre ou répétées. Aucun `kmalloc`, buffer interne ou copie n’est introduit.

Validation AOS-179 : **316/316 tests verts**, build i386 réussi, `qemu-ai-provider` réussi et `qemu-ne2k-status` réussi. Le harnais IA réessaie une seule fois une commande en cas de perte ponctuelle d’un caractère par l’injection clavier QEMU. Les échanges de clés, la dérivation de secrets, X.509, le chiffrement TLS, Finished, HTTP et les appels LLM sécurisés de bout en bout restent non implémentés. Référence : [aos179_tls_server_hello_done.md](aos179_tls_server_hello_done.md).


### AOS-180 à AOS-184 — macro-lot messages serveur TLS, transcript et TCP

Le handshake TLS dispose maintenant d’une vue générique, d’un accumulateur de fragments et d’un transcript borné fournis par l’appelant. Le macro-lot parse `ServerKeyExchange` ECDHE et `CertificateRequest`, étend l’automate jusqu’à `ServerHelloDone`, puis connecte le dispatch serveur au record TLS reçu sur TCP. Le chemin TCP est transactionnel : une erreur de type de record ou de handshake restaure sa séquence et sa fenêtre de réception.

Validation AOS-180/AOS-184 : **320/320 tests verts**. Les 13 tests TLS ciblés et le test TCP d’intégration couvrent le framing, les limites, les transitions, le transcript et la restauration transport. Aucun `kmalloc` n’est introduit. La validation X.509, la signature ServerKeyExchange, ECDHE, la dérivation de clés, le chiffrement TLS, Finished, HTTP et les appels LLM sécurisés de bout en bout restent non implémentés. Référence : [aos180_184_tls_server_macro.md](aos180_184_tls_server_macro.md).


### AOS-185 à AOS-188 — flight client TLS après ServerHelloDone

Le macro-lot construit maintenant `Certificate` client vide, `ClientKeyExchange`, `ChangeCipherSpec` et `Finished` dans des buffers caller-owned. L’automate distingue le chemin avec `CertificateRequest` du chemin sans certificat client, puis impose l’ordre ClientKeyExchange → ChangeCipherSpec → Finished. Le `verify_data` est fourni par l’appelant : aucune dérivation ECDHE, PRF TLS, signature ou cryptographie de record n’est simulée.

Validation AOS-185/AOS-188 : **321/321 tests verts**. La dérivation du secret partagé, l’authentification X.509, la vérification de signature, le PRF TLS 1.2, le chiffrement AEAD, Finished authentique, HTTP et les appels LLM sécurisés de bout en bout restent non implémentés. Référence : [aos185_188_tls_client_flight.md](aos185_188_tls_client_flight.md).


### AOS-189 à AOS-192 — PRF TLS 1.2, master secret et Finished

Le PRF TLS 1.2 HMAC-SHA256 est maintenant disponible avec workspace caller-owned, ainsi que le hash SHA-256 du transcript, la dérivation d’un master secret de 48 octets à partir d’un premaster secret fourni et le `verify_data` client Finished de 12 octets. Les vecteurs déterministes valident les quatre primitives. Les dépendances SHA-256 sont déclarées pour les tests TLS, TCP et NE2000 dans les deux couches du harness.

Validation AOS-189/AOS-192 : **322/322 tests verts**. ECDHE réel, le premaster secret, X.509, la signature ServerKeyExchange, les clés de trafic, AEAD, Finished serveur, HTTP et les appels LLM sécurisés de bout en bout restent non implémentés. Référence : [aos189_192_tls_prf_finished.md](aos189_192_tls_prf_finished.md).


### AOS-193 à AOS-196 — post-flight serveur ChangeCipherSpec et Finished

Le post-flight serveur est maintenant encadré par le parsing strict de ChangeCipherSpec (`0x01`) puis de Finished (12 octets). La comparaison de `verify_data` est effectuée sans sortie anticipée. L’intégration TCP accepte les deux records de manière transactionnelle, ajoute Finished validé au transcript et restaure la connexion, l’automate et la longueur du transcript en cas d’erreur.

Validation AOS-193/AOS-196 : **324/324 tests verts**. Le code ne déchiffre ni n’authentifie de records TLS chiffrés : ECDHE, X.509, clés de trafic, AEAD, HTTP et appels LLM sécurisés de bout en bout restent non implémentés. Référence : [aos193_196_tls_server_postflight.md](aos193_196_tls_server_postflight.md).


### AOS-197 à AOS-200 — expansion des clés AES-128-GCM

Une fonction caller-owned dérive maintenant le bloc de 40 octets `key expansion` pour AES-128-GCM à partir du master secret et de `server_random || client_random`. Elle publie les vues client/server write keys de 16 octets et les IV fixes de 4 octets. Le vecteur HMAC-SHA256 couvre le bloc entier et les bornes de capacité.

Validation AOS-197/AOS-200 : **325/325 tests verts**. AES-128-GCM, nonce explicite, tags, chiffrement/déchiffrement de records, ECDHE réel, X.509, HTTP et les appels LLM TLS sécurisés de bout en bout restent non implémentés. Référence : [aos197_200_tls_key_block.md](aos197_200_tls_key_block.md).


### AOS-201 à AOS-208 — AES-128-GCM, records protégés et TCP transactionnel

AES-128, GHASH et GCM sont maintenant implémentés en freestanding. Les records TLS 1.2 utilisent le nonce explicite de huit octets, l’AAD de séquence TLS et un tag de seize octets. Les sessions caller-owned sélectionnent les clés client/serveur, avancent les séquences après succès seulement et les adaptateurs TCP restaurent les états TCP/TLS en cas d’échec de chiffrement, d’authentification ou de déchiffrement.

Validation AOS-201/AOS-208 : **330/330 tests verts**, build i386 réussi, smokes `qemu-ai-provider` et `qemu-ne2k-status` réussis. ECDHE réel, X.509, vérification `ServerKeyExchange`, validation des suites négociées, orchestration TLS de production, HTTP et appels LLM sécurisés de bout en bout restent à implémenter. Référence : [aos201_208_tls_aes_gcm.md](aos201_208_tls_aes_gcm.md).


### AOS-209 à AOS-216 — DER/X.509 minimal caller-owned

Un lecteur DER borné et un parseur X.509 minimal publient maintenant sans copie le TBSCertificate, le serial, issuer, subject, dates de validité et la clé publique RSA du certificat serveur. Le handshake peut déclencher explicitement cette analyse après réception de Certificate. Les longueurs tronquées, indéfinies ou incohérentes sont rejetées.

Validation AOS-209/AOS-216 : **332/332 tests verts**, build i386 réussi. La chaîne de confiance, les dates, l’hôte, les usages, les signatures de certificats, ECDHE réel, la signature ServerKeyExchange et l’orchestration TLS de production restent à implémenter. Référence : [aos209_216_x509_der.md](aos209_216_x509_der.md).


### AOS-217 — Interface RSA PKCS#1 v1.5 SHA-256 : état

L’interface caller-owned `rsa_pkcs1_v15_sha256_verify` est réservée pour la future vérification de signature. Elle ne contient encore ni bigint, ni exponentiation modulaire, ni décodage PKCS#1, ni tests RSA, ni intégration `ServerKeyExchange` ; elle ne fournit donc aucune authentification. Référence : [aos217_rsa_interface_status.md](aos217_rsa_interface_status.md).


### AOS-225 à AOS-232 — bigint multi-limb, RSA PKCS#1 v1.5 et ServerKeyExchange

Le noyau contient désormais une arithmétique bigint caller-owned multi-limb, une réduction modulaire binaire, une exponentiation publique et `rsa_pkcs1_v15_sha256_verify`. La vérification exige un workspace de `7 × ceil(taille_module/4)` limbs fourni et aligné par l’appelant ; elle ne recourt à aucune allocation dynamique. Les tests couvrent une signature RSA-512 PKCS#1 v1.5 SHA-256 valide, les rejets de digest et de signature falsifiés, ainsi qu’une exponentiation à deux limbs.

Une API TLS authentifiée `net_tls_handshake_accept_server_key_exchange_rsa` vérifie désormais `SHA-256(client_random || server_random || ServerECDHParams)` avec la clé RSA extraite du certificat, avant la transition de l’automate. Le dispatcher historique ne reçoit pas encore le random client ni le workspace RSA et reste donc un chemin de parsing non authentifié ; il ne faut pas l’employer comme preuve d’un handshake TLS valide. Référence : [aos225_232_rsa_verify.md](aos225_232_rsa_verify.md).

Les validations de chaîne X.509 et de nom d’hôte, ECDHE réel, les signatures ECDSA, le secret partagé, le raccordement du dispatcher TLS authentifié, HTTP et les appels LLM TLS de bout en bout restent non implémentés.


### AOS-233 à AOS-240 — flux TLS authentifié, X.509 RSA et ECDHE structurel

Le dispatcher `net_tls_handshake_accept_server_message_authenticated` reçoit désormais explicitement le `client_random`, le transcript et le workspace RSA caller-owned. Il parse et transcrit `ServerHello`, analyse `Certificate` en DER/X.509, impose une clé `rsaEncryption` valide, puis vérifie la signature RSA/SHA-256 de `ServerKeyExchange` avant toute transition d’état. L’état du handshake et le transcript sont restaurés transactionnellement sur erreur.

Les paramètres ECDHE font l’objet d’une validation de forme pour secp256r1 (point non compressé de 65 octets) et X25519 (32 octets). Cette validation ne réalise ni multiplication de courbe, ni validation d’appartenance du point, ni dérivation de secret. Le flux historique reste un parseur non authentifié et ne doit pas être confondu avec le nouveau dispatcher. Référence : [aos233_240_tls_authenticated_flow.md](aos233_240_tls_authenticated_flow.md).

La chaîne de confiance, les dates, le nom d’hôte, les usages, les signatures de certificats, ECDSA, ECDHE P-256/X25519 réel, le secret partagé, HTTP et les appels LLM HTTPS de bout en bout restent non implémentés.


### AOS-241 à AOS-248 — X25519, ECDHE et secret partagé TLS

Le noyau contient une implémentation caller-owned de X25519 sur Curve25519 : ladder Montgomery, clamp, clé publique, secret partagé et rejet du résultat nul. Le calcul emploie un workspace fixe de 136 limbs de 32 bits et ne fait appel à aucune allocation dynamique. Le vecteur RFC 7748 est couvert par `test_x25519` [1].

Le ClientHello offre désormais `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` (`0xC02F`), X25519 et RSA/SHA-256. Le dispatcher authentifié impose cette suite, exige X25519 au ServerKeyExchange signé, puis fournit la préparation du ClientKeyExchange et la dérivation du master secret depuis le secret partagé. Référence : [aos241_248_x25519_ecdhe.md](aos241_248_x25519_ecdhe.md).

**Limite critique :** le backend bigint modulaire est fonctionnel mais non constant-temps. X25519 ne doit pas encore protéger un secret dans un contexte hostile. Le transcript automatique du ClientKeyExchange, la chaîne X.509, les dates, le nom d’hôte, les signatures de certificats, ECDSA, l’achèvement TLS associé à ce contexte, HTTP et les appels LLM HTTPS de bout en bout restent non implémentés.

[1] [RFC 7748 — Elliptic Curves for Security](https://datatracker.ietf.org/doc/html/rfc7748)


### AOS-249 à AOS-256 — flight client X25519 transactionnel

L’API `net_tls_x25519_client_flight_build` construit désormais le flight client TLS dans des buffers caller-owned : ClientKeyExchange X25519, ChangeCipherSpec et Finished AES-128-GCM. Elle ajoute ClientKeyExchange puis Finished au transcript, dérive le master secret et le key block depuis le contexte X25519, initialise la session AEAD et publie trois records totalisant 93 octets.

La construction est transactionnelle : état du handshake, contexte X25519, transcript et longueur de sortie sont restaurés ou annulés sur erreur. Le chemin refuse actuellement les certificats clients pour ne pas émettre un flight incomplet. Référence : [aos249_256_x25519_client_flight.md](aos249_256_x25519_client_flight.md).

Le transport TCP de ce flight, la validation du ChangeCipherSpec/Finished serveur dans ce même contexte, la chaîne X.509, les dates, le nom d’hôte, les certificats clients, HTTP et les appels LLM HTTPS de bout en bout restent non implémentés. X25519 reste non constante-temps sur le backend bigint actuel.


### AOS-257 à AOS-264 — transport TCP du flight X25519 et post-flight serveur

Le transport TCP expose désormais `net_tcp_connection_build_tls_x25519_flight`, qui encapsule le flight client X25519 transactionnel dans un segment TCP et restaure connexion, handshake, contexte X25519, transcript, session, secrets et longueur publiée sur erreur. Le codec TLS calcule aussi le `server finished`, tandis que `net_tcp_connection_accept_tls_x25519_postflight` accepte le ChangeCipherSpec serveur puis déchiffre, vérifie et transcrit le Finished AES-GCM.

Le test de transport couvre le rollback d’une capacité TCP insuffisante, le suivi des séquences TCP, le rejet transactionnel d’un tag AES-GCM falsifié, le Finished serveur valide et le premier record applicatif chiffré après handshake complet. Référence : [aos257_264_tcp_tls_x25519.md](aos257_264_tcp_tls_x25519.md).

La connexion automatique entre le réassembleur TCP de production et cette orchestration, la chaîne X.509, les dates, le nom d’hôte, les certificats clients, HTTP, l’authentification HTTP, la reprise de session et les appels LLM HTTPS de bout en bout restent non implémentés. Le backend X25519/bigint reste non constante-temps.


### AOS-265 à AOS-272 — flux TCP/TLS authentifié et framing HTTP

Le contexte caller-owned `net_tcp_tls_stream_t` réassemble désormais record et message handshake TLS à travers des fragments TCP, puis applique le dispatcher RSA authentifié. Les fragments incomplets sont conservés et signalés par le statut `1`; un message invalide restaure connexion, handshake, transcript et accumulateurs. Le test couvre un `ServerKeyExchange` RSA authentifié sur deux fragments et le rollback d’une signature falsifiée.

Le module `net_http_tls` construit un GET HTTP/1.1 minimal, le chiffre via la session TLS AES-GCM/TCP et ouvre transactionnellement une réponse HTTP/1.1 complète déjà déchiffrée. Référence : [aos265_272_tls_stream_http.md](aos265_272_tls_stream_http.md).

Le flux ne prend encore en charge qu’un message handshake par record et une réponse HTTP complète dans un seul plaintext. La chaîne X.509, les dates, le nom d’hôte, les certificats clients, `Content-Length`, chunked, les réponses streaming, HTTP POST, l’authentification HTTP, les appels LLM et HTTPS de production complet restent non implémentés. Le backend X25519/bigint reste non constante-temps.


### AOS-273 à AOS-280 — réponse HTTP Content-Length progressive

Le module HTTP/TLS expose désormais `net_http_response_accumulator_t` et `net_http_tls_open_response_stream`. Ils accumulent une réponse HTTP/1.1 sur plusieurs records AES-GCM, exigent un `Content-Length` décimal unique borné à 65 535, et publient une vue body uniquement lorsque la taille reçue correspond exactement à la taille annoncée. Les erreurs HTTP, TCP ou AEAD restaurent le contexte de l’appel.

Le test couvre une réponse `200` dont le body de cinq octets arrive en deux records TLS successifs. Référence : [aos273_280_http_content_length_stream.md](aos273_280_http_content_length_stream.md).

`Transfer-Encoding: chunked`, les réponses terminées par fermeture, trailers, compression, HTTP/2, POST, authentification applicative, pagination, streaming LLM, handshake de production, chaîne X.509, dates, nom d’hôte et backend X25519 constante-temps restent non implémentés.


### AOS-281 à AOS-288 — HTTP POST JSON borné sur TLS

La couche HTTP/TLS fournit `net_http_build_post_json` et `net_http_tls_build_post_json`. La première construit un POST HTTP/1.1 caller-owned avec `Host`, `Content-Type: application/json`, `Content-Length` décimal et `Connection: close`; la seconde chiffre ce plaintext dans un record TLS AES-GCM encapsulé dans TCP. Les tests contrôlent le framing exact, les refus de paramètres ou capacités invalides et le déchiffrement octet pour octet côté serveur simulé.

Référence : [aos281_288_http_post_json.md](aos281_288_http_post_json.md). L’implémentation n’assure pas encore la fragmentation de très grandes requêtes, chunked, HTTP/2, authentification HTTP/API, retries, streaming LLM, handshake de production, hostname, chaîne X.509, dates ni X25519 constante-temps.


### AOS-289 à AOS-296 — validation hostname X.509/TLS

Le parseur X.509 publie le `commonName` et les `subjectAltName` DNS, puis `x509_certificate_hostname_validate` applique une comparaison DNS ASCII bornée. Les dNSName SAN sont prioritaires sur le CN, la casse ASCII est ignorée, et le wildcard `*.suffix` ne couvre qu’un seul label. Les tests couvrent le CN, la priorité SAN, le fallback sans SAN, la casse et le rejet d’un wildcard multi-label.

Référence : [aos289_296_tls_hostname.md](aos289_296_tls_hostname.md). Les IP, IDNA, Unicode, contraintes de nom, usages de clé, chaîne de confiance, ancres, dates, signatures de certificats et l’invocation automatique depuis le pilote restent à implémenter ; X25519/bigint demeure non constante-temps.


### AOS-297 à AOS-304 — chaîne X.509 RSA minimale à une ancre

`x509_certificate_chain_validate_one` valide désormais une feuille directement émise par une ancre X.509 caller-owned. Il impose l’égalité `issuer`/`subject`, valide la clé RSA de l’ancre, hash le DER complet TBSCertificate avec SHA-256 et vérifie la signature `sha256WithRSAEncryption` via RSA PKCS#1 v1.5 avec workspace fourni. Les vues TBS DER, algorithme et signature sont publiées par le parseur.

Le test utilise une racine RSA 1024 bits et une feuille RSA/SHA-256 signée, intégrées comme vecteur DER hors ligne ; il couvre le succès, un émetteur d’ancre altéré et une signature tronquée. Référence : [aos297_304_x509_trust_chain.md](aos297_304_x509_trust_chain.md).

Les intermédiaires, dates, contraintes et usages de clé, révocation, ECDSA/EdDSA, algorithmes SHA-384+, configuration des ancres et invocation automatique depuis NE2000 restent absents. Le hostname demeure une validation séparée et X25519/bigint reste non constante-temps.


### AOS-305 à AOS-312 — orchestrateur handshake TLS depuis NE2000

`ne2k_tls_client_t` relie désormais le polling TCP NE2000 au réassembleur TLS authentifié, à la validation de chaîne RSA à une ancre et hostname, au flight X25519 et au post-flight AES-GCM. `ne2k_tls_client_start` émet ClientHello transactionnellement ; `ne2k_tls_client_poll` accumule les fragments, contrôle l’identité, émet le flight une fois `ServerHelloDone` reçu et marque le handshake complet après Finished serveur.

Le contexte et tous les buffers/workspaces sont caller-owned. Les tests NE2000 couvrent l’initialisation, l’émission ClientHello, les séquences/transcript et le polling vide sans mutation ; les tests TCP/TLS existants couvrent le flux cryptographique sous-jacent. Référence : [aos305_312_ne2k_tls_orchestrator.md](aos305_312_ne2k_tls_orchestrator.md).

Le flux ne constitue pas encore un HTTPS de production : un message serveur par record, pas de certificats clients, chaîne RSA directe sans intermédiaires/dates/usages/révocation/ECDSA, pas de test QEMU contre un serveur externe, ni résolution/connexion complète, HTTP LLM, auth API, retries ou fragmentation de grosses requêtes. X25519/bigint reste non constante-temps.


### AOS-313 à AOS-320 — client HTTPS LLM sur NE2000

`ne2k_https_llm_post_json` construit et chiffre un POST JSON HTTP/1.1 uniquement après le Finished TLS complet, puis l’émet via TCP/NE2000 avec rollback de la séquence TCP et du compteur AEAD en cas d’échec. `ne2k_https_llm_poll_response` ouvre les records AES-GCM reçus puis alimente l’accumulateur HTTP Content-Length caller-owned et ACK seulement après traitement réussi.

Le module HTTP/TLS est désormais lié au test NE2000 ; les tests HTTP/TLS couvrent le framing POST et le round-trip AES-GCM, tandis que le test NE2000 couvre la préparation TLS et le polling vide. Référence : [aos313_320_https_llm_client.md](aos313_320_https_llm_client.md).

Aucune clé API, auth Authorization, intégration spécifique OpenAI/Ollama, DNS/SYN de connexion, test QEMU contre serveur externe, streaming SSE, chunked, HTTP/2, retries, pagination, compression, grandes requêtes ni parsing JSON n’est encore livré. La PKI reste une ancre RSA directe et X25519/bigint reste non constante-temps.


### AOS-321 à AOS-328 — Authorization Bearer caller-owned

`net_http_build_post_json_bearer` ajoute `Authorization: Bearer <token>` à un POST JSON borné sans conserver le token dans les contextes TCP, TLS ou NE2000. Le framing impose un token non vide en ASCII imprimable, la capacité caller-owned et les mêmes règles HTTP Content-Length. Le test vérifie le plaintext exact, un token fictif, les bornes et les rejets.

Référence : [aos321_328_http_authorization.md](aos321_328_http_authorization.md). Le secret réel, son stockage/effacement, rotation, révocation, auth mutuelle, formats OpenAI/Ollama, proxy, HTTP/2 et streaming restent hors périmètre.


### AOS-329 à AOS-336 — HTTP Transfer-Encoding chunked

`net_http_chunked_accumulator_t` décode désormais des réponses HTTP `Transfer-Encoding: chunked` à travers plusieurs plaintexts TLS, avec tailles hexadécimales bornées, CRLF contrôlés et compactage du body dans le buffer caller-owned. La publication intervient uniquement après `0\r\n\r\n`. Le vecteur couvre `Wikipedia` réparti sur deux fragments et le rejet d’une taille `Z` invalide.

Référence : [aos329_336_http_chunked.md](aos329_336_http_chunked.md). Les extensions de chunk, trailers non vides, compression, HTTP/2, SSE, grands corps et le wrapper TLS/NE2000 chunked restent hors périmètre.


### AOS-337 à AOS-344 — validité temporelle X.509

`x509_certificate_valid_at` vérifie désormais `notBefore <= instant <= notAfter` à partir d’un instant UTC caller-owned `YYYYMMDDhhmmssZ`. Les dates ASN.1 UTCTime et GeneralizedTime sont contrôlées, y compris calendrier et années bissextiles. Aucune horloge implicite n’est utilisée.

Les tests couvrent les instants interne, prématuré, expiré et une date calendairement invalide. Référence : [aos337_344_x509_validity_dates.md](aos337_344_x509_validity_dates.md). RTC/NTP, synchronisation sécurisée, tolérance d’horloge et appel automatique depuis l’orchestrateur TLS restent à implémenter.


### AOS-345 à AOS-352 — politique TLS chaîne, hostname et date

`x509_certificate_tls_identity_validate` exige désormais simultanément la chaîne RSA directe caller-owned, le hostname DNS et l’instant UTC caller-owned. Le test couvre le succès sur l’ancre/feuille de référence, le rejet d’un nom incorrect et d’une date prématurée.

Référence : [aos345_352_tls_trust_policy.md](aos345_352_tls_trust_policy.md). L’orchestrateur NE2000 ne fournit pas encore l’instant UTC à la politique. Intermédiaires, contraintes/usages, révocation, ECDSA et RTC/NTP sécurisé restent absents.


### AOS-353 à AOS-360 — politique temporelle dans NE2000/TLS

`ne2k_tls_client_poll` reçoit un instant UTC caller-owned et applique maintenant `x509_certificate_tls_identity_validate` avant de marquer le pair valide ou d’émettre le flight X25519. Toute erreur de chaîne, hostname ou date déclenche le rollback existant. Le test NE2000 fournit explicitement l’instant UTC du nouveau contrat ; les tests X.509 couvrent la politique de confiance.

Référence : [aos353_360_ne2k_tls_time_policy.md](aos353_360_ne2k_tls_time_policy.md). RTC/NTP sécurisé, handshake complet QEMU contre serveur externe, intermédiaires, révocation et ECDSA restent à implémenter.


### AOS-361 à AOS-368 — extraction JSON de réponses LLM

`net_json_extract_string` extrait et décode une valeur JSON string pour une clé ASCII dans un buffer caller-owned. Les échappements JSON simples sont décodés ; clé absente, contrôle brut, Unicode `\uXXXX`, string incomplète et capacité insuffisante sont rejetés. Le test couvre le champ `response`, le saut de ligne, les limites et le rejet Unicode.

Référence : [aos361_368_llm_json_extractor.md](aos361_368_llm_json_extractor.md). Ce n’est pas un parseur JSON complet : Unicode, tableaux, objets structurés, types non-string et SSE restent hors périmètre, de même que les adaptateurs OpenAI/Ollama spécifiques.


### AOS-369 à AOS-376 — adaptateurs de réponse Ollama/OpenAI

`net_llm_ollama_response_extract` extrait le champ `response` d’Ollama non-streaming ; `net_llm_openai_response_extract` extrait le premier champ string `content` d’une réponse OpenAI compatible. Les adaptateurs restent caller-owned et propagent les rejets de l’extracteur JSON. Les tests couvrent les deux formats simples et les champs fournisseur absents.

Référence : [aos369_376_llm_response_adapters.md](aos369_376_llm_response_adapters.md). L’adaptateur OpenAI ne valide pas la structure `choices[0].message.content`; SSE, tool calls, multi-choix, Unicode, médias et variations de schéma restent hors périmètre.


### AOS-377 à AOS-384 — builders JSON de requêtes LLM

`net_llm_build_ollama_generate_json` produit le body Ollama non-streaming `model`/`prompt`; `net_llm_build_openai_chat_json` produit le body OpenAI compatible avec un message utilisateur. Les chaînes caller-owned échappent guillemets, antislashs et espaces de contrôle courants ; contrôles non gérés, octets non ASCII et capacités insuffisantes sont rejetés.

Les tests vérifient les bodies exacts avec guillemets/saut de ligne et les rejets. Référence : [aos377_384_llm_request_builders.md](aos377_384_llm_request_builders.md). Header Bearer par fournisseur, Unicode, paramètres de génération, multi-tours, tool calls, média et SSE restent hors périmètre.

### AOS-385 à AOS-392 — requête LLM NE2000 unifiée

`ne2k_https_llm_request` compose le JSON Ollama/OpenAI, le POST HTTP, le Bearer obligatoire pour OpenAI et l’émission AES-GCM sur une session TLS complète. Tous les états et buffers restent caller-owned ; un échec après construction restaure la connexion et la séquence TLS. `ne2k_https_llm_poll_text` propage l’attente de body, refuse les statuts non-2xx et extrait le texte de réponse spécifique au fournisseur.

Le test NE2000 vérifie la requête OpenAI chiffrée, JSON exact, Bearer, séquences et rejets de sécurité. Référence : [aos385_392_ne2k_llm_request.md](aos385_392_ne2k_llm_request.md). Les erreurs HTTP détaillées, retry borné, SSE, Unicode, tool calls, multi-tours, DNS/SYN automatisés et test externe restent à traiter.

### AOS-393 à AOS-400 — erreurs HTTP LLM et retry borné

`net_llm_http_status_classify` distingue succès, authentification, erreurs retryables, erreurs permanentes et réponses hors protocole. `net_llm_http_retry_consume` augmente uniquement le compteur caller-owned lorsqu’un nouveau POST est autorisé dans la limite explicite. Il n’attend pas et ne réémet pas automatiquement un POST LLM.

Les vecteurs couvrent `401`, `403`, `408`, `429`, `503`, classes 2xx/3xx/4xx et l’épuisement du budget. Référence : [aos393_400_llm_http_errors.md](aos393_400_llm_http_errors.md). `Retry-After`, temporisation, circuit-breaker, SSE, Unicode, tools, multi-tours, DNS/SYN automatisés et test externe restent hors périmètre.

### AOS-401 à AOS-408 — RTC UTC i386

Le module `rtc` produit `YYYYMMDDHHMMSSZ` depuis les registres CMOS i386 via une photographie stable bornée, BCD/binaire et 12/24 h. Le buffer UTC reste caller-owned et peut être transmis à la politique de dates X.509/TLS. Les données instables, invalides ou insuffisantes sont refusées.

Les tests RTC couvrent BCD 12 h, binaire 24 h, date bissextile, capacité, BCD invalide et bit UIP persistant. Référence : [aos401_408_rtc_utc.md](aos401_408_rtc_utc.md). Batterie RTC, fuseau firmware, siècle matériel, dérive et synchronisation NTP restent hors périmètre.

### AOS-409 à AOS-416 — durcissement X25519/bigint

Le ladder X25519 utilise les opérations modulaires bigint à largeur fixe : échanges XOR masqués, addition/soustraction avec sélection masquée et multiplication parcourant tous les limbs et les bits. Les éléments du ladder conservent huit limbs initialisés ; aucun `kmalloc` n’est introduit.

Les vecteurs X25519 restent verts et le test bigint compare les résultats classiques et constants. Référence : [aos409_416_constant_time_crypto.md](aos409_416_constant_time_crypto.md). L’analyse du compilateur et du microprocesseur, les accès cache, RSA et une preuve de temps constant restent hors périmètre.

### AOS-417 à AOS-424 — chaîne X.509 avec intermédiaire RSA

`x509_certificate_chain_validate_two` vérifie successivement la feuille signée par l’intermédiaire et l’intermédiaire signé par l’ancre. `x509_certificate_tls_identity_validate_two` complète la chaîne avec hostname et dates UTC des trois certificats ; le workspace RSA reste caller-owned et réutilisé.

Les vecteurs DER contiennent une racine RSA, un intermédiaire RSA et une feuille SAN signée. Les tests couvrent la chaîne valide, le sujet émetteur incohérent et la signature intermédiaire tronquée. Référence : [aos417_424_x509_intermediate_chain.md](aos417_424_x509_intermediate_chain.md). BasicConstraints, KeyUsage, pathLen, AKI, révocation, ECDSA et sélection de chaînes longues restent hors périmètre.

### AOS-425 à AOS-432 — contraintes CA des intermédiaires

Le parseur expose `BasicConstraints CA` et `KeyUsage keyCertSign`. La validation `leaf → intermédiaire → ancre` exige maintenant les deux droits sur l’intermédiaire avant toute signature RSA. Les champs DER restent des vues caller-owned.

Les tests vérifient le décodage des extensions, le chemin autorisé et les refus lorsque `CA=true` ou `keyCertSign` manque. Référence : [aos425_432_x509_ca_constraints.md](aos425_432_x509_ca_constraints.md). `pathLenConstraint`, extensions critiques, AKI/SKI, contraintes de nom, révocation, ECDSA et chaînes longues restent hors périmètre.

### AOS-433 à AOS-440 — intermédiaire X.509 dans NE2000/TLS

`ne2k_tls_client_poll_chain_two` compose le polling TCP/TLS existant avec la politique `leaf → intermédiaire → ancre`, en réutilisant les buffers et rollbacks caller-owned. L’API directe demeure inchangée pour les chaînes feuille-ancre.

Le test NE2000 couvre la nouvelle façade et le rejet d’un intermédiaire nul ; les signatures et contraintes CA sont déjà couvertes par les vecteurs X.509. Référence : [aos433_440_ne2k_x509_intermediate.md](aos433_440_ne2k_x509_intermediate.md). Le parsing et la sélection automatique de la liste `Certificate` TLS restent hors périmètre.

### AOS-441 à AOS-448 — intermédiaire de la liste Certificate TLS

Le parser TLS conserve désormais la feuille et le premier intermédiaire de la liste `Certificate`, avec références et longueurs caller-owned. Les deux vues X.509 sont disponibles après parsing ; un DER intermédiaire invalide invalide la feuille pour éviter une utilisation partielle.

Les tests couvrent la liste TLS à deux certificats et les bornes existantes. Référence : [aos441_448_tls_certificate_intermediate.md](aos441_448_tls_certificate_intermediate.md). Sélection de plusieurs intermédiaires, profondeur arbitraire et raccordement automatique à la politique NE2000 restent à implémenter.

### AOS-449 à AOS-456 — chaîne reçue dans NE2000/TLS

`ne2k_tls_client_poll_received_chain` utilise automatiquement la vue du premier intermédiaire reçu par `Certificate` et impose son parsing X.509 avant la politique `leaf → intermédiaire → ancre`. Les rollbacks, buffers et workspaces restent caller-owned.

Le test NE2000 couvre la nouvelle façade et son rejet de client nul. Référence : [aos449_456_ne2k_received_intermediate.md](aos449_456_ne2k_received_intermediate.md). Chaînes multiples, cross-signatures, révocation et sélection automatique de profondeur arbitraire restent hors périmètre.

### AOS-457 à AOS-464 — profondeur de chaîne X.509

Le parseur conserve `pathLenConstraint` comme entier DER positif borné et exige `CA=true` lorsqu’il est présent. La chaîne `leaf → intermédiaire → ancre` refuse maintenant une ancre limitée à zéro, car elle autorise un CA sous-jacent.

Les vecteurs de chaîne testent le refus d’une ancre forcée à `pathLenConstraint=0`. Référence : [aos457_464_x509_pathlen.md](aos457_464_x509_pathlen.md). Profondeur arbitraire, contraintes de nom, AKI/SKI, extensions critiques, ECDSA et révocation restent hors périmètre.

### AOS-465 à AOS-472 — extensions X.509 critiques

Le parser refuse toute extension inconnue marquée critique et vérifie le booléen DER `critical`. SAN, BasicConstraints et KeyUsage restent reconnues et interprétées via des vues caller-owned.

Le test transforme un KeyUsage critique connu en OID inconnu et vérifie le refus. Référence : [aos465_472_x509_critical_extensions.md](aos465_472_x509_critical_extensions.md). AKI/SKI, contraintes de nom, CertificatePolicies, CDP, AIA/OCSP, ECDSA et révocation restent hors périmètre.

### AOS-473 à AOS-480 — liaison AKI/SKI X.509

Le parser expose SKI et le `keyIdentifier` AKI depuis le DER caller-owned. Une chaîne vérifie la correspondance AKI enfant/SKI autorité lorsqu’un AKI est présent, avant la signature RSA.

Les vecteurs root/intermédiaire/leaf vérifient la présence des identifiants et le rejet d’un SKI intermédiaire incompatible. Référence : [aos473_480_x509_aki_ski.md](aos473_480_x509_aki_ski.md). Les autres champs AKI, chaînes multiples, contraintes de nom, ECDSA et révocation restent hors périmètre.


### AOS-481 à AOS-488 — ExtendedKeyUsage `serverAuth` X.509

Le parseur reconnaît maintenant l’extension EKU `2.5.29.37`, en contrôle la séquence DER encapsulée et publie les indicateurs `extended_key_usage_present` et `extended_key_usage_server_auth`. Les politiques TLS directe et avec intermédiaire refusent une feuille dont EKU est présente sans l’OID `serverAuth` (`1.3.6.1.5.5.7.3.1`). Le chemin reste sans allocation dynamique et toutes les vues DER restent caller-owned.

Le vecteur DER de non-régression couvre une EKU valide `serverAuth`, tandis que le test vérifie le refus d’une feuille dont EKU ne permet pas l’authentification de serveur. Validation locale : **367/367 tests**, build i386 et smokes QEMU réussis. Référence : [aos481_488_x509_eku.md](aos481_488_x509_eku.md). `anyExtendedKeyUsage`, les contraintes EKU de CA, ECDSA/EdDSA, révocation et contraintes de nom restent hors périmètre.


### AOS-489 à AOS-496 — contraintes de nom DNS X.509

Le parseur reconnaît désormais `NameConstraints` (`2.5.29.30`) pour les intermédiaires et publie jusqu’à quatre sous-arbres DNS `permittedSubtrees` et `excludedSubtrees`, toujours sous forme de vues DER caller-owned. La politique TLS à intermédiaire refuse l’hôte demandé lorsqu’il est exclu ou absent de la liste des sous-arbres permis. Les comparaisons ASCII sont insensibles à la casse et respectent les frontières de labels ; une contrainte commençant par `.` exige un descendant.

Le sous-ensemble refuse les formes de nom autres que `dNSName`, les distances non nulles, `maximum`, les extensions vides ou dupliquées, afin de ne pas ignorer une contrainte critique. Validation locale : **368/368 tests**, build i386 et smokes QEMU réussis. Référence : [aos489_496_x509_name_constraints.md](aos489_496_x509_name_constraints.md). L’intersection de contraintes sur chaînes profondes, IP/URI/rfc822Name/directoryName, IDNA, révocation et ECDSA restent hors périmètre.


### AOS-497 à AOS-504 — RTC UTC dans la politique NE2000/TLS

La façade `ne2k_tls_client_poll_received_chain_rtc` lit désormais un instant UTC CMOS stable via `rtc_io_t` puis délègue au polling TLS NE2000 utilisant l’intermédiaire reçu. Elle remplace le paramètre UTC caller-owned de ce chemin de production sans modifier les buffers, workspaces ou états TCP/TLS caller-owned. Un échec RTC est retourné avant toute délégation et préserve les sorties du polling.

Le test NE2000 injecte un RTC BCD 12 h valide et une seconde invalide ; les deux chemins de build Unity lient maintenant explicitement `rtc.c`. Validation locale : **368/368 tests**, build i386 et smokes QEMU réussis. Référence : [aos497_504_ne2k_rtc_time_policy.md](aos497_504_ne2k_rtc_time_policy.md). RTC authentifiée, batterie, siècle, dérive, NTP sécurisé et remplacement des APIs UTC historiques restent hors périmètre.


### AOS-505 à AOS-512 — récupération transactionnelle TCP/TLS à retry borné

Le noyau expose un budget de reprise caller-owned (`net_tcp_connection_retry_t`) avec consommation bornée et réouverture TCP en `SYN_SENT`. `ne2k_tls_client_retry_reset` emploie ce budget pour reconstruire la connexion et purger transactionnellement le handshake TLS, les accumulateurs, le transcript, les secrets, le contexte X25519, la session AEAD et les indicateurs d’identité, sans toucher aux pointeurs ni capacités des buffers caller-owned.

Une reprise épuisée retourne `0` sans mutation. Les retransmissions de payload TCP et le retry HTTP LLM restent volontairement séparés. Validation locale : **369/369 tests**, build i386 et smokes QEMU réussis. Référence : [aos505_512_tcp_tls_connection_retry.md](aos505_512_tcp_tls_connection_retry.md). Temporisation, backoff, jitter, SYN automatisé, classification de pannes, session resumption et basculement de fournisseur restent hors périmètre.


### AOS-513 à AOS-520 — streaming LLM SSE sur HTTPS

Le chemin LLM HTTPS prend désormais en charge les requêtes JSON `stream:true`, les réponses HTTP chunked et un sous-ensemble SSE borné : une ligne `data:` par événement, délimitée par `LF LF` ou `CRLF CRLF`, avec extraction des deltas JSON Ollama (`response`) et OpenAI compatible (`content`). Les contextes HTTP/SSE, buffers plaintext, texte, requête et record restent entièrement caller-owned ; `[DONE]` termine le flux sans allocation dynamique.

Les façades NE2000 émettent une requête streaming puis pollent, authentifient, décodent et acquittent les fragments SSE avec rollback de transport en cas d’échec. Validation locale : **371/371 tests**, build i386 et smokes QEMU réussis. Référence : [aos513_520_llm_sse_streaming.md](aos513_520_llm_sse_streaming.md). Commentaires/champs SSE génériques, `id`, `retry`, événements multi-lignes, UTF-8 complet, `Last-Event-ID`, reconnexion SSE, annulation et formats fournisseurs non compatibles restent hors périmètre.


### AOS-521 à AOS-528 — bootstrap DNS/ARP/SYN des hôtes LLM

La façade `ne2k_llm_dns_syn_bootstrap` enchaîne une requête DNS A, le polling DNS borné, la résolution ARP de l’IPv4 obtenue et l’émission du premier SYN TCP. L’IPv4 distante et `net_tcp_connection_t` restent caller-owned et ne sont publiés qu’après succès complet ; les échecs DNS, ARP, framing ou SYN préservent les sorties. Le chemin réussi se termine explicitement en `SYN_SENT`, avant l’acceptation séparée du SYN-ACK et le handshake TLS.

Les tests couvrent budgets DNS non nuls et absence de publication sur attente DNS. Validation locale : **372/372 tests**, build i386 et smokes QEMU réussis, avec une relance locale requise une fois pour une détection NIC QEMU transitoire. Référence : [aos521_528_llm_dns_syn_bootstrap.md](aos521_528_llm_dns_syn_bootstrap.md). AAAA/CNAME/DNSSEC/TTL/cache DNS, DHCP, multi-adresses, backoff/retry SYN, SYN-ACK automatisé et démarrage TLS restent hors périmètre.


### AOS-529 à AOS-536 — SYN-ACK automatique et démarrage TLS LLM

Les façades `ne2k_tls_client_accept_syn_ack_start` et `ne2k_llm_syn_ack_tls_start` relient désormais l’acceptation du SYN-ACK TCP au premier ClientHello TLS. Elles valident SYN-ACK sur des copies locales, démarrent le ClientHello TLS existant, puis publient connexion et client TLS seulement après émission réussie. Le polling NE2000 vide retourne `1` sans mutation ni émission ; le segment TCP transportant ClientHello porte l’acquittement final du handshake TCP.

Les tests couvrent le rollback d’un ACK invalide, la transition TCP `SYN_SENT → ESTABLISHED`, TLS `IDLE → CLIENT_HELLO_SENT`, la progression de séquence et le polling vide. Validation locale : **373/373 tests**, build i386 et smokes QEMU réussis après une relance NE2000 due à une détection NIC QEMU transitoire. Référence : [aos529_536_llm_synack_tls_bootstrap.md](aos529_536_llm_synack_tls_bootstrap.md). Une machine d’état unifiée DNS/SYN/SYN-ACK/TLS, temporisations, retransmission SYN, timeouts et backoff restent hors périmètre.


### AOS-537 à AOS-544 — orchestrateur LLM DNS vers ClientHello

Le contexte caller-owned `ne2k_llm_connection_state_t` publie l’IPv4 résolue et les phases `IDLE`, `SYN_SENT` puis `TLS_STARTED`. `ne2k_llm_connection_start` délègue DNS/ARP/SYN et ne publie l’état qu’après succès complet ; `ne2k_llm_connection_poll_tls_start` délègue SYN-ACK/ClientHello et préserve phase, TCP et TLS sur toute erreur ou RX vide. Le contexte ne conserve ni hostname, ni secret, ni buffer.

Les tests couvrent initialisation, réentrance, erreurs de phase, absence de publication sur échec et pointeur nul. Validation locale : **374/374 tests**, build i386 et smokes QEMU réussis. Référence : [aos537_544_llm_connection_orchestrator.md](aos537_544_llm_connection_orchestrator.md). DNS asynchrone, timeouts, retransmission SYN, backoff, AAAA/CNAME/DNSSEC, DHCP, multi-adresses et machine d’état TLS complète restent hors périmètre.


### AOS-545 à AOS-552 — progression TLS authentifiée de session LLM

Le contexte LLM introduit maintenant `TLS_COMPLETE`. `ne2k_llm_connection_poll_tls` délègue le polling au chemin TLS avec chaîne reçue et UTC RTC, sur copies transactionnelles de l’état LLM, TCP et TLS. Les erreurs négatives ne publient aucun état ; les pollings non négatifs publient leurs progrès, mais la phase ne devient complète qu’après Finished serveur valide et session TLS effectivement complète.

Le test NE2000 couvre le rejet hors phase avant tout accès RTC, réseau ou crypto et vérifie l’absence de mutation de la séquence TCP. Validation locale : **375/375 tests**, build i386 et smokes QEMU réussis. Référence : [aos545_552_llm_tls_session_progress.md](aos545_552_llm_tls_session_progress.md). Timeouts, retransmission, backoff, annulation, chaînes de profondeur arbitraire et orchestration automatique POST/streaming LLM restent hors périmètre.


### AOS-553 à AOS-560 — émission LLM depuis une session TLS complète

La façade `ne2k_llm_connection_request` délègue maintenant la requête Ollama/OpenAI chiffrée au wrapper HTTPS existant uniquement lorsque la session est en phase `TLS_COMPLETE`. Elle utilise l’IPv4 détenue par le contexte, travaille sur copies transactionnelles de l’état LLM/TCP/TLS et publie `REQUEST_SENT` seulement après transmission réussie. Les erreurs JSON, Bearer, HTTP, AES-GCM, TCP ou TX ne publient aucune mutation ; le Bearer reste une entrée caller-owned non stockée.

Le test NE2000 vérifie le rejet avant toute dépendance réseau lorsque la session n’est pas prête et la conservation de la phase ainsi que de la séquence TCP. Validation locale : **375/375 tests**, build i386 et smokes QEMU réussis. Référence : [aos553_560_llm_session_request.md](aos553_560_llm_session_request.md). Polling de réponse dans le contexte, extraction provider, SSE, retry automatique, Unicode, tool calls, multi-tours et persistance de secret restent hors périmètre.


### AOS-561 à AOS-568 — polling et extraction de réponse LLM de session

`ne2k_llm_connection_poll_text` délègue désormais le polling HTTP non-streaming et l’extraction Ollama/OpenAI depuis la phase `REQUEST_SENT`. Il copie transactionnellement contexte LLM, TCP, TLS, accumulateur et vue HTTP : un retour `1` publie les fragments HTTP mais conserve `REQUEST_SENT`, tandis qu’un retour `0` publie le texte et passe à `RESPONSE_READY`. Les erreurs ne publient aucun état et ramènent les longueurs texte/consommation à zéro.

Le test NE2000 couvre le rejet hors phase avant toute dépendance réseau/HTTP/crypto et la préservation des longueurs de sortie. Validation locale : **376/376 tests**, build i386 et smokes QEMU réussis. Référence : [aos561_568_llm_session_response.md](aos561_568_llm_session_response.md). Façade SSE de session, boucle de conversation, réutilisation de connexion, timeout, retry automatique, pagination, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-569 à AOS-576 — façade SSE du contexte LLM
`ne2k_llm_connection_poll_sse` ajoute le streaming LLM au contexte unifié. La façade accepte `REQUEST_SENT` ou `STREAMING`, délègue à `ne2k_https_llm_poll_sse` et publie transactionnellement la connexion TCP, le client TLS, l’accumulateur SSE et les sorties caller-owned. Un retour `1` demeure non bloquant et bascule/conserve `STREAMING`, tandis que le marqueur final du décodeur SSE publie le dernier texte et passe à `RESPONSE_READY`. Les erreurs ne publient aucun état et annulent les longueurs de sortie.
Le test NE2000 couvre le rejet hors phase, la préservation des sentinelles et l’absence de blocage sur RX vide. Validation locale : **377/377 tests**, build i386 et smokes QEMU réussis. Référence : [aos569_576_llm_session_sse.md](aos569_576_llm_session_sse.md). La réinitialisation multi-tours, les timeouts, le retry automatique, la fermeture de session, les tools, le multimodal et l’Unicode complet restent hors périmètre.

### AOS-577 à AOS-584 — réutilisation d’une session TLS LLM
`ne2k_llm_connection_reset_for_request` permet maintenant un second tour sur la même connexion TCP/TLS : elle accepte uniquement `RESPONSE_READY` et publie `TLS_COMPLETE`. L’IPv4 résolue, la connexion TCP, les séquences, les clés TLS et les sessions AES-GCM restent inchangées ; HTTP et SSE sont explicitement réinitialisés par l’appelant dans ses buffers existants avant le polling suivant. Toute phase incomplète, un second appel ou un pointeur nul sont rejetés sans mutation.
Le test NE2000 couvre le refus hors phase, la conservation de l’IPv4, la transition terminale unique et le pointeur nul. Validation locale : **378/378 tests**, build i386 et smokes QEMU réussis. Référence : [aos577_584_llm_session_reuse.md](aos577_584_llm_session_reuse.md). Les headers de connexion persistante, le pipeline, les timeouts, le retry/backoff, la fermeture de session, les tools, le multimodal et l’Unicode complet restent hors périmètre.

### AOS-585 à AOS-592 — plan de contrôle shell/noyau de session LLM
`SYS_LLM_SESSION_STATUS` expose désormais au shell un entier de lecture seule : le bit de préparation NE2000 et la phase du contexte LLM unifié, sans IPv4, buffer, clé TLS ni secret fournisseur. Le noyau initialise ce contexte au boot et `ai-runtime` affiche notamment `Session LLM noyau : IDLE (NE2000 absent)` dans le QEMU sans NIC. Le choix `ai-provider openai` reste un contrôle explicite, sans émettre de requête tant que DHCP/DNS/destination/identifiants n’ont pas été provisionnés hors image.
Le smoke QEMU fournisseur vérifie la phase `IDLE`, tandis que le smoke NE2000 et la suite globale assurent la non-régression. Validation locale : **378/378 tests**, build i386 et smokes QEMU réussis. Référence : [aos585_592_llm_session_control_plane.md](aos585_592_llm_session_control_plane.md). Les syscalls d’émission/polling sous capacité, provisionnement réseau, identifiants, timeout/retry, fermeture de session, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-593 à AOS-600 — identité Ethernet requise par la session LLM
Le boot NE2000 lit maintenant la MAC PROM et la valide avant de publier la carte comme prête. Une MAC nulle, multicast ou illisible empêche `boot_ne2k_present` et donc le statut LLM « prêt » ; le contexte ne peut annoncer `IDLE (NE2000 pret)` que si l’identité Ethernet nécessaire à ARP/DHCP/DNS est effectivement disponible. La MAC n’est ni exposée par le syscall LLM ni imprimée par le shell.
Le smoke QEMU avec `ne2k_isa` vérifie désormais `net-status json` puis `ai-runtime` avec `Session LLM noyau : IDLE (NE2000 pret)` ; le scénario sans NIC conserve `IDLE (NE2000 absent)`. Validation locale : **378/378 tests**, build i386 et smokes QEMU réussis. Référence : [aos593_600_llm_network_identity.md](aos593_600_llm_network_identity.md). DHCP live, DNS, destination, syscalls de service LLM, identifiants, validation TLS, timeout/retry, fermeture de session, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-601 à AOS-608 — acquisition DHCP transactionnelle NE2000
`ne2k_dhcp_acquire` compose maintenant `DISCOVER → OFFER → REQUEST → ACK` au moyen de buffers TX/RX caller-owned et d’un polling RX borné. `ne2k_dhcp_poll_ack` filtre UDP `67 → 68`, valide le XID et les options DHCP, puis ne publie le `net_dhcp_lease_t` qu’après ACK correct. L’orchestrateur utilise un bail local ; toutes les erreurs de garde, transmission, OFFER, REQUEST ou ACK préservent le bail appelant.
Le test NE2000 vérifie les gardes, l’absence de publication du bail sentinelle et le polling ACK invalide ; les codecs DHCP continuent de couvrir OFFER/REQUEST/ACK. Validation locale : **379/379 tests**, build i386 et smokes QEMU réussis. Référence : [aos601_608_dhcp_transactional_acquire.md](aos601_608_dhcp_transactional_acquire.md). Attachement d’un bail au service LLM noyau, route/passerelle/DNS live, endpoint, syscalls d’émission/polling, identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-609 à AOS-616 — routeur et DNS dans le bail DHCP
Le DISCOVER DHCP demande désormais le masque, le routeur et le DNS via l’option de liste de paramètres 55. Le bail caller-owned contient `router_valid`/`router_ipv4` et `dns_valid`/`dns_ipv4`, alimentés par la première adresse des options 3 et 6. Le parsing ACK utilise une copie locale et rejette toute longueur d’option routeur/DNS nulle ou non multiple de quatre, sans modifier le bail déjà publié.
Le vecteur DHCP couvre la liste demandée, routeur, deux DNS, la sélection du premier DNS et le rollback d’une option routeur malformée ; le test NE2000 valide également les nouveaux champs du bail sentinelle. Validation locale : **379/379 tests**, build i386 et smokes QEMU réussis. Référence : [aos609_616_dhcp_route_dns.md](aos609_616_dhcp_route_dns.md). Configuration d’interface/routage, bootstrap automatique DNS/TCP/TLS depuis le bail, service LLM noyau, syscalls d’émission/polling, identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-617 à AOS-624 — masque DHCP et prochain saut réseau
Le bail DHCP conserve désormais `subnet_valid` et `subnet_mask` depuis l’option 1. `net_dhcp_lease_next_hop` choisit transactionnellement la destination lorsqu’elle appartient au sous-réseau ou le routeur DHCP lorsqu’elle est distante ; un bail/masque invalide, l’absence de routeur ou un argument nul rejette la demande sans modifier la sortie. Le parsing ACK rejette aussi les options masque de longueur différente de quatre sans publier de bail partiel.
Le vecteur DHCP couvre `/24`, une destination locale, une destination distante routée via la passerelle et le refus sans mutation lorsque le masque est invalidé ; le test NE2000 conserve le bail sentinelle étendu. Validation locale : **379/379 tests**, build i386 et smokes QEMU réussis. Référence : [aos617_624_dhcp_next_hop.md](aos617_624_dhcp_next_hop.md). Le raccordement de ce prochain saut au transport ARP/DNS/TCP/TLS, le service LLM noyau, les syscalls d’émission/polling, les identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-625 à AOS-632 — UDP et DNS routés par prochain saut
`ne2k_tx_udp_via` résout désormais la MAC du prochain saut ARP tout en conservant une destination IPv4 distincte dans le paquet UDP. `ne2k_dns_query_via` applique ce contrat à DNS ; l’API historique conserve la compatibilité en utilisant l’adresse DNS à la fois comme destination et prochain saut. Tous les buffers ARP/RX/TX restent caller-owned et aucune allocation dynamique n’est introduite.
Le test NE2000 met en cache une passerelle et vérifie que l’émission vers un DNS distant utilise la MAC Ethernet de passerelle tout en conservant l’IPv4 DNS distante. Validation locale : **380/380 tests**, build i386 et smokes QEMU réussis. Référence : [aos625_632_routed_dns_next_hop.md](aos625_632_routed_dns_next_hop.md). Le raccordement du prochain saut à la résolution DNS et au TCP du bootstrap LLM, le service noyau, les syscalls d’émission/polling, les identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-633 à AOS-640 — bootstrap LLM DNS/TCP routé depuis DHCP
`ne2k_tcp_syn_via` sépare la MAC du prochain saut de l’IPv4 du fournisseur pour les SYN TCP. `ne2k_llm_dns_syn_bootstrap_dhcp` sélectionne successivement les routes DNS et hôte depuis le bail DHCP, puis compose DNS A et SYN de façon transactionnelle. `ne2k_llm_connection_start_dhcp` publie `SYN_SENT`, l’IPv4 distante et la connexion seulement après succès complet ; les budgets DNS/ARP restent bornés et caller-owned.
Les tests NE2000 vérifient le SYN via passerelle avec destination IPv4 distante conservée et le rejet transactionnel de la façade DHCP. Validation locale : **382/382 tests**, build i386 et smokes QEMU réussis. Référence : [aos633_640_llm_dhcp_routed_bootstrap.md](aos633_640_llm_dhcp_routed_bootstrap.md). L’acquisition DHCP + démarrage unifiés, le service noyau, les syscalls d’émission/polling, le provisionnement d’endpoint/identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-641 à AOS-648 — façade unifiée DHCP vers session LLM
`ne2k_llm_connection_acquire_start_dhcp` compose maintenant acquisition DHCP et démarrage LLM routé en une opération caller-owned. Bail, contexte LLM et connexion TCP sont copiés localement puis publiés ensemble après DHCP, route DNS, résolution A et SYN réussis. Les erreurs DHCP et bootstrap ne publient aucun état ; une phase autre que `IDLE` est rejetée avant toute opération réseau logique.
Le test NE2000 vérifie la garde de phase du flux unifié et la préservation du bail sentinelle, de `SYN_SENT` et de la séquence TCP. Validation locale : **383/383 tests**, build i386 et smokes QEMU réussis. Référence : [aos641_648_llm_dhcp_unified_start.md](aos641_648_llm_dhcp_unified_start.md). Le syscall noyau contrôlé, le provisionnement d’endpoint/identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-649 à AOS-656 — contexte réseau LLM caller-owned
`ne2k_llm_network_context_t` agrège désormais le bail DHCP, la phase LLM et la connexion TCP persistante sans retenir de buffer, endpoint, secret ou clé. Ses façades initialisent l’état à `IDLE`, délèguent le démarrage DHCP→SYN transactionnel et réarment un tour `RESPONSE_READY → TLS_COMPLETE` tout en préservant le bail et le transport.
Le test NE2000 couvre l’initialisation, le réarmement, la conservation du bail et de la séquence TCP, ainsi que le pointeur nul. Validation locale : **384/384 tests**, build i386 et smokes QEMU réussis. Référence : [aos649_656_llm_network_context.md](aos649_656_llm_network_context.md). Le service noyau persistant, le syscall de démarrage contrôlé, le provisionnement d’endpoint/identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-657 à AOS-664 — contexte réseau LLM dans le plan de contrôle noyau
`boot_llm_network` devient le contexte réseau LLM persistant du noyau et est initialisé à chaque amorçage avant la sonde NE2000. `kernel_llm_session_status()` conserve le bit NE2000 historique et la phase dans les bits 8..15, tout en ajoutant le bit 1 pour signaler uniquement la présence d’un bail DHCP. Le shell `ai-runtime` restitue ce statut sans exposer IPv4, masque, routeur, DNS, endpoint, clé ou identifiant ; le contexte statique et tous les chemins associés restent sans allocation dynamique.
Les smokes QEMU fournisseur et NE2000 vérifient explicitement l’état `IDLE` et un bail absent avant toute acquisition DHCP. Validation locale : **384/384 tests**, build i386 et smokes QEMU réussis ; `git diff --check` est propre. Référence : [aos657_664_llm_kernel_network_context.md](aos657_664_llm_kernel_network_context.md). Les syscalls contrôlés de démarrage DHCP→LLM, de polling TLS, d’émission HTTP et de lecture de réponse, le provisionnement d’endpoint/identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-665 à AOS-672 — syscall contrôlé de démarrage DHCP vers LLM
`SYS_LLM_ACQUIRE_START` reçoit désormais en `EBX` une requête POD bornée contenant seulement hostname, ports, budgets et identifiants de transport non secrets. Le noyau valide le nom, les budgets, les ports, la disponibilité NE2000 et la phase `IDLE`, puis délègue au contexte persistant `boot_llm_network` et à `ne2k_llm_network_context_acquire_start_dhcp`. Les buffers DHCP/ARP/DNS/TCP sont statiques et bornés dans le noyau ; aucune allocation dynamique n’est ajoutée et aucun token, clé, adresse IP ou détail de bail n’est exposé.
La commande `ai-acquire <hostname> [port]` construit une requête avec des valeurs par défaut sûres. Le smoke QEMU fournisseur couvre le rejet de port et l’indisponibilité sans NE2000, puis vérifie que phase `IDLE` et bail absent sont conservés ; le smoke NE2000 reste vert. Référence : [aos665_672_llm_acquire_start_syscall.md](aos665_672_llm_acquire_start_syscall.md). Le polling SYN-ACK/TLS, l’émission HTTP, la lecture de réponse, le provisionnement d’identifiants, TLS live, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-673 à AOS-680 — syscall contrôlé de polling TLS LLM
`SYS_LLM_POLL_TLS` pilote désormais les phases `SYN_SENT` et `TLS_STARTED` sans accepter de buffer, clé, token ou paramètre de confiance depuis Ring 3. Le noyau conserve les espaces de travail TLS/RSA/X25519, les records, le transcript, le hostname non secret, le client TLS et l’interface RTC dans des objets statiques bornés ; `ai-tls-poll` restitue uniquement les résultats de contrôle. Le démarrage DHCP initialise le client TLS, mais le poller refuse `OS_LLM_TLS_UNCONFIGURED` avant tout ClientHello lorsque l’entropie cryptographique et une ancre X.509 de production ne sont pas encore provisionnées : aucune valeur dérivée de l’horloge ou ancre vide n’est acceptée.
Le smoke QEMU fournisseur vérifie le rejet sans NE2000 et la conservation de `IDLE`/bail absent. Référence : [aos673_680_llm_poll_tls_syscall.md](aos673_680_llm_poll_tls_syscall.md). Le provisionnement sécurisé d’entropie et d’ancre, le handshake live complet, l’émission HTTP, la lecture de réponse, les identifiants, timeout/retry, fermeture, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-681 à AOS-688 — syscalls HTTP LLM et polling texte contrôlés
`SYS_LLM_REQUEST` accepte une requête POD bornée (fournisseur, modèle, chemin, prompt) et n’émet un POST JSON chiffré que depuis `TLS_COMPLETE`. `SYS_LLM_POLL_TEXT` copie uniquement le texte fournisseur extrait et le code HTTP dans une sortie bornée ; les buffers JSON/HTTP/TLS, le plaintext, l’accumulateur et les états TCP/TLS restent statiques et privés au noyau, sans allocation dynamique. Ollama peut être émis sans bearer token ; OpenAI est refusé sans provisionnement sécurisé de credential noyau, afin qu’aucun secret n’apparaisse dans le shell, l’ABI ou l’image de boot.
Les commandes `ai-request` et `ai-text-poll` sont couvertes par le smoke QEMU fournisseur : émission refusée avant TLS, polling refusé avant POST, puis conservation de `IDLE`/bail absent. La séquence clavier QEMU encode désormais le caractère `/` des chemins HTTP. Validation locale : **384/384 tests**, build i386 et smokes QEMU réussis. Référence : [aos681_688_llm_http_text_syscalls.md](aos681_688_llm_http_text_syscalls.md). Le provisionnement noyau d’entropie, d’ancre X.509 et de credential OpenAI, le handshake live, timeout/retry, fermeture, SSE via syscall, tools, multimodal et Unicode complet restent hors périmètre.

### AOS-689 à AOS-696 — syscall de streaming SSE LLM contrôlé
Le champ `streaming` de `os_llm_request_t` sélectionne désormais l’émission JSON `stream:true`. `ne2k_llm_connection_stream_request` prolonge la façade de session avec la même garde `TLS_COMPLETE` et la même publication transactionnelle que la requête texte. `SYS_LLM_POLL_SSE` utilise des accumulateurs HTTP chunked/SSE statiques privés au noyau et recopie seulement un delta texte borné avec le statut HTTP ; il accepte `REQUEST_SENT` ou `STREAMING` exclusivement pour une requête réellement streaming et reste non bloquant lorsqu’aucun delta n’est disponible.
Les commandes `ai-stream-request` et `ai-sse-poll` sont couvertes par le smoke QEMU fournisseur : requête streaming avant TLS et polling sans flux refusés, état `IDLE`/bail absent préservés. Validation locale : **384/384 tests**, build i386 et smokes QEMU réussis. Référence : [aos689_696_llm_sse_syscall.md](aos689_696_llm_sse_syscall.md). Le provisionnement noyau d’entropie, d’ancre X.509 et de credential OpenAI, le handshake live, reset/fermeture par syscall, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

### AOS-697 à AOS-704 — syscall de réarmement de session LLM
`SYS_LLM_RESET_FOR_REQUEST` réarme désormais une session terminée uniquement depuis `RESPONSE_READY`, via `ne2k_llm_network_context_reset_for_request`, vers `TLS_COMPLETE`. Le bail DHCP, la connexion TCP, le client TLS, le hostname et les matériaux cryptographiques noyau sont conservés ; les états HTTP/SSE, les sorties texte et les buffers applicatifs transitoires sont purgés afin qu’aucune donnée du tour précédent ne soit relue. La commande sans argument `ai-next` ne transporte aucune donnée réseau, buffer, clé ou secret.
Le smoke QEMU fournisseur vérifie que `ai-next` est refusé depuis `IDLE`, puis confirme phase et bail inchangés ; le smoke NE2000 reste vert. Validation locale : **384/384 tests**, build i386 et smokes QEMU réussis. Référence : [aos697_704_llm_session_reset_syscall.md](aos697_704_llm_session_reset_syscall.md). Le provisionnement noyau d’entropie, d’ancre X.509 et de credential OpenAI, handshake live, fermeture/annulation, timeout/retry, historique, outils, multimodal et Unicode complet restent hors périmètre.

### AOS-705 à AOS-712 — entropie matérielle RDRAND pour TLS
Le noyau détecte désormais `CPUID.01H:ECX.RDRAND` au boot et génère les 32 octets de `client_random` ainsi que la clé privée X25519 de 32 octets par RDRAND avant tout bootstrap DHCP→LLM. Chaque mot dispose de dix tentatives au plus ; absence/échec retourne `OS_LLM_TLS_ENTROPY_UNAVAILABLE`, conserve la phase et interdit tout fallback RTC, TSC ou compteur faible. Un bootstrap ultérieur en erreur efface les matériaux générés. Le bit 2 du statut LLM et `ai-runtime` exposent seulement la disponibilité matérielle, jamais les valeurs d’entropie ou clés.
Le smoke QEMU fournisseur confirme le diagnostic RDRAND indisponible du CPU émulé sans fallback ; le smoke NE2000 reste vert. Validation locale : **384/384 tests**, build i386 et smokes QEMU réussis. Référence : [aos705_712_tls_rdrand_entropy.md](aos705_712_tls_rdrand_entropy.md). Une ancre X.509 RSA SHA-256 de production, les chaînes ECDSA publiques, credential OpenAI, fermeture/annulation, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

### AOS-713 à AOS-720 — ancre X.509 RSA immuable pour TLS
Le noyau embarque désormais l’ancre DER auto-signée ISRG Root X1 dans `kernel/tls_trust_anchor.h` (1 391 octets ; SHA-256 `96bcec06264976f37460779acf28c5a7cfe8a3c0aae11a8ffcee05c0bddf08c6`). Au boot, `x509_certificate_parse` et `x509_rsa_public_key_validate` doivent réussir avant de publier le bit 3 de statut LLM ; aucune commande, syscall, fichier Ring 3 ou téléchargement ne peut modifier cette confiance. Les matériaux TLS RDRAND ne deviennent prêts qu’avec cette ancre validée, et les erreurs de bootstrap effacent les secrets éphémères.
`ai-runtime` affiche uniquement la présence `ISRG Root X1 validee`. Le smoke QEMU fournisseur vérifie ce parsing au boot ; le smoke NE2000 reste vert. Validation locale : **384/384 tests**, build i386 et smokes QEMU réussis. Référence : [aos713_720_tls_immutable_trust_anchor.md](aos713_720_tls_immutable_trust_anchor.md). ECDSA, profondeur de chaîne arbitraire, révocation, second store de confiance, compatibilité intégrale des endpoints publics, credential OpenAI, fermeture/annulation, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

### AOS-721 à AOS-728 — chaîne TLS RSA à deux intermédiaires
Le parser TLS `Certificate` accepte maintenant exactement la feuille et au plus deux intermédiaires, puis rejette une quatrième entrée. `x509_certificate_chain_validate_three` vérifie `leaf → int1 → int2 → ancre` par trois signatures RSA SHA-256, avec CA/`keyCertSign`, AKI/SKI, contraintes `pathLen`, dates UTC, hostname et NameConstraints DNS. L’orchestrateur NE2000 sélectionne obligatoirement cette validation lorsqu’un second intermédiaire est reçu, avant tout flight X25519 ; les structures restent caller-owned et aucune allocation dynamique n’est ajoutée.
Les nouvelles fixtures DER RSA couvrent la chaîne complète ainsi que les rejets `pathLen`, CA et liaison émetteur/sujet ; le parser est couvert pour le second intermédiaire et le refus d’une quatrième entrée. Validation locale : **385/385 tests**, build i386 et smokes QEMU réussis. Référence : [aos721_728_tls_two_intermediate_chain.md](aos721_728_tls_two_intermediate_chain.md). ECDSA, profondeur supérieure, révocation, compatibilité intégrale des endpoints publics, credential OpenAI, fermeture/annulation, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

### AOS-729 à AOS-736 — exponentiation modulaire à exposant multi-limb
`bigint_modexp` accepte désormais un exposant `bigint_t` de largeur bornée et calcule une exponentiation modulaire par square-and-multiply avec quatre segments de workspace caller-owned (résultat, base réduite, produit et temporaire). L’API rejette les capacités insuffisantes, n’utilise ni état global ni allocation dynamique, et conserve `bigint_modexp_u32` pour les appelants existants. Cette primitive rend possibles les futurs inverses modulaires P-256, sans encore introduire certificat, clé, signature ou suite ECDSA.
Le test Unity couvre le vecteur multi-limb `7^(2^32+1) mod 101 = 48` ainsi qu’un workspace trop court. Validation locale : **385/385 tests**, build i386 et smokes QEMU réussis. Référence : [aos729_736_bigint_multilimb_modexp.md](aos729_736_bigint_multilimb_modexp.md). L’arithmétique P-256, la validation de point, ASN.1 ECDSA, X.509 `id-ecPublicKey`, `ServerKeyExchange` ECDSA, l’extension ClientHello, la suite ECDHE_ECDSA, les credentials OpenAI, fermeture/annulation, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

### AOS-737 à AOS-744 — annulation et fermeture contrôlées de session LLM
`SYS_LLM_CLOSE` et `ai-close` annulent désormais une session active sans argument Ring 3. Si TCP est `ESTABLISHED`, le noyau tente le `FIN+ACK` transactionnel NE2000 ; en cas d’échec d’émission, `OS_LLM_CLOSE_FIN_FAILED` confirme que la purge locale a malgré tout été exécutée. L’annulation efface session TCP, IPv4 distante, cache ARP, aléas/clé X25519, secrets TLS/AES-GCM, transcript, workspaces, hostname et tous les buffers HTTP/SSE/prompt/réponse. Seul le bail DHCP validé reste publié ; une session `IDLE` est refusée sans effet.
Le smoke QEMU fournisseur couvre le refus `ai-close` depuis `IDLE` puis vérifie phase et bail inchangés ; le smoke NE2000 reste vert. Validation locale : **385/385 tests**, build i386 et smokes QEMU réussis. Référence : [aos737_744_llm_close_cancel_syscall.md](aos737_744_llm_close_cancel_syscall.md). Le handshake TCP de fermeture complet, TLS `close_notify`, ECDSA, révocation, credentials OpenAI, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

### AOS-745 à AOS-752 — vérification ECDSA P-256/SHA-256 bornée
`ecdsa_p256_sha256_verify` vérifie désormais une signature ECDSA SHA-256 sur secp256r1/P-256 avec une clé SEC1 non compressée de 65 octets. Le module valide le préfixe, les coordonnées et l’appartenance du point public à la courbe, puis parse une `SEQUENCE(INTEGER r, INTEGER s)` DER strictement canonique. Il rejette les scalaires nuls ou hors ordre, les entiers négatifs, les zéros de tête superflus, les tailles incohérentes, les clés hors courbe et les workspaces insuffisants.

L’arithmétique emploie des coordonnées Jacobiennes et calcule `u1·G + u2·Q` après inversion modulaire via `bigint_modexp`. Tous les buffers restent de taille fixe ou détenus par l’appelant ; aucune allocation dynamique n’est ajoutée. Les copies locales des opérandes protègent les opérations bigint qui sont légitimement réalisées en place. Les tests Unity couvrent un vecteur OpenSSL valide, une signature altérée, un point public invalide, un DER non canonique et un workspace insuffisant. Validation locale : **390/390 tests verts**, build i386, smoke `qemu-ai-provider` et smoke `qemu-ne2k-status` réussis. Référence : [aos745_752_ecdsa_p256_verify.md](aos745_752_ecdsa_p256_verify.md). L’extraction X.509 `id-ecPublicKey`, les signatures de certificats ECDSA et la suite TLS ECDHE_ECDSA restent le prochain périmètre.

### AOS-753 à AOS-760 — X.509 ECDSA secp256r1 et signature de certificat
Le parseur X.509 DER publie désormais une vue caller-owned de clé `id-ecPublicKey` sur `secp256r1`/P-256. Il exige l’`AlgorithmIdentifier` exact, le `namedCurve` `prime256v1`, un BIT STRING sans bits inutilisés et un point SEC1 non compressé de 65 octets préfixé par `0x04`. Les certificats RSA historiques gardent leur parsing PKCS#1 et leurs vues module/exposant inchangés ; les clés ECC ne peuvent pas être confondues avec ce chemin.

`x509_certificate_chain_validate_one` sélectionne explicitement RSA/SHA-256 ou `ecdsa-with-SHA256` selon l’algorithme de signature de la feuille. Pour ECDSA, il conserve les liaisons issuer/subject et AKI/SKI, calcule SHA-256 du TBSCertificate DER puis appelle `ecdsa_p256_sha256_verify` avec le workspace de 2 048 mots détenu par l’appelant. Les fixtures DER P-256 racine/feuille signées localement couvrent parsing, identité TLS, signature altérée, clé raccourcie, algorithme tronqué, signature DER raccourcie et workspace insuffisant. Aucun `kmalloc` ni copie de certificat n’est ajouté. Validation locale : **391/391 tests verts**, build i386 et smokes QEMU fournisseur/NE2000 réussis. Référence : [aos753_760_x509_ecdsa_p256.md](aos753_760_x509_ecdsa_p256.md). Les chaînes ECDSA à intermédiaires et la suite TLS ECDHE_ECDSA restent le prochain périmètre.

### AOS-761 à AOS-768 — TLS 1.2 ECDHE_ECDSA avec ServerKeyExchange authentifié
La pile TLS 1.2 annonce désormais `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` (`0xC02B`) avant la suite ECDHE_RSA historique. Le dispatcher authentifié sélectionne strictement le type de clé du certificat et la primitive de signature suivant la suite négociée : P-256 `id-ecPublicKey`/`secp256r1` avec ECDSA/SHA-256 pour `0xC02B`, ou RSA PKCS#1 v1.5/SHA-256 pour `0xC02F`.

`net_tls_handshake_accept_server_key_exchange_ecdsa` vérifie `SHA-256(client_random || server_random || ServerECDHParams)` avec la clé P-256 du certificat, exige `hash=sha256`, `signature=ecdsa`, une signature DER bornée et une clé éphémère X25519 de 32 octets. L’état du handshake et le transcript restent transactionnels : toute signature altérée, suite incohérente, clé incorrecte ou workspace insuffisant est rejeté sans publication de clé éphémère. Les étapes X25519, PRF, AES-GCM et flight client acceptent les deux suites ECDHE authentifiées. Aucun `kmalloc` n’est ajouté. Les régressions incluent un ServerKeyExchange X25519 réellement signé ECDSA, sa falsification et un vecteur TCP RSA/X25519 réel cohérent avec le chemin historique. Validation locale : **392/392 tests verts**, build i386 et smokes QEMU fournisseur/NE2000 réussis. Référence : [aos761_768_tls_ecdhe_ecdsa.md](aos761_768_tls_ecdhe_ecdsa.md). Les chaînes ECDSA à intermédiaires et l’exercice HTTPS de bout en bout restent les prolongements logiques.

### AOS-769 à AOS-776 — Chaîne X.509 ECDSA P-256 à intermédiaire
Une chaîne DER P-256 racine–intermédiaire–feuille, intégralement signée `ecdsa-with-SHA256`, couvre désormais `x509_certificate_chain_validate_two` et `x509_certificate_tls_identity_validate_two`. Les trois clés `id-ecPublicKey`/`secp256r1` sont validées ; les deux signatures, issuer/subject, AKI/SKI, usages CA, `pathLen`, SAN `api.example.test`, EKU serveur et dates doivent tous réussir. Les tests rejettent un workspace inférieur à `ECDSA_P256_WORKSPACE_WORDS`, une signature de feuille altérée, un intermédiaire qui n’est plus CA et une contrainte de chemin racine contradictoire.

Les façades NE2000 de chaîne à intermédiaire appliquaient déjà cette validation transactionnelle avant `peer_identity_validated`; le contexte noyau DHCP→TLS→LLM fournit désormais les **2 048 mots** nécessaires au vérificateur P-256 au lieu des 224 mots historiquement suffisants pour RSA. Le buffer reste statique, caller-owned, effacé à la purge de session et sans `kmalloc`. Validation locale : **393/393 tests verts**, build i386 et smokes QEMU fournisseur/NE2000 réussis. Référence : [aos769_776_x509_ecdsa_intermediate_chain.md](aos769_776_x509_ecdsa_intermediate_chain.md). Le prochain prolongement est un handshake HTTPS contrôlé de bout en bout avec certificat ECDSA et chaîne intermédiaire.

### AOS-777 à AOS-784 — Flight client TLS ECDHE_ECDSA complet
Un scénario Unity unique couvre désormais la chaîne complète `ServerHello (0xC02B) → Certificate P-256 → ServerKeyExchange ECDSA/SHA-256 → ServerHelloDone → ClientKeyExchange X25519 → ChangeCipherSpec → Finished AES-GCM`. Il vérifie le transcript (618 puis 671 octets), les 93 octets de flight client, la dérivation X25519/secret maître/key block AES-128-GCM et le déchiffrement du `Finished` côté serveur. Un tag du `Finished` altéré est refusé sans incrément de `read_sequence`, ce qui maintient l’invariant transactionnel des nonces AEAD.

Le smoke QEMU de contrôle fournisseur IA conserve toutes ses assertions LLM et augmente uniquement son attente initiale de marqueur de boot de 15 à 30 secondes, car le boot complet pouvait atteindre le marqueur après la limite sur runner lent. Aucun `kmalloc` n’est ajouté. Validation locale : **394/394 tests verts**, build i386 et smokes QEMU fournisseur/NE2000 réussis. Référence : [aos777_784_tls_ecdhe_ecdsa_full_flight.md](aos777_784_tls_ecdhe_ecdsa_full_flight.md). Le prochain périmètre logique est le postflight ECDHE_ECDSA avec `ChangeCipherSpec` et `Finished` serveur, avant HTTPS applicatif contrôlé.

### AOS-785 à AOS-808 — transport HTTPS ECDHE_ECDSA de bout en bout
Le scénario TCP couvre désormais la suite ECDHE_ECDSA depuis `ServerHelloDone` jusqu’au postflight serveur `ChangeCipherSpec`/`Finished`, au premier record applicatif AES-GCM et à un POST JSON Ollama chiffré vers `/api/generate`. La variante ECDHE_RSA historique reste testée séparément. Le serveur simulé vérifie l’ouverture AEAD, les séquences TCP, l’ACK, la fenêtre, le plaintext HTTP et la longueur exacte du body.

La fixture respecte le contrat `net_tcp_connection_commit_send`, qui reçoit la longueur du payload TLS et non la longueur totale du segment TCP. Le lanceur global et la règle Make lient `net_http_tls.c` uniquement au test TCP qui exerce les builders HTTP/LLM. Aucun endpoint réel, token ou allocation dynamique n’est ajouté. Validation locale : **395/395 tests verts**, build i386 et smokes QEMU fournisseur/NE2000 réussis. Référence : [aos785_808_https_ecdhe_ecdsa_end_to_end.md](aos785_808_https_ecdhe_ecdsa_end_to_end.md). Le prochain prolongement est le raccordement de cette session au chemin NE2000 recevant réellement les records ECDSA, puis la robustesse applicative.

### AOS-809 à AOS-832 — JSON UTF-8/Unicode pour les échanges LLM
Les builders Ollama/OpenAI, leurs variantes `stream:true` et l’extracteur JSON acceptent désormais l’UTF-8 canonique borné. `net_json_extract_string` décode les échappements `\\uXXXX`, combine les paires surrogate valides en code points non-BMP et produit les octets UTF-8 dans le buffer caller-owned. Les surrogates isolés, séquences surlongues, octets tronqués, hexadécimaux invalides et contrôles JSON non échappés sont rejetés sans publier de sortie partielle.

Le chemin SSE réutilise le même extracteur pour les champs Ollama `response` et OpenAI `content`; aucun second codec ni `kmalloc` n’est introduit. Les builders recopient les séquences UTF-8 valides et continuent d’échapper guillemets, antislashs et contrôles. Les tests couvrent `\\u0041`, `caf\\u00e9`, l’emoji `\\ud83d\\ude00`, UTF-8 brut dans une requête Ollama et prompt tronqué. Validation locale : **396/396 tests verts**, build i386 et smokes QEMU fournisseur/NE2000 réussis. Référence : [aos809_832_json_utf8_unicode_llm.md](aos809_832_json_utf8_unicode_llm.md). Le prochain prolongement est la propagation Unicode dans le service LLM interactif fragmenté et les réponses JSON volumineuses.


### AOS-833 à AOS-840 — réponses LLM fragmentées et propagation Unicode

La sortie texte du service LLM interactif passe de `OS_LLM_TEXT_MAX=512` à `OS_LLM_TEXT_MAX=2048`, toujours dans un POD copié par valeur et sans pointeur utilisateur. Le buffer HTTP noyau de 8 192 octets continue d’accumuler transactionnellement les fragments `Content-Length` avant extraction ; le codec Unicode existant publie les séquences UTF-8 valides, les échappements `\\uXXXX` et les paires surrogate sans sortie partielle.

Le test HTTP/TLS construit une réponse Ollama JSON de 1 015 octets, la fournit en deux fragments et vérifie l’extraction de 1 000 octets, dont un emoji UTF-8 final. Validation locale : **397/397 tests verts**, compilation complète et `git diff --check` attendus propres. Référence : [aos833_840_llm_fragmented_unicode_responses.md](aos833_840_llm_fragmented_unicode_responses.md). La pagination, les tool calls, les objets/tableaux JSON structurés, les réponses supérieures aux buffers fixes, l’augmentation du prompt, l’authentification OpenAI et le streaming SSE multi-événements restent hors périmètre.


### AOS-841 à AOS-848 — capacité de prompt UTF-8 du service LLM

`OS_LLM_PROMPT_MAX` passe de 256 à 1 024 octets. Le prompt reste un champ POD copié par valeur dans l’ABI Ring 3 ; le shell, le noyau et les builders conservent leurs contrôles de capacité, de séquence UTF-8 canonique et de rejet transactionnel. Aucun pointeur utilisateur, secret ou `kmalloc` n’est ajouté.

Le test construit un prompt UTF-8 de 1 020 octets, composé de 1 016 caractères ASCII et d’un emoji `U+1F600`, puis vérifie l’acceptation dans un buffer suffisant et le rejet d’une capacité JSON trop courte. Validation locale : **398/398 tests verts**. Référence : [aos841_848_llm_utf8_prompt_capacity.md](aos841_848_llm_utf8_prompt_capacity.md). Les prompts multi-segments, la pagination, la multimodalité, les pièces jointes et les champs modèle/chemin non ASCII restent hors périmètre.


### AOS-849 à AOS-856 — SSE multi-ligne et UTF-8 fragmenté

L’accumulateur SSE accepte désormais plusieurs lignes `data:` dans un événement et concatène leurs charges utiles avant extraction JSON. Une séquence UTF-8 peut être coupée entre deux appels `net_llm_sse_accumulator_feed` ; l’extraction reste différée jusqu’à l’événement complet, sans publier de sortie partielle. Les lignes SSE qui ne commencent pas strictement par `data:` restent rejetées.

Le test couvre un emoji UTF-8 réparti entre deux fragments et un événement Ollama multi-ligne reconstruit en `bonjour`. Validation locale ciblée : **17/17 tests HTTP/TLS verts**. Référence : [aos849_856_sse_multiline_utf8.md](aos849_856_sse_multiline_utf8.md). Pagination, file de sortie multi-appels, reconnexion SSE, champs `id`/`retry`, commentaires et réponses dépassant les buffers fixes restent hors périmètre.


### AOS-857 à AOS-864 — backoff borné des retries HTTP LLM

`net_llm_http_retry_schedule` combine désormais classification HTTP retryable, budget caller-owned et calcul d’un instant de reprise avec backoff exponentiel borné. Les délais suivent `base`, `2*base`, puis `max`, avec addition saturante à l’instant caller-owned. Aucun timer implicite, blocage, état global, retransmission automatique ou `kmalloc` n’est ajouté ; les erreurs et statuts non-retryables préservent les sorties.

Les tests vérifient les délais 10/20/25, l’épuisement du budget, le statut 2xx et le rejet transactionnel d’un délai nul. Référence : [aos857_864_llm_retry_backoff.md](aos857_864_llm_retry_backoff.md). Timer IRQ, jitter, circuit breaker, backoff réseau automatique et réémission TCP restent hors périmètre.


### AOS-865 à AOS-872 — store bearer sécurisé pour les appels LLM

Ajout d’un store fixe `NET_LLM_BEARER_MAX=128` pour provisionner un bearer ASCII imprimable dans une structure caller-owned. Le builder POST utilise une copie locale terminée par `NUL`, sans getter de secret ; `net_llm_bearer_store_clear` zéroise tout le tableau et invalide l’état. Les tailles invalides et caractères de contrôle sont rejetés sans écraser un store valide. Aucun champ credential n’est ajouté à l’ABI Ring 3 et aucun `kmalloc` n’est utilisé.

Validation ciblée : **19/19 tests HTTP/TLS verts**. Référence : [aos865_872_secure_llm_bearer_store.md](aos865_872_secure_llm_bearer_store.md). Le raccordement à un secret de boot privilégié, au stockage chiffré matériel, au TPM, à la rotation et au chemin HTTPS ECDHE_ECDSA reste hors périmètre immédiat.


### AOS-873 à AOS-880 — raccordement HTTPS du store bearer

Ajout de `net_http_tls_build_post_json_bearer_store`, qui construit le POST Authorization depuis le store bearer fixe puis réutilise le transport TCP et l’encapsulation TLS AES-128-GCM existants. Un store non provisionné est refusé avant émission ; aucun token n’est passé comme chaîne à l’appelant TLS et aucun champ credential n’est exposé à Ring 3.

Le test ouvre une paire TLS de fixture, déchiffre le record côté serveur et compare la requête HTTP complète. Validation ciblée : **20/20 tests HTTP/TLS verts**. Référence : [aos873_880_https_bearer_store.md](aos873_880_https_bearer_store.md). Secret de boot privilégié, coffre matériel/TPM, rotation, persistance chiffrée et sélection dynamique du fournisseur restent hors périmètre immédiat.


### AOS-881 à AOS-888 — scheduler de reconnexion SSE

Ajout d’un état caller-owned `net_llm_sse_reconnect_t` qui conserve le budget, le prochain tick et l’état planifié. `net_llm_sse_reconnect_schedule` réutilise le backoff HTTP borné, réinitialise les accumulateurs HTTP chunked/SSE sans libérer ni remplacer les buffers, et expose une deadline via `net_llm_sse_reconnect_ready`. Aucun sleep, timer global, réémission TCP automatique ou `kmalloc` n’est ajouté.

Les tests couvrent les deadlines 110 et 130 ticks, le reset des longueurs, l’état prêt/non prêt et l’épuisement du budget. Validation ciblée : **21/21 tests HTTP/TLS verts**. Référence : [aos881_888_sse_reconnect_scheduler.md](aos881_888_sse_reconnect_scheduler.md). Last-Event-ID, jitter, timers IRQ intégrés au poller NE2000, reprise au milieu d’un événement et reconnexion automatique multi-fournisseur restent hors périmètre immédiat.


### AOS-897 à AOS-904 — hint `retry:` SSE borné

L’accumulateur SSE accepte désormais `retry:` comme entier décimal strict en millisecondes, borné à `NET_LLM_SSE_RETRY_MAX_MS=600000`. La valeur est exposée par `retry_delay_ms` et `retry_valid`, tandis que les valeurs vides, non numériques ou hors borne sont rejetées sans mutation. Le hint n’est pas appliqué implicitement au scheduler, au timer IRQ, au poller NE2000 ou à une reconnexion automatique.

Le test couvre `retry: 1500` avec delta JSON `ok`, la valeur mémorisée et le rejet de `retry: 600001`. Validation ciblée : **23/23 tests HTTP/TLS verts**. Référence : [aos897_904_sse_retry_hint.md](aos897_904_sse_retry_hint.md). L’application automatique du hint, le jitter, les timers IRQ et la persistance inter-session restent hors périmètre immédiat.

### AOS-913 à AOS-920 — émission NE2000/TLS du GET SSE de reprise
Le chemin matériel dispose maintenant de `ne2k_https_llm_sse_resume_request`. Il valide l’ID `Last-Event-ID` mémorisé par l’accumulateur, construit le GET caller-owned, le chiffre avec la session TLS AES-GCM, suit le segment TCP, transmet via NE2000 puis ne commit la séquence qu’après succès. Toute erreur restaure la connexion et le compteur de séquence TLS. Validation ciblée ajoutée dans `test_ne2k`; aucun `kmalloc` ni buffer global n’est introduit. Le scheduler `retry` reste fourni par `net_llm_sse_reconnect_schedule` et son appel au poller temporel est le prochain axe.

### AOS-921 à AOS-928 — adaptateur temporel du scheduler SSE dans le poller
Le contexte de connexion NE2000 expose désormais `ne2k_llm_connection_schedule_sse_retry` et `ne2k_llm_connection_sse_retry_ready`. Le premier accepte les phases de streaming/réponse, consomme le hint `retry`, programme le tick caller-owned et replace la session en `TLS_COMPLETE` pour permettre l’émission du GET de reprise. Le second reste non bloquant et ne signale prêt qu’après échéance. Validation ciblée ajoutée ; aucun timer global ni allocation dynamique.

### AOS-929 à AOS-936 — orchestration du polling SSE et de la reprise
`ne2k_llm_connection_poll_sse_or_resume` choisit désormais le chemin RX SSE lorsque la session est en streaming et le chemin GET de reprise lorsque la session est en `TLS_COMPLETE` et que le tick `retry` est arrivé. Le chemin non bloquant conserve les longueurs de sortie lorsque la reconnexion n’est pas encore prête ; après émission réussie, il replace la phase en `REQUEST_SENT`. Validation locale : **408/408 tests verts** avant ajout du test d’orchestration, puis test dédié à relancer.

### AOS-937 à AOS-944 — classification et planification des fins SSE/TCP
Le chemin NE2000 distingue désormais progression, fin normale, statut HTTP retryable, erreur transport et erreur terminale. `ne2k_llm_connection_handle_sse_terminal` convertit automatiquement les erreurs retryables et transport en planification SSE bornée, avec `503` comme statut synthétique pour une panne de transport ; une fin normale passe en `RESPONSE_READY` et une erreur terminale reste rejetée. Aucun timer implicite ni allocation dynamique n’est introduit. Validation locale : **409/409 tests verts** après le test de classification.

### AOS-945 à AOS-952 — jitter SSE borné et tick caller-owned
Le scheduler expose désormais `net_llm_sse_reconnect_schedule_jittered`. Une LCG caller-owned produit un décalage déterministe symétrique autour du délai de base, borné par `max_delay`; le résultat réutilise la classification HTTP et le budget de retry existants. Le tick reste fourni par l’appelant, notamment `timer_get_ticks()` côté poller, sans travail bloquant dans l’IRQ. Validation locale : **410/410 tests verts** attendus après le test jitter.

### AOS-953 à AOS-960 — rotation multi-fournisseur après épuisement du budget
Le caller peut désormais basculer explicitement entre Ollama et OpenAI lorsque `retries_used` atteint `retry_limit`. `ne2k_llm_connection_rotate_provider` refuse les fournisseurs inconnus, ne bascule pas avant épuisement et ne modifie qu’un octet provider ; les secrets, l’hôte, le chemin et le modèle restent caller-owned. Validation ciblée ajoutée ; aucune reconnexion implicite ni allocation dynamique.

### AOS-961 à AOS-968 — persistance inter-session minimale de la reprise SSE
Un enregistrement fixe, versionné et caller-owned conserve uniquement le fournisseur, le compteur de retries et `Last-Event-ID`. Une empreinte FNV-1a détecte les corruptions ; les valeurs magiques, la version, la longueur d’ID et le fournisseur sont vérifiés au chargement. Aucun bearer, hôte, chemin, modèle ou secret n’est persisté. Validation locale : **412/412 tests verts**.

### AOS-969 à AOS-976 — politique explicite fournisseur/modèle
`net_llm_model_policy_t` valide un fournisseur connu, un nom de modèle caller-owned imprimable et une autorisation de rotation binaire. La politique ne copie pas le modèle et ne déclenche aucune bascule implicite ; elle fournit un garde avant sérialisation de requête. Validation locale : **413/413 tests verts**.

### AOS-977 à AOS-984 — rattachement de l’identité X.509 au handshake TLS
Le handshake expose désormais une validation d’identité serveur qui réutilise la chaîne X.509, le trust anchor, le hostname et le temps fournis par l’appelant. Elle exige un certificat serveur déjà parsé et validé, ne copie aucun certificat et ne réalise aucune allocation. La vérification ECDSA de `ServerKeyExchange` existante reste inchangée.

### AOS-985 à AOS-992 — postflight TLS ECDHE_ECDSA transactionnel
Le handshake expose désormais une opération unique pour accepter `ChangeCipherSpec` puis vérifier `Finished` serveur. Toute erreur de parsing, d’état ou de verify_data restaure l’état précédent ; l’état complet n’est publié qu’après `SERVER_FINISHED_RECEIVED`, avant l’ouverture HTTPS applicative. Aucun buffer dynamique n’est ajouté.

### AOS-993 à AOS-1000 — garde HTTPS applicatif après handshake ECDHE_ECDSA
Le chemin applicatif expose désormais `net_https_application_ready`, qui exige simultanément `SERVER_FINISHED_RECEIVED` et les quatre pointeurs clé/IV AES-GCM initialisés. Les builders et décodeurs restent caller-owned ; aucun record applicatif ne doit être émis avant ce prédicat. Validation locale : **413/413 tests verts**.

### AOS-1001 à AOS-1008 — émission HTTPS conditionnée par la session TLS complète
Le wrapper `net_https_build_application_record_if_ready` raccorde le prédicat de readiness à la construction AES-GCM. Il refuse toute émission avant `SERVER_FINISHED_RECEIVED` ou avec une session partiellement initialisée, puis délègue sans copie au builder TLS existant. Validation locale : **413/413 tests verts**.

### AOS-1009 à AOS-1016 — réception HTTPS conditionnée par la session TLS complète
Le wrapper `net_https_open_application_record_if_ready` complète le garde d’émission : aucun record entrant n’est déchiffré avant `SERVER_FINISHED_RECEIVED` et l’initialisation complète des clés/IV AES-GCM. La séquence de lecture est déléguée au décodeur TLS existant. Validation locale : **413/413 tests verts**.

### AOS-1017 à AOS-1024 — adaptateur PIT pour le polling SSE
`ne2k_llm_connection_poll_sse_or_resume_now` fournit le tick matériel `timer_get_ticks()` au poller SSE sans déplacer de logique réseau dans l’IRQ0. La référence timer est faible pour garder les tests unitaires autonomes ; le noyau i386 utilise le PIT lié. Tous les buffers et états restent caller-owned, sans allocation dynamique. Validation locale : **413/413 tests verts**.

### AOS-1025 à AOS-1032 — ingestion SSE transactionnelle
`net_llm_sse_response_feed_transactional` restaure les métadonnées HTTP/SSE et `text_length` lorsqu’un feed échoue, sans copier ni remplacer les buffers caller-owned. Les retours positifs et le budget de reprise restent inchangés. Aucun `kmalloc` ni blocage. Validation locale : **413/413 tests verts**.

### AOS-1049 à AOS-1064 — RTO TCP caller-owned borné
`net_tcp_rto_timer_t` fournit une échéance non bloquante et un backoff borné pour les retransmissions TCP. La consommation réutilise `pending_payload` et `retransmit_limit`, restaure timer/connexion sur erreur et ne déplace aucune logique dans IRQ0. Validation locale : **414/414 tests verts**.

### AOS-1065 à AOS-1080 — bail DHCP live borné
Le parseur ACK extrait l’option 51 et le bail caller-owned expose sa durée, son tick d’acquisition, sa validité et son seuil de renouvellement. Les contrôles sont non bloquants, sûrs au wraparound et transactionnels en cas d’option mal formée. Aucun `kmalloc`. Validation locale : **414/414 tests verts**.

### AOS-1081 à AOS-1096 — contrat writer sectoriel FAT16
Le volume FAT16 accepte un writer sectoriel explicite caller-owned, désactivé par défaut, avec contrôle du montage et de la plage LBA. Cette primitive prépare les futures écritures FAT/entrées de répertoire sans introduire LFN, FAT32 ou allocation de clusters prématurément. Validation locale : **415/415 tests verts**.

### AOS-1097 à AOS-1112 — provisionnement OpenAI sécurisé
Ajout de `SYS_LLM_OPENAI_CREDENTIAL` et d’un bearer fixe, borné, validé et effaçable dans le noyau. Le token n’est utilisé que pour OpenAI, jamais retourné ni affiché, et est refusé pendant une session active. Validation locale : **415/415 tests verts**.

### AOS-1113 à AOS-1128 — écriture bornée d’un cluster FAT16
`fat16_write_cluster_range` réalise une lecture-modification-écriture caller-owned dans un cluster existant, avec contrôle du volume, du cluster et de la plage. Aucun cluster n’est alloué et la FAT reste inchangée ; LFN/FAT32 seront ajoutés dans les lots suivants. Validation locale : **415/415 tests verts**.

### AOS-1129 à AOS-1144 — allocation bornée de cluster FAT16
`fat16_allocate_cluster` réserve le premier cluster libre, écrit EOC dans toutes les copies FAT et ne publie le cluster qu’après succès complet. Aucun `kmalloc`; création de fichiers, LFN, chaînes multi-clusters et FAT32 restent des lots supérieurs. Validation locale : **415/415 tests verts**.

### AOS-1145 à AOS-1160 — liaison de chaînes FAT16
`fat16_link_clusters` remplace l’EOC d’un cluster source par une cible déjà allouée et réplique la valeur dans toutes les FAT. L’API refuse les clusters libres, BAD, hors volume et les auto-lien ; aucun cluster ni répertoire n’est créé implicitement. Validation locale : **415/415 tests verts**.

### AOS-1161 à AOS-1176 — création bornée d’une entrée racine FAT16
`fat16_create_root_entry` convertit un nom court conforme au format 8.3 en entrée FAT16 caller-owned, recherche le premier slot libre ou supprimé de la racine, initialise l’attribut, le cluster initial et la taille, puis écrit le secteur par le writer explicite. La conversion refuse les chemins, caractères, extensions ou noms dépassant 8 caractères de base et 3 caractères d’extension ; les entrées LFN ne sont pas générées. Les clusters doivent être préalablement valides et alloués par l’appelant, ce qui évite toute allocation implicite et conserve la politique sans `kmalloc`. Les écritures restent répliquées selon le contrat sectoriel du volume et les erreurs d’I/O n’annoncent jamais une entrée comme créée. Validation locale : **415/415 tests verts**.

La prochaine étape logique est l’orchestration d’un fichier FAT16 persistant combinant allocation, chaîne multi-clusters, initialisation des données et création d’entrée. Le lot suivant devra conserver l’API caller-owned et la stratégie de rollback ; le support LFN et la transition FAT32 restent explicitement séparés.

Voir [aos1161_fat16_root_entry.md](aos1161_fat16_root_entry.md) pour le contrat détaillé, les invariants, les scénarios d’erreur et la matrice de tests.

---

### AOS-1177 à AOS-1192 — orchestration de création d’un fichier FAT16
`fat16_create_file` compose les primitives caller-owned existantes pour réserver une chaîne de clusters, la relier, écrire un buffer de données par plages puis publier une entrée 8.3 dans la racine. En cas d’échec avant publication, la chaîne réservée est parcourue et ses entrées FAT sont libérées dans toutes les copies accessibles. Le fichier vide reçoit un cluster initial afin de rester compatible avec le contrat actuel de création d’entrée. Aucun `kmalloc`, buffer de taille variable ou copie persistante interne n’est ajouté. Validation noyau : **33/33 tests verts** ; la suite complète doit confirmer le total global.

La fonction ne fournit toujours pas de nom long LFN et ne modifie pas les règles FAT32. Elle constitue la fondation du stockage persistant des sessions LLM avant l’ajout des métadonnées LFN et de la généralisation FAT32.

Voir [aos1177_fat16_create_file.md](aos1177_fat16_create_file.md) pour le contrat, la séquence transactionnelle et les limites.

### AOS-1193 à AOS-1208 — sous-lot LFN FAT16 borné
`fat16_create_lfn_file` publie une séquence LFN ASCII bornée en UTF-16LE avec checksum de l’alias 8.3, puis l’entrée courte et les données déjà persistées. `fat16_list_root` valide l’ordre, le checksum et reconstruit le nom long dans `OS_NAME_MAX` sans allocation dynamique. Les caractères non ASCII sont représentés de manière conservative lors du listage ; la recherche directe par nom long dans les APIs de lecture et l’Unicode complet restent le prochain incrément LFN. Validation noyau : **33/33 tests verts** et suite globale **417/417 tests verts**.

Voir [aos1193_fat16_lfn.md](aos1193_fat16_lfn.md).

### AOS-1209 à AOS-1224 — registre socket TCP statique
Le registre caller-owned fournit quatre slots TCP avec ouverture, acceptation SYN-ACK, émission bornée, alimentation RX après validation TCP, lecture et fermeture. Aucun buffer dynamique ni couplage à la session LLM n’est introduit. Validation locale : **418/418 tests verts**. L’exposition par syscalls `socket/connect/send/recv/close`, l’émission NIC et l’écoute passive sont le prochain incrément.

Voir [aos1209_tcp_socket_registry.md](aos1209_tcp_socket_registry.md).

### AOS-1225 à AOS-1240 — syscalls socket TCP utilisateur
Les opérations du registre TCP statique sont exposées par six syscalls ABI avec structures POD caller-owned. Les wrappers imposent une tâche utilisateur, copient la vue SYN-ACK par valeur et conservent les buffers d’émission/réception côté appelant. Validation locale : **418/418 tests verts**. Le durcissement suivant doit valider explicitement les fenêtres d’adresses utilisateur avant les copies, puis ajouter l’écoute passive et l’émission NIC.

Voir [aos1225_socket_syscalls.md](aos1225_socket_syscalls.md).

### AOS-1241 à AOS-1256 — montage et lecture FAT32
Le volume FAT32 valide le BPB, calcule la région de données, lit les entrées FAT 28 bits et restitue un cluster dans un buffer caller-owned. Aucun chemin FAT16 n’est modifié et aucune allocation dynamique n’est introduite. Validation historique du lot : **419 tests verts**. La création/écriture FAT32 et les syscalls de montage ont ensuite été livrés ; les noms longs complets restent partiels.

Voir [aos1241_fat32_mount_read.md](aos1241_fat32_mount_read.md).

### AOS-1257 à AOS-1272 — écriture et chaînage FAT32
Le writer FAT32 caller-owned réalise une lecture-modification-écriture 28 bits, préserve les bits réservés et réplique chaque entrée dans toutes les FAT. L’allocation marque EOC et le chaînage exige une source EOC ainsi qu’une cible déjà allouée. Validation historique du lot : **419 tests verts**. L’écriture de données, le rollback de chaîne et les entrées de répertoire ont ensuite été livrés.

Voir [aos1257_fat32_write_chain.md](aos1257_fat32_write_chain.md).

### AOS-1273 à AOS-1288 — données et entrée racine FAT32
Le système écrit un cluster FAT32 caller-owned et publie une entrée racine 8.3 après validation du nom, de l’attribut, du cluster initial et de la taille. La recherche parcourt la chaîne racine sans allocation implicite. Validation historique du lot : **419 tests verts**. L’extension de la racine, la création transactionnelle et les primitives LFN bornées ont ensuite été livrées, y compris la publication multi-entrée, la lecture, le renommage et la suppression par nom long UTF-8.

Voir [aos1273_fat32_data_root.md](aos1273_fat32_data_root.md).

### AOS-1289 à AOS-1304 — création transactionnelle de fichier FAT32
`fat32_create_file` réserve, écrit et chaîne une ou plusieurs unités FAT32, puis publie l’entrée racine 8.3. Toute erreur libère la chaîne partielle dans toutes les FAT ; le buffer de données reste caller-owned. Validation historique du lot : **419 tests verts**. L’extension automatique de la racine et les primitives LFN bornées ont ensuite été livrées, suivies par la publication de séquences multi-entrée, la lecture, le renommage et la suppression par nom long UTF-8.

Voir [aos1289_fat32_create_file.md](aos1289_fat32_create_file.md).

### AOS-1305 à AOS-1320 — extension automatique du répertoire racine FAT32
Le répertoire racine FAT32 peut être étendu de façon caller-owned : le dernier cluster EOC est trouvé, un cluster est réservé et nettoyé, puis le lien est persisté dans toutes les FAT. En cas d’erreur, le cluster nouveau est libéré. Validation historique du lot : **420 tests verts**. L’encodage LFN borné et le checksum ont ensuite été livrés.

Voir [aos1305_fat32_root_extension.md](aos1305_fat32_root_extension.md).

### AOS-1321 à AOS-1332 — fondations LFN FAT32 bornées
`fat32_lfn_checksum` calcule le checksum de l’alias 8.3 et `fat32_encode_lfn_entry` encode une entrée LFN de 32 octets en UTF-16LE sans allocation dynamique. `fat32_create_lfn_file` publie une séquence multi-entrée dans la chaîne racine, `fat32_list_root` reconstruit le nom dans un buffer caller-owned après validation des ordinals et du checksum, et la lecture, le renommage ainsi que la suppression reconnaissent les noms longs. Le contrat accepte au plus 13 unités UTF-16 par entrée et 20 entrées LFN par fichier, convertit les noms UTF-8 avec paires substituts et conserve l’alias 8.3 en repli si la séquence est invalide. Les tests couvrent la création, la lecture, le listage, le renommage, la suppression et les noms BMP/non-BMP ; seule l’intégration FAT32 au VFS reste distincte.

Voir [aos1321_fat32_lfn.md](aos1321_fat32_lfn.md).

### AOS-1333 à AOS-1344 — fondation TCP d’écoute passive
`net_tcp_connection_listen`, `net_tcp_connection_accept_syn`, `net_tcp_connection_build_syn_ack` et l’extension de `net_tcp_connection_accept_ack` livrent les transitions bornées `LISTEN → SYN_RECEIVED → ESTABLISHED` sans allocation dynamique. Le registre `net_socket` et les syscalls 105–108 exposent `listen`, l’acceptation SYN, la construction caller-owned du SYN-ACK et l’ACK final, avec validation VMM page-par-page. `net_socket_feed` route les segments SYN/ACK passifs, en cohérence avec les vues produites par `ne2k_rx_poll_tcp`. `net_tcp_build_syn_ack_ipv4` produit le paquet IPv4 et ses checksums, tandis que `ne2k_tcp_syn_ack_via` ajoute la résolution ARP bornée, l’encapsulation Ethernet et l’émission TX NE2000. Validation locale : **35/35 tests noyau**, **427/427 tests complets**. Le prochain incrément est la migration du client LLM vers le registre socket générique.

Voir [aos1333_1344_tcp_passive_foundation.md](aos1333_1344_tcp_passive_foundation.md).
