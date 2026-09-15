# User Stories - SE Mohhdy (un produit)

**Un seul produit** : Mohhdy, le SE agentique autonome. Il n'y a pas trois couches produit, pas de piste parallele, pas de widget SaaS a cote du noyau.

Deux **niveaux de maturite runtime** (ingenierie, pas deux produits), plus des archives de spec :

| Niveau | Document | Statut |
|---|---|---|
| **Prototype guest** | [mohhdy_us.md](mohhdy_us.md) + [docs/ETAT_REEL.md](../docs/ETAT_REEL.md) | Tranche noyau i386 Multiboot **mesuree** : AOS-001 a AOS-026 verifies ; FAT16/FAT32 ; reseau local `ai-acquire` ; pas de client OpenAI public |
| **Instance OS autonome** | [mohhdy_agent_support_web.md](mohhdy_agent_support_web.md) + [mohhdy_os_ui_migration.md](mohhdy_os_ui_migration.md) | Backlog de **capacites OS** (`ASSIST-xxx`) portees dans le chrome `osui/` (chat central, slash, panes ; OS-UI-0/1/2 premieres tranches) avec scaffold `agent/` temporaire. Stub local. Playwright = profil optionnel. LLM de production, US-031 et billing **non livres**. Prochain : OS-UI-3 |
| **Specs historiques** | fichiers `mohhdy_*.md` + [individual_us/](individual_us/INDEX.md) | Archives de conception (phases 1-8). Un `[OK]` = fichier present, **pas** "implemente" |

En cas de contradiction sur ce qui **tourne dans le guest**, **ETAT_REEL** et **mohhdy_us.md** priment. L'intention produit et le **catalogue complet** sont dans [docs/PLAN_SE_MOHHDY_COMPLET.md](../docs/PLAN_SE_MOHHDY_COMPLET.md) (premieres tranches OS-UI-0/1/2 : [docs/osui_0_1_2.md](../docs/osui_0_1_2.md), [docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md)). Gardes guest : [docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md). Portage ASSIST vers OS graphique : [mohhdy_os_ui_migration.md](mohhdy_os_ui_migration.md).

## Prototype guest (tranche noyau verifiee)

Chemin pedagogique i386 32-bit sous QEMU : pas une distribution Linux, boot Multiboot, shell Ring 3, overlay AIOV persiste, `spawn`/`yield`/`exec`, GPT-2 124M optionnel. Lexique : [docs/vocabulaire.md](../docs/vocabulaire.md).

- Backlog guest : [mohhdy_us.md](mohhdy_us.md) (`AOS-001` ... `AOS-026` livres ; FAT16/FAT32 sous-repertoire 8.3 a un niveau ; sockets et `ai-acquire` locaux)
- Runtime mesure : [docs/ETAT_REEL.md](../docs/ETAT_REEL.md)
- Volumes FAT16/FAT32 : [docs/aos_fat_volume.md](../docs/aos_fat_volume.md)
- Roadmap produit : [README.md](../README.md)
- Suite d'implementation : [docs/PLAN_SE_MOHHDY_COMPLET.md](../docs/PLAN_SE_MOHHDY_COMPLET.md) (roadmap produit) et [docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md) (gardes guest 0-4)

Ce n'est **pas** TensorFlow Lite, pas un microkernel, pas `fake_ai` comme moteur principal (`fake_ai` est un binaire historique ; `ai <texte>` appelle `SYS_GPT2_GENERATE`).

## Specs historiques (archives de conception)

Plan historique de vision (8 phases, 120 US), désormais sous le nom produit unique MOHHDY. **Soixante-quatre** incréments de **Foundation** sont maintenant livrés (IPC, médiateur de chemins, registre, montages, capacités backend, supervision de tâches). Ils préparent US-001/US-003/US-012/US-013, mais ne déplacent encore ni le stockage, ni les pilotes, ni le réseau hors du noyau ; le noyau reste monolithique. Les volumes FAT16 et FAT32 du prototype publient les LFN à la racine et les mutations 8.3 d'un seul sous-répertoire via le VFS ([docs/aos_fat_volume.md](../docs/aos_fat_volume.md)). Le reseau est un pilote NE2000, des sockets utilisateur et un bootstrap `ai-acquire` sur pair local, pas un service Ring 3 ni un client OpenAI public.

