# User Stories — MOHHDY et vision MOHHDY

Deux couches distinctes. Ne pas les mélanger.

| Couche | Document | Statut |
|---|---|---|
| **Prototype qui tourne** | [mohhdy_us.md](mohhdy_us.md) + [docs/ETAT_REEL.md](../docs/ETAT_REEL.md) | AOS-001 à AOS-026 vérifiés ; FAT16/FAT32 avec LFN racine et un sous-répertoire 8.3 mutable via VFS ; réseau local `ai-acquire` ; pas de client OpenAI public |
| **Vision MOHHDY** | fichiers `mohhdy_*.md` + [individual_us/](individual_us/INDEX.md) | Spécifications, sauf incrément Foundation IPC documenté |

En cas de contradiction, **ETAT_REEL** et **mohhdy_us.md** priment.

## Couche 1 — MOHHDY (à utiliser)

Hobby OS i386 32-bit — pas une distribution Linux : boot QEMU, shell Ring 3, overlay AIOV persisté, `spawn`/`yield`/`exec`, GPT-2 124M optionnel. Lexique : [docs/vocabulaire.md](../docs/vocabulaire.md).

- Backlog réel : [mohhdy_us.md](mohhdy_us.md) (`AOS-001` ... `AOS-026` livrés ; FAT16/FAT32 sous-répertoire 8.3 à un niveau ; sockets et `ai-acquire` locaux)
- Runtime : [docs/ETAT_REEL.md](../docs/ETAT_REEL.md)
- Volumes FAT16/FAT32 (LFN racine et mutations 8.3 à un niveau via VFS) : [docs/aos_fat_volume.md](../docs/aos_fat_volume.md)
- Roadmap courte : [README.md](../README.md)

Ce n'est **pas** TensorFlow Lite, pas un microkernel, pas `fake_ai` comme moteur principal (`fake_ai` est un binaire historique ; `ai <texte>` appelle `SYS_GPT2_GENERATE`).

## Couche 2 — MOHHDY (archives de conception)

Plan historique de vision (8 phases, 120 US), désormais sous le nom produit unique MOHHDY. **Soixante-quatre** incréments de **Foundation** sont maintenant livrés (IPC, médiateur de chemins, registre, montages, capacités backend, supervision de tâches). Ils préparent US-001/US-003/US-012/US-013, mais ne déplacent encore ni le stockage, ni les pilotes, ni le réseau hors du noyau ; le noyau reste monolithique. Les volumes FAT16 et FAT32 du prototype publient les LFN à la racine et les mutations 8.3 d’un seul sous-répertoire via le VFS ([docs/aos_fat_volume.md](../docs/aos_fat_volume.md)). Le reseau est un pilote NE2000, des sockets utilisateur et un bootstrap `ai-acquire` sur pair local, pas un service Ring 3 ni un client OpenAI public.

Les autres fichiers MOHHDY restent des **specifications**. Le recouvrement avec le prototype (memoire, tests, moteur IA local, assistant, IPC local, mediateur VFS et decouverte de service) est partiel : voir le tableau dans [individual_us/INDEX.md](individual_us/INDEX.md). Un [OK] dans l'index MOHHDY signifie "fichier de spec present", **pas** "implemente", sauf lorsqu'un statut explicite de tranche livree est indique.

### Fichiers MOHHDY

