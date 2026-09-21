# Plan de suite d'implémentation (gardes guest)

**Date :** 15 septembre 2026
**Statut :** gardes noyau du guest i386, pas la feuille de route produit complete
**Ponctuation :** ASCII usuel et accents français uniquement

**Roadmap produit (toutes les capacites visees, portage OS+UI) :** [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md). Epiques de migration : [../US/mohhdy_os_ui_migration.md](../US/mohhdy_os_ui_migration.md).

Ce document **ne** remplace **pas** le plan maitre. Il detaille seulement les **tranches 0-4** : gardes du guest i386 mesure (CI, ACL, GGUF, stockage). Un seul produit : le SE Mohhdy. Docker / PC / hyperviseur = boot de l'instance QEMU, pas un sidecar. La surface OS-UI vit dans `userspace/osui_runtime.c` (OS-UI-0 a 3 livres, facade Python metier retiree, commande `gui` + bureau VBE QEMU). Les tickets `ASSIST-xxx` restent la spec fonctionnelle ([../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md)). LLM de production, Chromium de session et US-031 **ne sont pas** livres. En cas de contradiction sur le **guest**, [ETAT_REEL.md](ETAT_REEL.md) et [../US/mohhdy_us.md](../US/mohhdy_us.md) priment. En cas de contradiction sur l'ordre **produit**, le plan maitre prime.

**Prochain build produit :** gardes 0-4 et honnetete US-031. OS-UI-3 (facade Python metier) est livre. Bureau VBE QEMU : [osui_0_1_2.md](osui_0_1_2.md), [osui_chat_desktop.md](osui_chat_desktop.md). `guest_html_stage=false`. `display_host=false`.

## Sources lues (sans les réécrire)

- [../US/mohhdy_us.md](../US/mohhdy_us.md), section "Prochaines tranches, hors livraison actuelle"
- [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md), catalogue et tranches OS-UI
- [../US/mohhdy_os_ui_migration.md](../US/mohhdy_os_ui_migration.md), epiques OS-UI-000 a OS-UI-3
- [../README.md](../README.md), section "Roadmap du SE (un produit)" (items ouverts ou partiels)
- [../US/README.md](../US/README.md), un produit, deux niveaux de maturite
- [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md), backlog de capacites OS (`ASSIST-xxx`) a porter dans l'OS+UI
- [../US/mohhdy_us_phase3_web_runtime.md](../US/mohhdy_us_phase3_web_runtime.md), navigateur-OS du SE (devoir produit, US-031 non livre)
- [ETAT_REEL.md](ETAT_REEL.md), état vérifié et prochaines tranches priorisées
- [todo.md](todo.md), journal de livraison (compteurs historiques)
- [BILAN_MASTER.md](BILAN_MASTER.md), snapshot `9078d5f` / 13 septembre 2026

## Un produit, deux niveaux de maturite

Ne pas vendre trois produits. Un item de spec historique n'est pas un ticket de build du guest. Un epic `ASSIST-xxx` n'est pas un `AOS-xxx`, mais c'est **la meme instance Mohhdy**.

| Niveau | Perimetre | Role dans ce plan |
|---|---|---|
| **Prototype guest** | i386 Multiboot, QEMU, shell Ring 3, FAT, NE2000 local, GPT-2 / GGUF local | Tranches 0-4. Seule tranche **verifiee** dans ETAT_REEL |
| **Instance OS autonome** | Docker / PC / hyperviseur / machine vierge ; meme guest C sous QEMU | Devoirs ASSIST portes (OS-UI-0..3 livres). Chromium / LLM prod non livres |
| **Specs historiques** | `US/mohhdy_*.md` et `US/individual_us/` (US-001, US-016 TFLite, P2P, etc.) | Archives. Pas des cibles de build courantes du guest |

Le guest verifie AOS-001 a AOS-026, plus les lots reseau / VFS / GGUF documentes dans ETAT_REEL. Ce n'est pas TensorFlow Lite, pas un microkernel abouti, pas un client OpenAI public, pas Internet. Ce n'est **pas** non plus "le SE autonome deja complet" : widget, Docker et navigateur-OS ne tournent pas dans QEMU i386.