Les autres fichiers MOHHDY restent des **specifications**. Le recouvrement avec le prototype (memoire, tests, moteur IA local, assistant, IPC local, mediateur VFS et decouverte de service) est partiel : voir le tableau dans [individual_us/INDEX.md](individual_us/INDEX.md). Un [OK] dans l'index MOHHDY signifie "fichier de spec present", **pas** "implemente", sauf lorsqu'un statut explicite de tranche livree est indique.

### Fichiers MOHHDY

| Fichier | Contenu |
|---|---|
| [mohhdy_user_stories_master.md](mohhdy_user_stories_master.md) | Index historique des 120 titres (numérotation parfois **différente** des fichiers) |
| [recherche_technologies_mohhdy.md](recherche_technologies_mohhdy.md) | Veille (P2P, federated learning, navigateur-OS) |
| [mohhdy_us_phase1_foundation.md](mohhdy_us_phase1_foundation.md) | Phase 1 détaillée : IPC, capacité et état de service, VFS et métadonnées, découverte, cycle de vie, corrélation, transfert, conservation différée, politique virtuelle, backend réservé, révocation, montages bornés et alias dynamiques, notifications, écriture, suppression, renommage médiés, statistiques locales et lectures source-spécifiques livrés ; microkernel/services séparés non commencés |
| [../docs/mohhdy_foundation_increment_01_ipc.md](../docs/mohhdy_foundation_increment_01_ipc.md) | Conception et contrat de l'incrément IPC Foundation livré |
| [../docs/mohhdy_foundation_increment_02_vfs_service.md](../docs/mohhdy_foundation_increment_02_vfs_service.md) | Médiateur VFS Ring 3 et contrat de lecture IPC livré |
| [../docs/mohhdy_foundation_increment_03_service_registry.md](../docs/mohhdy_foundation_increment_03_service_registry.md) | Registre nommé, découverte `vfs` et limites de sécurité |
| [../docs/mohhdy_foundation_increment_04_service_lifecycle.md](../docs/mohhdy_foundation_increment_04_service_lifecycle.md) | Retrait propriétaire et nettoyage des services terminés |
| [../docs/mohhdy_foundation_increment_05_ipc_correlation.md](../docs/mohhdy_foundation_increment_05_ipc_correlation.md) | Corrélation IPC locale et filtrage VFS borné |
| [../docs/mohhdy_foundation_increment_06_service_grant.md](../docs/mohhdy_foundation_increment_06_service_grant.md) | Transfert limité de publication et nettoyage du bénéficiaire |
| [../docs/mohhdy_foundation_increment_07_ipc_deferred.md](../docs/mohhdy_foundation_increment_07_ipc_deferred.md) | Conservation FIFO bornée des messages IPC non corrélés côté Ring 3 |
| [../docs/mohhdy_foundation_increment_08_vfs_virtual_policy.md](../docs/mohhdy_foundation_increment_08_vfs_virtual_policy.md) | Source VFS virtuelle servie par le médiateur Ring 3 |
| [../docs/mohhdy_foundation_increment_09_vfs_backend.md](../docs/mohhdy_foundation_increment_09_vfs_backend.md) | Backend VFS réservé au propriétaire courant du nom `vfs` |
| [../docs/mohhdy_foundation_increment_10_vfs_revocation.md](../docs/mohhdy_foundation_increment_10_vfs_revocation.md) | Transfert du médiateur VFS et révocation effective de l'ancien propriétaire |
| [../docs/mohhdy_foundation_increment_11_vfs_mounts.md](../docs/mohhdy_foundation_increment_11_vfs_mounts.md) | Montage `initrd/` déclaré, sources virtuelles et refus des chemins hors préfixe |
| [../docs/mohhdy_foundation_increment_12_service_notifications.md](../docs/mohhdy_foundation_increment_12_service_notifications.md) | Abonnements de service bornés et événements IPC best-effort de changement de propriétaire |
| [../docs/mohhdy_foundation_increment_13_vfs_write.md](../docs/mohhdy_foundation_increment_13_vfs_write.md) | Montage `overlay/ rw`, écriture IPC corrélée et backend réservé au propriétaire de `vfs` |
| [../docs/mohhdy_foundation_increment_14_vfs_source_reads.md](../docs/mohhdy_foundation_increment_14_vfs_source_reads.md) | Lectures initrd/overlay distinctes et réservées au propriétaire de `vfs` |
| [../docs/mohhdy_foundation_increment_15_vfs_remove.md](../docs/mohhdy_foundation_increment_15_vfs_remove.md) | Suppression `overlay/` IPC corrélée et réservée au propriétaire de `vfs` |
| [../docs/mohhdy_foundation_increment_16_vfs_rename.md](../docs/mohhdy_foundation_increment_16_vfs_rename.md) | Renommage `overlay/` IPC corrélé entre deux chemins réservés au propriétaire de `vfs` |
| [../docs/mohhdy_foundation_increment_17_vfs_stats.md](../docs/mohhdy_foundation_increment_17_vfs_stats.md) | Compteurs VFS volatils exposés par une source virtuelle, sans nouveau syscall |
| [../docs/mohhdy_foundation_increment_18_vfs_dynamic_mounts.md](../docs/mohhdy_foundation_increment_18_vfs_dynamic_mounts.md) | Alias initrd/overlay dynamiques, corrélés, bornés et locaux au serveur VFS |
| [../docs/mohhdy_foundation_increment_19_service_capacity.md](../docs/mohhdy_foundation_increment_19_service_capacity.md) | Limite IPC de deux messages clients pour un propriétaire de service publié |
| [../docs/mohhdy_foundation_increment_20_service_status.md](../docs/mohhdy_foundation_increment_20_service_status.md) | Instantané public PID/profondeur/capacités d'un service vivant |
| [../docs/mohhdy_foundation_increment_21_vfs_stat.md](../docs/mohhdy_foundation_increment_21_vfs_stat.md) | Métadonnées VFS corrélées et source-spécifiques par montage |
| [../docs/mohhdy_foundation_increment_32_vfs_backend_status.md](../docs/mohhdy_foundation_increment_32_vfs_backend_status.md) | Consultation médiée d'un masque backend VFS par le propriétaire public |
| [../docs/mohhdy_foundation_increment_33_vfs_backend_list.md](../docs/mohhdy_foundation_increment_33_vfs_backend_list.md) | Inventaire médié, corrélé et borné des délégations backend VFS actives |
| [mohhdy_os_ui_migration.md](mohhdy_os_ui_migration.md) | Epiques `OS-UI-xxx` : chrome graphique (OS-UI-0), portage chat/admin/droits (1), actes navigateur-OS (2), retrait Python (3) |
| [mohhdy_agent_support_web.md](mohhdy_agent_support_web.md) | Capacites OS (`ASSIST-xxx`) : support sur le web, navigateur-OS du SE, autonomie. Scaffold `agent/` a migrer. ASSIST-010..022/030/031/040/041/051/052/053/060/061 livres **dans le bootstrap** avec limites (stub, Playwright optionnel, pas US-031, pas de billing) |
| [mohhdy_us_phase2_ai_core.md](mohhdy_us_phase2_ai_core.md) | Phase 2 (TensorFlow Lite, NLU, federé) - non livree ; l'IA reelle du guest est GPT-2 freestanding |
| [mohhdy_us_phase3_web_runtime.md](mohhdy_us_phase3_web_runtime.md) | Navigateur-OS du SE : **devoir du produit**, pas une vision lointaine. US-031 **non livre**. Bootstrap ASSIST-060/061 dans `agent/` (Playwright optionnel) |
| [mohhdy_us_phases_4_8_synthese.md](mohhdy_us_phases_4_8_synthese.md) | Phases 4-8 (PromptMessage, P2P, etc.) - absentes |
| [individual_us/](individual_us/INDEX.md) | ~78 fichiers de spec ; IDs **023/024/025 dupliqués** ; pas 120 fichiers |