| Fichier | Contenu |
|---|---|
| [mohhdy_user_stories_master.md](mohhdy_user_stories_master.md) | Index historique des 120 titres (numérotation parfois **différente** des fichiers) |
| [recherche_technologies_mohhdy.md](recherche_technologies_mohhdy.md) | Veille (P2P, federated learning, navigateur-OS) |
| [mohhdy_us_phase1_foundation.md](mohhdy_us_phase1_foundation.md) | Phase 1 détaillée : IPC, capacité et état de service, VFS et métadonnées, découverte, cycle de vie, corrélation, transfert, conservation différée, politique virtuelle, backend réservé, révocation, montages bornés et alias dynamiques, notifications, écriture, suppression, renommage médiés, statistiques locales et lectures source-spécifiques livrés ; microkernel/services séparés non commencés |
| [../docs/mohhdy_foundation_increment_01_ipc.md](../docs/mohhdy_foundation_increment_01_ipc.md) | Conception et contrat de l’incrément IPC Foundation livré |
| [../docs/mohhdy_foundation_increment_02_vfs_service.md](../docs/mohhdy_foundation_increment_02_vfs_service.md) | Médiateur VFS Ring 3 et contrat de lecture IPC livré |
| [../docs/mohhdy_foundation_increment_03_service_registry.md](../docs/mohhdy_foundation_increment_03_service_registry.md) | Registre nommé, découverte `vfs` et limites de sécurité |
| [../docs/mohhdy_foundation_increment_04_service_lifecycle.md](../docs/mohhdy_foundation_increment_04_service_lifecycle.md) | Retrait propriétaire et nettoyage des services terminés |
| [../docs/mohhdy_foundation_increment_05_ipc_correlation.md](../docs/mohhdy_foundation_increment_05_ipc_correlation.md) | Corrélation IPC locale et filtrage VFS borné |
| [../docs/mohhdy_foundation_increment_06_service_grant.md](../docs/mohhdy_foundation_increment_06_service_grant.md) | Transfert limité de publication et nettoyage du bénéficiaire |
| [../docs/mohhdy_foundation_increment_07_ipc_deferred.md](../docs/mohhdy_foundation_increment_07_ipc_deferred.md) | Conservation FIFO bornée des messages IPC non corrélés côté Ring 3 |
| [../docs/mohhdy_foundation_increment_08_vfs_virtual_policy.md](../docs/mohhdy_foundation_increment_08_vfs_virtual_policy.md) | Source VFS virtuelle servie par le médiateur Ring 3 |
| [../docs/mohhdy_foundation_increment_09_vfs_backend.md](../docs/mohhdy_foundation_increment_09_vfs_backend.md) | Backend VFS réservé au propriétaire courant du nom `vfs` |
| [../docs/mohhdy_foundation_increment_10_vfs_revocation.md](../docs/mohhdy_foundation_increment_10_vfs_revocation.md) | Transfert du médiateur VFS et révocation effective de l’ancien propriétaire |
| [../docs/mohhdy_foundation_increment_11_vfs_mounts.md](../docs/mohhdy_foundation_increment_11_vfs_mounts.md) | Montage `initrd/` déclaré, sources virtuelles et refus des chemins hors préfixe |
| [../docs/mohhdy_foundation_increment_12_service_notifications.md](../docs/mohhdy_foundation_increment_12_service_notifications.md) | Abonnements de service bornés et événements IPC best-effort de changement de propriétaire |
| [../docs/mohhdy_foundation_increment_13_vfs_write.md](../docs/mohhdy_foundation_increment_13_vfs_write.md) | Montage `overlay/ rw`, écriture IPC corrélée et backend réservé au propriétaire de `vfs` |
| [../docs/mohhdy_foundation_increment_14_vfs_source_reads.md](../docs/mohhdy_foundation_increment_14_vfs_source_reads.md) | Lectures initrd/overlay distinctes et réservées au propriétaire de `vfs` |
| [../docs/mohhdy_foundation_increment_15_vfs_remove.md](../docs/mohhdy_foundation_increment_15_vfs_remove.md) | Suppression `overlay/` IPC corrélée et réservée au propriétaire de `vfs` |
| [../docs/mohhdy_foundation_increment_16_vfs_rename.md](../docs/mohhdy_foundation_increment_16_vfs_rename.md) | Renommage `overlay/` IPC corrélé entre deux chemins réservés au propriétaire de `vfs` |
| [../docs/mohhdy_foundation_increment_17_vfs_stats.md](../docs/mohhdy_foundation_increment_17_vfs_stats.md) | Compteurs VFS volatils exposés par une source virtuelle, sans nouveau syscall |
| [../docs/mohhdy_foundation_increment_18_vfs_dynamic_mounts.md](../docs/mohhdy_foundation_increment_18_vfs_dynamic_mounts.md) | Alias initrd/overlay dynamiques, corrélés, bornés et locaux au serveur VFS |
| [../docs/mohhdy_foundation_increment_19_service_capacity.md](../docs/mohhdy_foundation_increment_19_service_capacity.md) | Limite IPC de deux messages clients pour un propriétaire de service publié |
| [../docs/mohhdy_foundation_increment_20_service_status.md](../docs/mohhdy_foundation_increment_20_service_status.md) | Instantané public PID/profondeur/capacités d’un service vivant |
| [../docs/mohhdy_foundation_increment_21_vfs_stat.md](../docs/mohhdy_foundation_increment_21_vfs_stat.md) | Métadonnées VFS corrélées et source-spécifiques par montage |
| [../docs/mohhdy_foundation_increment_32_vfs_backend_status.md](../docs/mohhdy_foundation_increment_32_vfs_backend_status.md) | Consultation médiée d’un masque backend VFS par le propriétaire public |
| [../docs/mohhdy_foundation_increment_33_vfs_backend_list.md](../docs/mohhdy_foundation_increment_33_vfs_backend_list.md) | Inventaire médié, corrélé et borné des délégations backend VFS actives |
| [mohhdy_us_phase2_ai_core.md](mohhdy_us_phase2_ai_core.md) | Phase 2 (TensorFlow Lite, NLU, fédéré) — non livrée ; l'IA réelle est GPT-2 freestanding |
| [mohhdy_us_phase3_web_runtime.md](mohhdy_us_phase3_web_runtime.md) | Phase 3 navigateur-OS — absente |
| [mohhdy_us_phases_4_8_synthese.md](mohhdy_us_phases_4_8_synthese.md) | Phases 4-8 (PromptMessage, P2P, etc.) — absentes |
| [individual_us/](individual_us/INDEX.md) | ~78 fichiers de spec ; IDs **023/024/025 dupliqués** ; pas 120 fichiers |

### Phases MOHHDY (rappel)

1. Foundation — microkernel, plugins, logging distribué  
2. AI Core — TFLite, NLU, apprentissage fédéré, cloud-edge  
3. Web Runtime — navigateur comme FS  
4. PromptMessage — langage universel  
5. P2P Network  
6. Multi-platform  
7. Collaborative (points)  
8. Production  

La migration complète de US-001 reste une refonte à haut risque : les incréments actuels fournissent IPC avec capacité locale et instantané de propriétaire de service, médiateur VFS de lecture-écriture-suppression-renommage avec lectures et métadonnées source-spécifiques, statistiques locales et alias de montage dynamiques bornés, découverte de nom, nettoyage de cycle de vie, corrélation locale, conservation différée bornée, transfert de propriété, politique virtuelle, révocation du droit d’accès au backend et notifications best-effort.
 La suite doit introduire une identité vérifiée et des capabilities, des événements accusés ou persistants, des montages persistants associés à des services, externaliser le backend VFS lui-même, puis déplacer pilotes ou réseau derrière ces droits, sans affirmer prématurément que ces composants sont déjà hors du noyau.

## Contribution

1. Une PR = une tranche visible par `make ci` (code) **ou** un alignement doc (ce dossier).  
2. Nouvelles user stories du prototype : les ajouter dans `mohhdy_us.md` (préfixe `AOS-`), pas en renumérotant MOHHDY.  
3. Les specs MOHHDY peuvent rester ; mettre à jour seulement la bannière « état réel » si le recouvrement change.