Les specs historiques continuent seulement par de **petits increments Foundation deja entames** (identite verifiee, capabilities, pilote de stockage hors noyau). Elles ne doivent **pas** etre lues comme "implementer US-016 TFLite" ni comme "migrer US-001 d'un coup".

Les capacites `ASSIST-xxx` **reprennent** le vocabulaire Foundation (droits, grant, revocation) et phase 2 (assistant). Le navigateur-OS (phase 3) est un **devoir du SE**, pas une vision boltee. US-031 n'est **pas** livre. Le portage vers le shell graphique est [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md) (OS-UI-0..3), pas une prolongation du sidecar Python.

## État de départ (déjà livré, à ne pas réouvrir sans régression)

Constat courant (ETAT_REEL, 27 août 2026, complété par BILAN_MASTER) :

- Suite Unity : **522/522** documentés, **523/523** au rejeu local du 13 septembre 2026
- Sept contrats `make integration-qemu` séquentiels, mesure locale **760,9 s** (12 min 41 s), budget 25 minutes
- VFS Ring 3 : `vfsserver` / `vfsvirtual`, ACL droit-source-préfixe, diagnostic public sans préfixe
- Réseau local QEMU : `make qemu-ne2k-acquire`, `qemu-ne2k-tls-http`, `qemu-ne2k-tls-sse`, `qemu-ne2k-tls-close`, `qemu-ne2k-tls-next`, `qemu-ne2k-tls-multipair` (séquentiel, `127.0.0.1`)
- GGUF Q3_K/Q4_K/Q6_K local. Sous QEMU TCG, médiane documentée ~48,7 s (premier jeton) et ~22,8 s (continuation). Ce n'est pas une mesure matérielle.
- Axe de latence GGUF sous QEMU TCG **clôturé** avec mesures (lots AOS-1641...1648). L'item ouvert README vise une **autre** plateforme (matériel / KVM), pas un second tour TCG.

Limites FAT déjà énoncées et **hors** la table de priorité AOS : écrasement, LFN enfant, renommage inter-répertoire, remplacement atomique. Elles restent des limites du contrat livré. Elles ne deviennent des cibles que si `mohhdy_us.md` les ajoute explicitement.

## Ordre des prochaines tranches (prototype guest)

Ordre imposé par `mohhdy_us.md` (priorités 0 à 3), puis items README encore ouverts ou partiels.

### Tranche 0. Tenir le budget CI QEMU

**But.** Conserver les sept contrats QEMU séquentiels sous 25 minutes, plus le smoke multi-pairs dans GitHub Actions. Ne retirer aucune assertion métier. Ne jamais rejouer une mutation ou une I/O incertaine.

**IDs liés.** AOS-012 (contrats QEMU versionnés), AOS-022 (`make integration-qemu`). Pas de nouvel ID produit.

**Critère de sortie.**

- `make integration-qemu` reste sous 25 minutes en séquentiel, avec les sept contrats complets
- `make qemu-smoke` et `make qemu-ne2k-tls-multipair` restent verts en CI (`make ci` et workflow `.github/workflows/ci.yml`)
- Job GitHub `integration-qemu` : timeout 35 minutes, budget fonctionnel 25 minutes
- Mesure locale de référence : 760,9 s. La CI de la branche doit confirmer la stabilité avant fusion
- Aucune assertion VFS, IRQ0, IA, IPC ou TLS retirée pour "passer plus vite"

**Commandes de vérification.**

```text
make test-all
make qemu-smoke
make qemu-ne2k-tls-multipair
make integration-qemu
make ci
```

**Risques et limites.**

- Flake PS/2 déjà connu (caractère dupliqué, ligne partielle). Les contrats VFS/IRQ0 réconcilient la ligne avant `ret`. Ne pas "corriger" en rejouant une commande
- Contention clavier si l'on parallélise les QEMU. Le séquentiel est la politique actuelle
- Le README marque cet item `[x]` en validation locale. `mohhdy_us.md` et ETAT_REEL le gardent en priorité 0 : c'est un **garde-fou à tenir**, pas une fonctionnalité à créer

**Ordre suggéré.** Premier. Aucune autre tranche ne doit allonger `integration-qemu` au-delà du budget sans compensation mesurée.

### Tranche 1. Couverture de l'ACL préfixée

**But.** Garder les preuves négatives (voisin, racine, voie sans chemin). Le diagnostic public reste droit-source. Le préfixe interne ne sort jamais.