### Phases MOHHDY (rappel)

1. Foundation - microkernel, plugins, logging distribué
2. AI Core - TFLite, NLU, apprentissage fédéré, cloud-edge
3. Web Runtime - navigateur-OS du SE (coeur produit, US-031 non livre)
4. PromptMessage - langage universel
5. P2P Network
6. Multi-platform
7. Collaborative (points)
8. Production

Les tickets `ASSIST-xxx` ne sont **pas** une 9e phase de ce plan historique, ni un produit distinct. Ce sont des **capacites du SE Mohhdy** (droits, assistant, navigateur-OS, deploiement). Docker / PC / hyperviseur = deploiement de l'instance, pas un sidecar.

La migration complete de US-001 reste une refonte a haut risque : les increments actuels fournissent IPC avec capacite locale et instantane de proprietaire de service, mediateur VFS de lecture-ecriture-suppression-renommage avec lectures et metadonnees source-specifiques, statistiques locales et alias de montage dynamiques bornes, decouverte de nom, nettoyage de cycle de vie, correlation locale, conservation differee bornee, transfert de propriete, politique virtuelle, revocation du droit d'acces au backend et notifications best-effort.
 La suite guest doit introduire une identite verifiee et des capabilities, des evenements accuses ou persistants, des montages persistants associes a des services, externaliser le backend VFS lui-meme, puis deplacer pilotes ou reseau derriere ces droits, sans affirmer prematurement que ces composants sont deja hors du noyau.