**IDs liés.** AOS-026 (VFS FAT et scope backend), syscalls 124-126 (`SYS_VFS_BACKEND_*` / grant préfixe). Incréments Foundation VFS déjà livrés. Pas US-001 complet.

**Critère de sortie.**

- Tests unitaires : un droit `read` sur `initrd` n'autorise ni `overlay` ni FAT ni primitive générique ; `mutate` FAT32 reste isolé
- `make qemu-vfs-service` refuse un renommage hors préfixe **avant** mutation, source intacte, pas de cible créée
- `vfs-backend-scope <pid>` n'affiche que droits, sources et identifiant de requête. Jamais préfixe, chemin ou alias
- Primitive FAT sans chemin seulement sous scope racine

**Commandes de vérification.**

```text
make test-all
make qemu-vfs-service
make integration-qemu
```

**Risques et limites.**

- Élargir le diagnostic "pour déboguer" reviendrait à fuiter le préfixe. Interdit par le backlog
- Second niveau FAT, LFN enfant et remplacement atomique restent hors contrat actuel. Ce n'est pas cette tranche

**Ordre suggéré.** Juste après le budget CI. Toute évolution VFS / worker doit rejouer ces preuves négatives.

### Tranche 2. Topologie réseau locale partagée (optionnelle)

**But.** Plusieurs instances QEMU sur un réseau local partagé, seulement **après** une injection PS/2 démontrée avec plusieurs QEMU **simultanés**. Sans TAP, sans clé, sans Internet public, sans OpenAI.

**IDs liés.** AOS-025 (réseau minimal / profil OpenAI honnête), lots NE2000 / TLS local (`qemu-ne2k-tls-multipair`). Pas un daemon DHCP public. Harness : [aos_shared_ethernet_topology.md](aos_shared_ethernet_topology.md).

**Critère de sortie.**

- Préalable Garde 2 : `make qemu-ps2-dual` vert (deux QEMU TCG simultanés, `sendkey` mutexé hôte, écho confirmé)
- Topologie locale partagée : `make qemu-ne2k-shared-topology` — deux invités NE2000 sur un hub socket `127.0.0.1`, `nic=detected` des deux côtés, DHCP Discover de A observé pendant que B reste vivant sur le même segment
- Interdit : TAP, hôte Internet, secret, OpenAI réel
- Le contrat séquentiel `make qemu-ne2k-tls-multipair` reste le smoke TLS multi-pairs ; le partage simultané ne le remplace pas

**Commandes de vérification.**

```text
make qemu-ps2-dual
make qemu-ne2k-shared-topology
make qemu-ne2k-status
make qemu-ne2k-acquire
make qemu-ne2k-tls-http
make qemu-ne2k-tls-sse
make qemu-ne2k-tls-close
make qemu-ne2k-tls-next
make qemu-ne2k-tls-multipair
make qemu-smoke
```

`make qemu-ne2k-shared-topology` est hors `make ci` / `integration-qemu` (budget).
Suite : `make qemu-ne2k-tls-multi-guest` (TLS_COMPLETE x2) puis `make qemu-ne2k-guest-app-traffic` (ARP croise + SYN peer.local, hors ci).

**Risques et limites.**

- L'injection PS/2 reste mutexée hôte ; des sendkey chevauchés sous TCG restent un mode diagnostic seulement
- Topologie de base : un seul `ai-acquire` ; suite TLS multi : baux `10.32.0.15` / `10.32.0.16`
- Ce n'est pas le réseau public (voir tranche sous condition plus bas)

**Ordre suggéré.** Après Garde 2. TLS multi-invites livre (`qemu-ne2k-tls-multi-guest`). Guest-guest applicatif livre (`qemu-ne2k-guest-app-traffic`, hors ci).

### Tranche 3. Latence GGUF sur plateforme de référence (matériel / KVM)

**But.** Mesurer, puis éventuellement optimiser, la latence GGUF sur une plateforme stable distincte de QEMU TCG. Cible backlog : matériel ou KVM. QEMU TCG reste ~48 s / ~23 s et n'est pas l'objectif "inférieur à une seconde".

**IDs liés.** AOS-020 (GGUF et réduction de latence), lots AOS-1577...1648 (clôturé TCG), AOS-2053...2060 (benchmark QEMU). Item README encore ouvert : "Optimisation supplémentaire de la latence GGUF sur une plateforme de référence stable".

**Critère de sortie.**

- Campagne reproductible sur machine de référence ou QEMU KVM, séparée des chiffres TCG
- Rapport min / médiane / max / dispersion, isolant le temps de commande du boot (même contrat que le benchmark JSON existant)
- Ne pas déclarer "moins d'une seconde" sans mesure native ou KVM
- `make qemu-gguf-smoke` et `make test-all` restent verts. Pas de régression du chemin Q3_K réel
- Harness : `make gguf-kvm-benchmark` / `make gguf-kvm-benchmark-check` (skip CI sans KVM)

**Commandes de vérification.**

```text
make test-all
make gguf-disk
make qemu-gguf-smoke
make gguf-benchmark
make gguf-benchmark-check
make gguf-kvm-benchmark-check
make gguf-kvm-benchmark
```

`make gguf-kvm-benchmark` skippe (exit 0) sans `/dev/kvm` utilisable ou sans disque GGUF ; sur un hôte de référence, passer `GGUF_KVM_REQUIRE=1`. Détail : [aos_gguf_kvm_latency_harness.md](aos_gguf_kvm_latency_harness.md).

**Risques et limites.**

- AOS-1641...1648 : essais cache paresseux / spécialisation Q3_K top-k n'ont pas dépassé la variabilité TCG. Ne pas les réintroduire comme "prochaine optimisation QEMU"
- Les poids GPT-2 / GGUF ne sont pas dans Git. Leur absence n'est pas une régression
- Cette tranche n'est **pas** US-016 (moteur TFLite), **pas** NLU, **pas** apprentissage fédéré

**Ordre suggéré.** Après les gardes CI / ACL. Peut avancer en parallèle de la tranche 2 si le matériel / KVM est disponible, sans toucher au budget QEMU séquentiel.

### Tranche 4. Séparation du pilote de stockage (incrément Foundation, pas US-001 complet)

**But.** Pousser d'un cran la migration microkernel **déjà commencée** : `vfsserver` délègue à `vfsvirtual` les vues, mutations fixes, I/O d'alias **et** I/O des montages protégés sous capacité temporaire (AOS-2163...2170). README : **la séparation complète du pilote ATA/FAT hors noyau reste ouverte**.

**IDs liés.** US-001 (architecture microkernel) uniquement comme **incrément**. AOS-007 / AOS-023 / AOS-026 (overlay AIOV, FAT). Incréments Foundation IPC/VFS déjà livrés (01 à 64 et suivants). Pas US-010 "tous les pilotes modulaires". Pas US-016.

**Critère de sortie (incrément, pas refonte).**

- Le chemin ATA / FAT n'est plus le backend noyau unique et opaque du médiateur, **ou** un premier pas mesurable dans cette direction est livré avec contrat
- Capacités droit-source-préfixe conservées. Pas de rejeu. Diagnostic public toujours sans préfixe
- Le noyau peut rester monolithique pour le reste (NIC, IRQ, PMM). Interdit d'annoncer "microkernel terminé"
- `make qemu-vfs-service` et `make qemu-ipc-foundation` restent verts

**Commandes de vérification.**

```text
make test-all
make qemu-ipc-foundation
make qemu-vfs-service
make qemu-service-grant
make integration-qemu
```

**Risques et limites.**

- US/README : migrer US-001 d'un coup est une refonte à haut risque
- Externaliser le backend VFS puis déplacer les pilotes **derrière** des droits déjà vérifiés. Ne pas inverser
- Overlay magique `AIOV` et tickets `AOS-*` restent inchangés

**Ordre suggéré.** Après les gardes ACL (tranche 1), car toute sortie du stockage hors noyau casse l'ACL si elle est incomplète. Peut suivre ou chevaucher légèrement la tranche 3, jamais au détriment du budget CI.

### Tranche sous condition. Réseau public optionnel (pas une cible de build courante)

ETAT_REEL liste un ordre 3 "Réseau public optionnel". `mohhdy_us.md` place à la place la latence locale. Ce plan suit **mohhdy_us.md** pour l'ordre d'implémentation. Le réseau public reste **sous condition** :