## Instance OS autonome (backlog de capacites OS)

Le SE Mohhdy doit offrir le support sur le web, agir dans le navigateur-OS du SE, et rester autonome. Spec fonctionnelle : [mohhdy_agent_support_web.md](mohhdy_agent_support_web.md) (`ASSIST-000` a `ASSIST-090`). Ordre de **portage dans l'OS graphique** : [mohhdy_os_ui_migration.md](mohhdy_os_ui_migration.md) et [docs/PLAN_SE_MOHHDY_COMPLET.md](../docs/PLAN_SE_MOHHDY_COMPLET.md). Premieres tranches OS-UI-0/1/2 : [docs/osui_0_1_2.md](../docs/osui_0_1_2.md), [docs/osui_chat_desktop.md](../docs/osui_chat_desktop.md). Prochain : **OS-UI-3**.

Docker lance l'instance **comme une machine vierge**. `agent/` est le scaffold userspace **temporaire**. Le navigateur-OS est un **devoir du SE**. US-031 n'est **pas** livre.

**Honnêteté runtime.** Le guest i386 verifie n'heberge ni widget, ni Docker, ni admin, ni automatisation navigateur. Ils ne tiennent pas dans un noyau Multiboot nu. Le scaffold HTTP / Docker ([docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md), [docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md)) porte ces capacites hors guest : sante, sessions isolees, `embed.js`, admin a jeton. Les reponses sont un echo stub, pas un LLM de production. `make ci` et `make integration-qemu` restent inchanges. OpenAI public reste sous condition.

**Relation aux phases 1-8.** Reprend droits (phase 1), assistant (phase 2, US-021 / US-028), navigateur comme FS (phase 3, **coeur produit**, US-031 non livre), deploiement (US-015, phases 6 et 8) et connecteurs bornes (US-034). Ne renumerote pas `US-xxx`. Ne recycle pas `AOS-xxx`.

## Contribution

1. Une PR = une tranche visible par `make ci` (code) **ou** un alignement doc (ce dossier).
2. Nouvelles user stories du **guest** : les ajouter dans `mohhdy_us.md` (prefixe `AOS-`), pas en renumerotant les specs historiques.
3. Les specs historiques peuvent rester ; mettre a jour seulement la banniere "etat reel" si le recouvrement change.
4. Nouvelles **capacites OS** : spec dans `mohhdy_agent_support_web.md` (prefixe `ASSIST-`) ; portage dans `mohhdy_os_ui_migration.md` (prefixe `OS-UI-`), sans `AOS-` et sans renumeroter `US-xxx`.