- Accord explicite
- Secret fourni hors image, logs et dépôt
- Validation certificat / endpoint **séparée de la CI**
- Pas TAP dans la CI, pas OpenAI dans GitHub Actions

**Commandes** (seulement si le préalable humain est écrit) : les mêmes cibles locales `qemu-ne2k-*` plus une campagne hors CI. `make ci` ne doit pas appeler un hôte public.

## Migration des capacites ASSIST dans l'OS+UI (meme produit)

Detail, catalogue et DoD : [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md). Epiques : [../US/mohhdy_os_ui_migration.md](../US/mohhdy_os_ui_migration.md). Spec fonctionnelle conservee : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

Les tickets `ASSIST-xxx` sont des **devoirs du SE**. La surface stub (chat, simulateur DOM, MCP, FS) est un **fait guest** dans `osui_runtime.c` (ETAT_REEL). On ne reintroduit pas un sidecar Python.

Docker **doit** booter l'instance comme une machine vierge. `docker run -it mohhdy-os` lance QEMU Multiboot. Facade Python retiree (OS-UI-3).

### Gates (ne pas inverser)

| Gate | Règle |
|---|---|
| Tranches guest 0-4 | Gardes du noyau mesure. Le portage OS-UI **ne doit pas** allonger `make integration-qemu` ni relacher l'ACL prefixee |
| OS-UI-0 | Shell Ring 3 + slash + scene VGA + commande `gui`. Livre. Pas US-031 |
| OS-UI-1 | Sessions / chat / admin / droits / escalade / takeover / origine. Livre. Stub honnete |
| OS-UI-2 | Actes simulateur (gestes, MCP, facture, FS). Livre. US-031 **non livre** |
| OS-UI-3 | Facade Python retiree. Livre |
| OpenAI public | **Toujours sous condition** : accord, secret hors image, hors CI |
| ASSIST-053 | Scaffold non-billing seulement |
| ASSIST-090 | Futur. Pas une livraison proche |

### Ordre de portage (resume ; le maitre fait foi)

1. **OS-UI-000** : plan maitre (docs).
2. **OS-UI-0** : boot Docker = QEMU. Guides historiques : [assist050_docker_runtime.md](assist050_docker_runtime.md).
3. **OS-UI-1** : ASSIST-010..013, 030, 031, 040, 041 dans le guest C.
4. **OS-UI-2** : ASSIST-020..022, 060, 061 dans le guest C. `phase3_complete=false`, `us031_complete=false`.
5. **OS-UI-3** : facade Python retiree.
6. **ASSIST-090** / billing reel / TFLite / phases 4-8 : hors proche.

Le simulateur DOM, le stub et le FS sandbox vivent dans `osui_runtime.c`. Facade Python retiree.

### Relation aux phases 1-8 (sans réécrire l'histoire)

- Phase 1 Foundation : droits / capacites. L'instance reprend grant, revocation, moindre privilege, `request_id`. Elle n'affirme pas US-001 termine.
- Phase 2 AI Core / US-021 / US-028 : assistant qui parle et agit. Le guest n'a que `ai <texte>` (AOS-010). L'OS+UI porte cet assistant. Pas TFLite (US-016).
- Phase 3 Web Runtime : **navigateur-OS du SE**, devoir produit. ASSIST-060/061 = bootstrap userspace, pas US-031 livre. OS-UI-0 pose le chrome ; OS-UI-2 les actes.
- US-015 / phase 6 / phase 8 : deploiement. Docker / cloud = boot de l'instance, pas l'orchestrateur microkernel.
- US-034 : connecteurs. Les outils MCP du site en sont un voisinage borne (allowlist), pas un ERP generique.

## Specs historiques (hors build courant du guest i386)

Fichiers de spec : [../US/README.md](../US/README.md), [../US/mohhdy_us_phase1_foundation.md](../US/mohhdy_us_phase1_foundation.md), [../US/mohhdy_us_phase2_ai_core.md](../US/mohhdy_us_phase2_ai_core.md), [../US/individual_us/INDEX.md](../US/individual_us/INDEX.md).

Un `[OK]` dans l'index vision signifie "fichier de spec présent", **pas** "implémenté".

### Ce qui peut continuer, par petits incréments déjà entamés

US/README, suite Foundation (pas un sprint vision) :

1. Identité vérifiée et capabilities (au-delà du PID volatile actuel)
2. Événements accusés ou persistants (les notifications actuelles sont best-effort)
3. Montages persistants associés à des services
4. Externaliser le backend VFS lui-même (recouvre la tranche 4 prototype)
5. Puis seulement déplacer pilotes ou réseau derrière ces droits

Ces pas restent des **incréments** du prototype i386. Ils préparent US-001 / US-003 / US-012 / US-013. Ils ne livrent pas la phase 1 complète (plugins, logging distribué, virtualisation).

### Ce qu'il ne faut pas traiter comme prochaine implémentation

- **US-016** "Moteur IA local" vision = TensorFlow Lite, NLU, fédéré. L'IA réelle du prototype est GPT-2 freestanding / GGUF. Ne pas ouvrir TFLite
- Phase 2 complète, phase 3 navigateur-OS entier, phases 4 à 8 (PromptMessage, P2P, économie, multi-plateforme) comme sprint unique
- Client OpenAI public, DHCP sur réseau public, TLS vers un hôte réel, tant que la tranche sous condition n'est pas autorisée
- Annoncer que stockage, pilotes ou NIC sont "déjà hors du noyau"
- Annoncer qu'un LLM de production ou un Chromium de session tournent deja. Le stub, la KB, l'escalade, le handoff et le **simulateur DOM** du guest C ne sont pas cette livraison.

ASSIST-060/061 **n'est pas** "US-031 livre". C'est le bootstrap du navigateur-OS. Le portage est OS-UI-2. OS-UI-0 ne declare pas US-031.

## Ordre récapitulatif

Les rangs 0-4 sont **ce fichier**. Les rangs OS-UI sont le [plan maitre](PLAN_SE_MOHHDY_COMPLET.md).

| Rang | Tranche | Niveau | Nature |
|---:|---|---|---|
| 0 | Budget CI QEMU | Prototype guest | Garder, ne pas relacher |
| 1 | ACL prefixee | Prototype guest | Garder les preuves negatives |
| 2 | Topologie locale partagee | Prototype guest | `qemu-ne2k-shared-topology` + `qemu-ne2k-tls-multi-guest` + `qemu-ne2k-guest-app-traffic` (hub 127.0.0.1, hors ci) |
| 3 | Latence GGUF materiel / KVM | Prototype guest | Item ouvert README + priorite 3 `mohhdy_us.md` |
| 4 | Pilote de stockage hors noyau | Prototype guest, increment US-001 | I/O montages protégés via worker (AOS-2163) ; pilote ATA encore Ring 0 `[~]` |
| - | Reseau public | Prototype guest | Sous condition, hors CI |
| - | Identite / capabilities | Increment Foundation | Petits pas, pas US-016, pas US-001 total |
| OS-UI-000 | Spec migration | Docs | Plan maitre (fait dans cette vague) |
| OS-UI-0 | Shell Ring 3 + scene VGA sous Docker/QEMU | Instance OS | Livre (`osui_runtime.c`) |
| OS-UI-1 | Chat / admin / droits natifs | Instance OS | Livre (sessions C) |
| OS-UI-2 | Actes simulateur navigateur-OS | Instance OS | Livre ; **pas** US-031 |
| OS-UI-3 | Retrait facade Python | Instance OS | Livre |
| hote | tests/scripts + extracteur | Harness | Python autorise hors produit |
| futur | ASSIST-090, billing, TFLite, phases 4-8 | Hors proche | Voir catalogue du plan maitre |

Une PR = une tranche visible par `make ci` (code guest) ou un smoke OS-UI / alignement de documentation. Nouveaux tickets guest : prefixe `AOS-` dans `US/mohhdy_us.md`. Nouveaux tickets capacites : `ASSIST-` (spec) ou `OS-UI-` (portage). Ne pas renumeroter les specs `US-xxx`.

## Hors périmètre de ce plan

- Changement de logique C / ABI / make, sauf si une garde 0-4 l'exige
- Renommage de tickets `AOS-*`, de cibles `make`, de chemins `docs/aos*.md`
- Rebrand restant, magique `AIOV`, commandes `ai` / `ai-continue` / `ai-next`
- Faire passer le support web ou le shell graphique pour une fonction **deja mesuree** du guest i386
- Le catalogue produit complet (voir le plan maitre, pas ce fichier)
