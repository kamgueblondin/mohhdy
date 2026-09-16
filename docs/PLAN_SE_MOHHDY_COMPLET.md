# Plan maitre du SE Mohhdy

**Date :** 15 septembre 2026
**Statut :** plan produit. OS-UI-000 a OS-UI-3 livres dans le guest C. Facade Python retiree. Docker = boot Multiboot. Pas US-031, pas LLM de production
**Ponctuation :** ASCII usuel et accents francais uniquement
**Public :** chef de produit, mainteneur, contributeur. Une page pour **toutes** les capacites visees

Mohhdy est **un seul produit** : le systeme d'exploitation agentique
autonome **Multiboot**. Toutes les capacites d'agent et l'interface
vivent **dans le SE**, pas dans un sidecar Python. Le produit
final qui porte chat, scene IA, slash, shell Ring 3 et simulateur
navigateur-OS est **ce** SE Multiboot (`userspace/osui_runtime.c`).
Docker, un PC, un hyperviseur ou une machine vierge **bootent le SE**
comme un metal nu (QEMU dans le conteneur). Le SE agit dans **son**
navigateur-OS (simulateur aujourd'hui ; Chromium = US-031 non livre).

Ce fichier est la **feuille de route produit**. Les gardes noyau guest (tranches 0-4) restent detaillees dans [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md). Les faits guest mesurables restent dans [ETAT_REEL.md](ETAT_REEL.md). La facade Python `agent/` / `osui/` est retiree.

## Sommaire

1. Vision unitaire et non-objectifs
2. Etat actuel honnete (guest C, Python retire)
3. Catalogue des fonctionnalites visees
4. Principes et bonnes pratiques (checklist obligatoire par PR)
5. Architecture cible
6. Roadmap ordonnee par tranches
7. Mapping ASSIST-xxx / AOS-xxx / US-xxx vers OS-UI
8. Criteres de sortie globaux et definition of done
9. Risques et interdits
10. Sources

## 1. Vision unitaire et non-objectifs

### 1.1 Un produit

| Dire | Ne pas dire |
|---|---|
| Mohhdy, SE agentique autonome | Trois produits (hobby AOS, vision, Agent Support) |
| Capacites d'agent = devoirs du SE, avec GUI native | Widget SaaS a cote du noyau |
| Docker / PC / hyperviseur = boot de l'instance, machine vierge | Docker = serveur Python de support |
| `agent/` retire (OS-UI-3) ; surface = guest C | `agent/` = produit durable |
| Navigateur-OS = coeur du SE (phase 3) | Navigateur-OS = option lointaine ou sidecar Playwright |
| Guest i386 = laboratoire noyau mesure | Guest i386 = le SE autonome deja complet |

L'utilisateur rencontre **une** instance : elle parle, elle agit dans le perimetre accorde, elle cede la main a un humain, elle expose un shell graphique et un navigateur-OS. Le support web (embed, sessions, admin) n'est pas une appli tierce : c'est l'OS qui s'ouvre sur le web.

### 1.2 Non-objectifs (explicites)

- Ne pas declarer livres : LLM de production, US-031 (navigateur-OS Chromium), microkernel US-001 complet, TFLite (US-016), SaaS de paiement, corps physique
- Ne pas tout porter demain dans QEMU i386 (widget, Chromium, Docker-dans-le-guest)
- Ne pas falsifier [ETAT_REEL.md](ETAT_REEL.md) : les faits guest restent des faits guest
- Ne pas allonger `make integration-qemu` sans compensation mesuree
- Ne pas ouvrir TensorFlow Lite, NLU 90 %, apprentissage federe, P2P, economie de points, PromptMessage comme sprint proche
- Ne pas placer un secret dans l'image, l'embed, l'UI ou la CI
- Ne pas appeler OpenAI depuis GitHub Actions
- Ne pas reintroduire un sidecar Python comme recit produit
- Ne pas renumeroter `AOS-xxx`, `ASSIST-xxx` ou `US-xxx`

### 1.3 Personae (meme instance)

| Persona | Besoin dans le SE | Hors proche |
|---|---|---|
| Visiteur d'un site | Chat, explication, acte allowliste, escalade | Administrer l'instance |
| Operateur de site | Embed, droits, outils declares | Recompiler le guest i386 |
| Humain de support | Console native, meme session, handoff | Contourner la politique |
| Operateur d'instance | Boot Docker / PC / hyperviseur / metal nu | Dependance cloud vendeur |
| Abonne heberge (option) | Meme contrat, sans operer | Facturation SaaS (futur) |
| Mainteneur noyau | Gardes AOS 0-4, increments Foundation | "Microkernel termine" |

## 2. Etat actuel honnete

Deux **niveaux de maturite** du **meme** produit. Pas deux produits.

### 2.1 Prototype guest i386 (mesure)

Source de verite : [ETAT_REEL.md](ETAT_REEL.md), backlog [../US/mohhdy_us.md](../US/mohhdy_us.md).

Observable aujourd'hui :

- Boot Multiboot, VGA/serie, shell ELF Ring 3, syscalls 0-126
- VFS Ring 3 (`vfsserver` / `vfsvirtual`), FAT16/FAT32, ACL droit-source-prefixe, diagnostic public sans prefixe
- IPC, registre de services, grant/revoke backend, `request_id`, evenements best-effort
- NE2000 local, TLS/HTTP/SSE sur pair `127.0.0.1`, pas Internet public, pas OpenAI
- GPT-2 124M et GGUF Q3_K/Q4_K/Q6_K locaux ; sous QEMU TCG ~48,7 s / ~22,8 s
- `make test-all` 522/522 documentes, 523/523 au rejeu du 13 septembre 2026
- `make integration-qemu` : sept contrats sequentiels, 760,9 s local, budget 25 min

Le guest **n'heberge pas** : widget, admin web, sessions visiteur, simulateur DOM, MCP demo, `/browser`, FS sandbox, image Docker du SE graphique, scene HTML `#ai-stage`.

### 2.2 Surface OS-UI Ring 3 (guest C, facade Python retiree)

Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md). Guides [osui_0_1_2.md](osui_0_1_2.md).

`docker run -it mohhdy-os` boot **QEMU Multiboot** (serial). Le userspace
est `userspace/osui_runtime.c` + `userspace/shell.c`. Ce n'est **pas**
un Chromium de session ni un LLM de production.

Present dans le guest C (stub, pas production) :

- Chat / prompt echo, slash, pieges Linux, `request_id`
- Sessions `s0001+` isolees, grant/revoke, escalade, takeover
- Origine etrangere refusee (`origin_denied`, 403)
- Gestes simulateur DOM, MCP declare, facture mock
- FS sandbox lecture ; traversal et write refuses
- Scene VGA 8x48, `llm=stub_echo`, `phase3_complete=false`, `us031_complete=false`
- Pont : `shared/multiboot_shell_commands.json` + `userspace/mohhdy_osui_bridge.h`
- `python_facade=false`

Absent (ne pas marquer livre) :

- Navigateur-OS (US-031 phase 3) ; `us031_complete=false`, `chromium_session_engine=false`
- LLM de production (le chemin `ai` GPT-2 local n'est pas ce stub)
- Chromium comme harness de session
- Auth par comptes / par site
- Facturation SaaS
- Widget HTTP `embed.js` / chrome HTML `#ai-stage`

### 2.3 Specs historiques

[../US/README.md](../US/README.md), [../US/individual_us/INDEX.md](../US/individual_us/INDEX.md), [../US/mohhdy_user_stories_master.md](../US/mohhdy_user_stories_master.md).

Un `[OK]` dans l'index = fichier de spec present, **pas** implemente. Les IDs `US-023` / `US-024` / `US-025` existent en double. Le **US-031 du maitre / phase 3** = navigateur-OS. Le **fichier** `individual_us/US-031_Centre_Distribution_Applications.md` est un autre sujet. ASSIST-031 = escalade humaine. Ne pas les confondre.

## 3. Catalogue des fonctionnalites visees

Legende de statut :

| Statut | Signification |
|---|---|
| **verifie** | Observable dans le guest et [ETAT_REEL.md](ETAT_REEL.md) |
| **guest C OS-UI** | Observable dans `osui_runtime.c` et ETAT_REEL ; stub, pas production |
| **a porter dans OS+UI** | Devoir produit, pas livre dans le SE graphique |
| **futur** | Hors proche (ne pas ouvrir comme sprint) |
| **spec** | Archive de vision ; pas un ticket de build |

Une ligne peut cumuler bootstrap et "a porter" : le comportement existe hors OS, il doit entrer dans l'OS+UI.

### 3.1 Socle guest / noyau (AOS, gardes 0-4)

| ID / sujet | Capacite visee | Statut | Tranche |
|---|---|---|---|
| AOS-001 | Boot Multiboot i386, VGA/serie, ISO GRUB | verifie | garde (ne pas reouvrir) |
| AOS-002 | Memoire PMM/VMM/heap/paging | verifie | garde |
| AOS-003 | PIT, PS/2, EOI IRQ0 | verifie | garde |
| AOS-004 | Initrd TAR lecture seule | verifie | garde |
| AOS-005 | Shell ELF Ring 3 | verifie | garde |
| AOS-006 | ABI syscalls 0-126 | verifie | garde |
| AOS-007 / AOS-023 | Overlay AIOV V2 ATA PIO | verifie | garde |
| AOS-008 / AOS-024 | Taches, preemption IRQ0 | verifie | garde |
| AOS-009 | `exec`, attente parent | verifie | garde |
| AOS-010 / AOS-011 / AOS-021 | GPT-2 local, BPE, Unicode cible | verifie | IA guest |
| AOS-012 / AOS-022 | Tests Unity + `make integration-qemu` | verifie, a tenir | **garde 0** |
| AOS-020 | GGUF local Q3_K/Q4_K/Q6_K | verifie TCG ; latence KVM ouverte | **garde 3** |
| AOS-025 | NE2000 local, TLS/HTTP/SSE `127.0.0.1`, stub OpenAI honnete | verifie local | reseau |
| AOS-026 | FAT16/FAT32 VFS, ACL prefixe | verifie | **garde 1** |
| IPC Foundation | FIFO Ring 3, `request_id`, capacite service | verifie (pas capabilities completes) | Foundation |
| Capabilities VFS | grant/revoke/read/mutate/full, scope source, prefixe interne | verifie | **garde 1** |
| Topologie locale partagee | Plusieurs QEMU simultanes, reseau local partage | ouvert, bloque PS/2 | **garde 2** |
| Pilote stockage hors noyau | ATA/FAT plus seulement backend noyau opaque | partiel (`vfsvirtual`) | **garde 4** |
| Reseau public optionnel | TLS vers hote reel, OpenAI sous condition | sous condition, hors CI | conditionnel |
| CI budget 25 min | Sept contrats sequentiels, smoke multi-pairs | verifie a tenir | **garde 0** |

Limites FAT **hors** cible tant que `mohhdy_us.md` ne les ajoute pas : ecrasement, LFN enfant, renommage inter-repertoire, remplacement atomique.

### 3.2 Direction Foundation / microkernel (famille US-001)

Increments **petits**, deja entames. Pas US-001 "d'un coup". Pas US-010 "tous les pilotes". Pas US-016.

| Sujet | Visee | Statut | Ou |
|---|---|---|---|
| IPC message passing | Services Ring 3 | verifie, borne | guest |
| Identite verifiee | Au-dela du PID volatile | a porter (increments guest) | suite Foundation |
| Capabilities completes | Tokens, pas seulement masque PID | partiel VFS ; reste ouvert | suite Foundation |
| Evenements accuses / persistants | Au-dela du best-effort | ouvert | suite Foundation |
| Montages persistants lies aux services | Table volatile aujourd'hui | ouvert | suite Foundation |
| Externaliser backend VFS | Recouvre garde 4 | partiel | garde 4 |
| Pilotes / NIC derriere droits | Apres VFS et identite | ouvert, plus tard | apres garde 4 |
| Plugins, logging distribue, virtualisation (US-004, US-005, US-011) | Phase 1 complete | spec / futur | hors proche |
| US-001 microkernel termine | Services memoire/FS/reseau isoles | **non livre** | jamais un sprint unique |

Ordre impose : identite et capabilities, puis evenements, puis montages persistants, puis backend VFS, **puis seulement** pilotes ou reseau derriere ces droits.

### 3.3 Coeur IA du SE (phase 2 / US-021 / US-028)

| Sujet | Visee | Statut | Tranche OS-UI |
|---|---|---|---|
| Assistant guest `ai <texte>` | Completion locale bornee | verifie (AOS-010) | reste guest |
| Stub echo / KB | Reponses honnetes sans LLM | guest C OS-UI | OS-UI-1 (porter tel quel) |
| Assistant qui parle et agit | Session, outils, droits, GUI | a porter dans OS+UI | OS-UI-1 puis 2 |
| LLM de production local a l'instance | Modele userspace, pas TFLite | **non livre** | apres OS-UI-1, sous gates |
| Fournisseur public (OpenAI) | Accord, secret hors image, hors CI | sous condition | jamais en CI |
| US-016 TensorFlow Lite | Moteur vision | **futur**, pas proche | interdit proche |
| US-017 NLU 90 % | Intentions | spec / futur | hors proche |
| US-018 modeles distribues | Spec | spec / futur | hors proche |
| US-019 apprentissage federe | Spec | spec / futur | hors proche |
| US-020 cloud-edge | Spec | spec / futur | hors proche |
| US-021 assistant integre proactif | Voisinage ; builtin `ai` seulement | a porter progressivement | OS-UI-1+ |
| US-028 intelligence conversationnelle | Contexte multi-tours, actes | a porter (stub d'abord) | OS-UI-1+ |
| Voix / TTS / STT | US-028 | futur | hors proche |

Regle d'honnetete IA : tant que le moteur est un echo ou une KB, l'UI affiche `llm=stub`. Ne jamais presenter le stub comme un LLM de production.

### 3.4 Navigateur-OS et shell graphique (phase 3, devoir coeur)

Le **US-031 cite ici** est celui de [../US/mohhdy_us_phase3_web_runtime.md](../US/mohhdy_us_phase3_web_runtime.md) (navigateur-OS), **pas** le fichier `individual_us/US-031_Centre_Distribution_Applications.md`.

| Sujet | Visee | Statut | Tranche |
|---|---|---|---|
| Shell graphique minimal | Chat central, slash, scene IA, fenetres, chrome OS | **premiere tranche** `osui/` | **OS-UI-0** |
| Scene IA `#ai-stage` | Bureau = reflexion / action / resultats HTML (stub) | **premiere tranche** bootstrap | **OS-UI-0** |
| Shell Multiboot `/shell` | Vocabulaire Ring 3 (`ai`, `vfs-list`, `help`) | **premiere tranche** bootstrap, pas TTY live | **OS-UI-0** |
| Surface navigateur de l'instance | L'OS agit dans son navigateur-OS | pane Browser-OS + bootstrap `/browser` (miroir) | OS-UI-2 (1re tranche) |
| FS-as-web | Explorateur web du FS de l'instance | pane Browser-OS + sandbox lecture | OS-UI-2 (1re tranche) |
| Vue operateur | Admin + ce que l'agent voit | panes Admin / Statut + bootstrap `/admin` | OS-UI-0/1 |
| Windowing / onglets comme taches | US-031 criteres | **non livre** | OS-UI-2 puis suite |
| US-031 navigateur-OS Chromium/WebKit | Moteur web moderne, isolation | **non livre** (`us031_complete=false`) | pas OS-UI-0 |
| US-032 apps web natives / PWA | Spec phase 3 | spec / apres OS-UI-2 | futur proche seulement si 2 sorti |
| US-033 FS web unifie | Spec phase 3 | spec | apres OS-UI-2 |
| Playwright optionnel | Controle operateur hors slim | bootstrap ; a migrer dans le navigateur-OS | OS-UI-2 (ne pas en faire le coeur) |

### 3.5 Anciennes capacites ASSIST, natives OS + GUI

Toute la liste. Rien n'est "ASSIST livre dans le SE graphique". Le scaffold compte comme bootstrap.

| ID | Capacite | Statut | Destination OS-UI |
|---|---|---|---|
| ASSIST-000 | Cadrage produit unitaire | docs (ce plan complete) | OS-UI-000 |
| ASSIST-010 | Embed / chat support | guest C OS-UI | OS-UI-1 |
| ASSIST-011 | Sessions visiteur isolees | bootstrap | OS-UI-1 |
| ASSIST-012 | Expliquer (KB locale) | bootstrap stub_kb | OS-UI-1 |
| ASSIST-013 | Origine binding, snippet CSP | bootstrap 403 `origin_denied` | OS-UI-1 (garder les preuves) |
| ASSIST-020 | Gestes allowlistes | bootstrap simulateur DOM | OS-UI-2 |
| ASSIST-021 | Outils MCP / site | bootstrap allowlist | OS-UI-2 |
| ASSIST-022 | Acte metier demo (facture) | bootstrap mock | OS-UI-2 |
| ASSIST-030 | Droits grant/revoke site/session | bootstrap | OS-UI-1 |
| ASSIST-031 | Escalade humaine | bootstrap | OS-UI-1 |
| ASSIST-040 | Console admin | bootstrap `/admin` | OS-UI-1 (UI native) |
| ASSIST-041 | Handoff meme conversation | bootstrap | OS-UI-1 |
| ASSIST-050 | Docker = boot instance | image HTTP Python aujourd'hui ; **cible** = boot SE vierge | OS-UI-0 (surface) puis 3 (parite) |
| ASSIST-051 | Install PC | bootstrap `install.sh` Python | OS-UI-0/3 (meme contrat que Docker) |
| ASSIST-052 | Hyperviseur | dry-run cloud-init, pas `make iso` | OS-UI-0/3 |
| ASSIST-053 | Cloud heberge | scaffold `hosted`, `billing=false` | futur proche optionnel ; **pas** paiement |
| ASSIST-060 | Vue navigateur instance | bootstrap miroir | OS-UI-2 |
| ASSIST-061 | FS navigateur | bootstrap lecture | OS-UI-2 |
| ASSIST-090 | Corps physique | **futur** | hors proche |

Playwright : profil operateur, **pas** US-031, **pas** harness de session. A absorber par le navigateur-OS (OS-UI-2), pas a etendre comme produit Playwright.

### 3.6 Autonomie de deploiement

| Cible | Visee | Statut honnete | Tranche |
|---|---|---|---|
| Docker | Boot du SE sur machine vierge (comme metal nu) | Aujourd'hui : HTTP Python. Cible : instance OS+UI | OS-UI-0 puis 3 |
| PC | Meme instance, install native | `install.sh` Python | aligne OS-UI-0/3 |
| Hyperviseur | VM de l'instance, distincte de l'ISO GRUB i386 | dry-run documente | aligne OS-UI-0/3 |
| Metal nu | ISO / disque du SE graphique | guest : `make iso` pedagogique ; OS-UI : pas encore | apres OS-UI-0 |
| Cloud scaffold | Origine hebergee, quotas placeholder | `MOHHDY_AGENT_MODE`, pas de facture | optionnel, self-host d'abord |
| Secrets | Jamais dans l'image, l'embed, Git, les logs | pratique deja (`ADMIN_TOKEN` au run) | **gate toute PR** |
| Self-host d'abord | L'offre hebergee n'est pas exclusive | tenu | gate |

US-015 (deploiement), phases 6 et 8 : voisinage. Ce n'est pas l'orchestrateur microkernel ni un PaaS.

### 3.7 Specs historiques hors proche (inventaire, pas sprint)

Pour qu'un PM voie **tout** le vise, y compris ce qu'on **ne** construit pas maintenant.

**Phase 1 reste** (fichiers `individual_us/` US-002 a US-015, hors increments deja notes) : ressources IA, securite adaptative complete, plugins, logging distribue, config dynamique, monitoring, MAJ incrementale, virtualisation, APIs unifiees completes. Statut : **spec**, petits recouvrements guest seulement.

**Phase 2 reste** (US-016 a US-030 hors voisinage assistant) : TFLite, NLU, modeles distribues, federe, cloud-edge, personnalisation, doublons 023-025, workflows, ethique. Statut : **futur / spec**. Interdit d'ouvrir TFLite.

**Phase 3 reste** (maitre US-034 a US-045) : explorateur, medias, PWA, sync, cache, securite web, responsive, themes, Web Components, perf web, accessibilite, debugger. Statut : **spec**, apres un vrai navigateur-OS.

**Phase 4 PromptMessage** (maitre US-046 a US-060) : langage, compilateur, interpreteur, IDE. Statut : **futur**.

**Phase 5 P2P** (maitre US-061 a US-075) : protocole, discovery, consensus, gRPC, NAT. Statut : **futur**. Ne pas confondre avec les fichiers `individual_us/US-061+` (perf / eco), numerotation divergente.

**Phase 6 multi-plateforme** (maitre US-076 a US-090) : ARM, mobile, capteurs, continuite. Statut : **futur**. Le deploiement Docker/PC/hyperviseur actuel n'est pas cette phase.

**Phase 7 collaborative** (maitre US-091 a US-105) : points, marketplace, paiement decentralise. Statut : **futur**. Distinct du scaffold ASSIST-053 (`billing=false`).

**Phase 8 production** (maitre US-106 a US-120) : monitoring prod, rollback auto, scale, support. Statut : **futur**. Les gates CI actuelles ne sont pas cette phase.

**Fichiers `individual_us/` US-031 a US-075** (ecosysteme, RGPD, zero-trust, perf) : **spec**. Voisinage utile : US-034 connecteurs (MCP borne), US-048 RGPD (sessions = donnee perso), US-015 deploiement. Pas des sprints OS-UI-0.

## 4. Principes et bonnes pratiques

Checklist **obligatoire** pour **chaque** PR (guest, OS-UI, docs). Une case non tenue = la PR ne fusionne pas.

### 4.1 Securite et droits

- [ ] Moindre privilege : pas de droit implique "pour aider"
- [ ] Grant et revoke observables ; un outil revoque n'execute plus l'acte suivant
- [ ] Correlation : `request_id` (et `session_id` cote instance) sur chaque acte
- [ ] Aucun secret dans embed, UI publique, image Docker, depot, logs
- [ ] Origine binding : document vs site declare ; preuve negative 403
- [ ] Pas d'acte irreversible (paiement, suppression massive) sans politique ecrite ou humain
- [ ] Diagnostic public sans borne interne (prefixe ACL, jeton admin, chemin sandbox)

### 4.2 Honnetete et docs

- [ ] Faits guest seulement dans [ETAT_REEL.md](ETAT_REEL.md) ; ne pas y coller `agent/` ni US-031
- [ ] Stub vs LLM : le contrat JSON/UI dit `stub` tant que ce n'est pas un moteur reel
- [ ] `phase3_complete` et `us031_complete` restent faux tant que le navigateur-OS n'est pas la
- [ ] Ponctuation docs : ASCII (`-`, `"`, `'`, `...`) plus accents francais ; pas de tiret long, fleches, points de suspension typographiques, guillemets courbes
- [ ] Une PR = une tranche visible, taille rollback-friendly

### 4.3 Tests et CI

- [ ] Guest : `make test-all` et, si le noyau est touche, ne pas casser les sept contrats
- [ ] `make integration-qemu` : ne pas allonger au-dela de 25 min sans compensation **mesuree** (et documentee)
- [ ] Pas d'OpenAI, pas d'hote public, pas de TAP dans GitHub Actions
- [ ] OS-UI : `make osui-smoke` + `make qemu-osui-runtime` **hors** `make integration-qemu`
- [ ] Preuves negatives conservees (voisin ACL, origine etrangere, outil non declare)

### 4.4 Deploiement

- [ ] Self-host possible a chaque etape
- [ ] Docker / PC / hyperviseur = **la meme** instance, pas trois produits
- [ ] Jeton et secrets uniquement a l'execution
- [ ] Ne pas bootstraper l'UI OS via `make iso` i386

## 5. Architecture cible

### 5.1 Instance Mohhdy (produit)

```text
Metal nu / Docker / PC / hyperviseur
        boot instance Mohhdy (machine vierge)
                |
                +-- shell graphique (fenetres, chrome, vue operateur)
                +-- navigateur-OS du SE (surface d'action, FS-as-web)
                +-- services natifs :
                |     sessions, chat, KB, droits, escalade, handoff, admin
                |     gestes allowlistes, MCP, acte demo
                +-- IA locale honnete (stub, puis LLM d'instance, pas TFLite proche)
                +-- politique : grant/revoke, origine, request_id, pas de secret UI
```

Le SE **agit dans son propre navigateur-OS**. L'embed public est une fenetre du SE vers un site tiers, pas un produit separe.

### 5.2 Docker aujourd'hui vs Docker cible

| | Aujourd'hui | Cible |
|---|---|---|
| `docker run` | QEMU Multiboot nographic (`mohhdy-os`) | Boot de l'instance OS |
| Surface | Shell Ring 3 + `osui_runtime.c` | UI native du SE (Chromium plus tard) |
| Noyau i386 | Boote dans le conteneur via QEMU | Laboratoire = produit pour cette tranche |
| Secret | aucun dans l'image | inchange |

OS-UI-3 : le noyau Multiboot **tourne** dans Docker via QEMU. Ce n'est pas Chromium-dans-QEMU.

### 5.3 Relation avec le guest i386

Le guest reste le **laboratoire noyau** : ABI, VFS, ACL, GGUF, NE2000, CI. Il n'est pas le vehicule du widget ni du navigateur-OS.

```text
                    SE Mohhdy (un produit)
                   /                      \
     guest i386 QEMU/ISO              instance Docker = meme guest
     AOS 0-4, Foundation              osui_runtime.c + shell Ring 3
     ETAT_REEL                        python_facade=false
                                      pas de sidecar HTTP
```

Interdit : fusionner les deux chemins en "on met Chromium dans QEMU TCG i386 demain".

### 5.4 Retrait de `agent/` (fait)

OS-UI-3 a retire `agent/` et le serveur Python `osui/`. La parite
comportementale (sessions, origine, grant/revoke, escalate/takeover,
gestes, MCP, FS) est dans le guest C. Contrats HTTP historiques :
guides `assist*` marques historiques. Packaging Docker pointe vers
QEMU. PC / cloud-init HTTP : non portes en C (liste explicite).

## 6. Roadmap ordonnee par tranches

Ordre de **build produit** (OS-UI) en parallele des **gardes guest 0-4**. OS-UI-000 a OS-UI-3 sont livres dans le guest C. Facade Python retiree.

### 6.1 OS-UI-000. Spec de migration (cette PR)

**But.** Une feuille de route unique, honnete, avec catalogue complet et gates.

**Livrable.** Ce fichier, [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md) reoriente, index README / docs / US, [../US/mohhdy_os_ui_migration.md](../US/mohhdy_os_ui_migration.md). Pas de changement `agent/` runtime.

**Critere de sortie.** Un PM trouve toutes les capacites visees, le prochain pas (OS-UI-0), et les interdits. ETAT_REEL inchange.

### 6.2 OS-UI-0. Shell graphique minimal dans l'instance Docker

**Statut.** Livre : `userspace/osui_runtime.c`, image `mohhdy-os` = QEMU. Guides : [osui_0_1_2.md](osui_0_1_2.md), [osui_chat_desktop.md](osui_chat_desktop.md), [osui_ai_stage.md](osui_ai_stage.md).

**But.** L'instance Docker presente un **shell graphique du SE** (chrome fenetre, vue operateur), pas seulement des pages HTML du sidecar comme identite produit.

**Inclut.**

- Surface GUI minimale de l'instance (fenetre operateur, cadre OS)
- Scene IA plein ecran (`#ai-stage`) : reflexion / action / resultats HTML stub
- `/shell` aligne sur le vocabulaire guest Ring 3 (bootstrap, pas TTY live)
- Boot Docker = entree dans cette surface (meme si le backend reste `agent/` un temps)
- Sante, pas de secret dans l'image, self-host
- Documenter clairement : ce n'est pas US-031, pas un LLM, pas le guest QEMU, pas de scene HTML dans ETAT_REEL

**N'inclut pas.** Portage complet chat/admin, Chromium de session, retrait de Python, ISO i386 dans Docker.

**Verification.** Smoke GUI/HTTP hors `make ci`. `make integration-qemu` inchange. `us031_complete=false`.

**Definition of done.** Un operateur lance Docker et voit le chrome du SE, pas "un serveur Flask de demo" comme recit produit.

### 6.3 OS-UI-1. Portage sessions / chat / admin / droits en UI native

**Statut.** Livre dans le shell Ring 3 (sessions, chat, admin, droits).

**But.** Les devoirs ASSIST-010..013, 030, 031, 040, 041, 012 vivent dans l'UI du SE.

**Inclut.** Embed ou equivalent OS, sessions isolees, KB honnete, origine binding, masque de droits, escalade, handoff, console native, stub LLM visible.

**N'inclut pas.** Gestes navigateur-OS reels, US-031, comptes multi-tenant, OpenAI CI.

**Verification.** Rejouer les preuves `agent-smoke` cote comportement (origine 403, session isolee, revoke, takeover). Gates section 4.

**Definition of done.** Parite chat/admin/droits dans le guest C ; facade Python retiree.

### 6.4 OS-UI-2. Actes navigateur-OS

**Statut.** Premiere tranche livree dans le pane Browser-OS (simulateur etiquete, FS lecture, facture mock). `phase3_complete=false`, `us031_complete=false`.

**But.** L'OS agit dans **son** navigateur-OS : gestes allowlistes, MCP, facture demo, vue instance, FS-as-web. Playwright n'est plus le recit ; il peut rester passerelle temporaire.

**Inclut.** ASSIST-020, 021, 022, 060, 061 portes dans la surface OS. Preuves : origine etrangere refusee, outil non declare refuse, pas d'acte irreversible sans politique.

**N'inclut pas.** Declarer US-031 livre. Pas de Chromium "95 % du web" comme critere de sortie de cette tranche.

**Verification.** Smoke actes + FS, drapeaux `phase3_complete=false` tant que le moteur web n'est pas un navigateur-OS reel. ETAT_REEL guest inchange.

**Definition of done.** Un acte demo (facture) et un geste allowliste passent par le navigateur-OS de l'instance, journal `request_id`, meme session.

### 6.4bis OS-UI-C. Convergence vers le SE Multiboot

**Statut.** Premiere tranche livree : registre partage, header guest, hook live `/shell`, scene IA multi-etapes, Prompt OS elargi.

**But.** Les features **land in Multiboot SE**. `osui/` reste bootstrap. Un artefact de code aligne osui et `userspace/shell.c`.

**Inclut.**

- `shared/multiboot_shell_commands.json` genere depuis `userspace/shell.c`
- `userspace/mohhdy_osui_bridge.h` (`MOHHDY_OSUI_GUEST_HTML_STAGE 0`)
- Hook live `MOHHDY_SHELL_ATTACH=live` + serie/HMP, defaut bootstrap
- Scene IA : dessins SVG, simulations, mini-plans stub (`/plan`, tick)
- Routes NL : ouvrir shell/browser/admin, dessiner, help/ai-help guest
- `make osui-shell-live-smoke` optionnel, skip si pas de QEMU

**N'inclut pas.** Scene HTML dans le VGA guest. US-031. LLM de production. Allonger `make integration-qemu`.

**Verification.** `make osui-smoke` (registre `--check`). Live : faux guest serie ; QEMU skip. Guides : [osui_convergence.md](osui_convergence.md), [osui_shell_live.md](osui_shell_live.md).

**Definition of done.** Un PM voit le pont de code, le hook live honnete, et ce qui reste a porter dans le guest.

### 6.5 OS-UI-3. Retrait de la facade Python

**Statut.** Livre. `agent/` et le serveur `osui/` Python sont supprimes. Docker boot QEMU. `make osui-smoke` prouve l'absence. `make qemu-osui-runtime` prouve la parite guest.

**But.** Un seul userspace d'instance : le guest C.

**Inclut.** Smokes de regression Unity + QEMU OS-UI, packaging Docker QEMU, docs assist* historiques.

**N'inclut pas.** Effacer les contrats comportementaux. Chromium. SaaS billing. Embed HTTP.

**Critere de go.** Checklist 8.3 tenue dans le guest C (sauf embed HTTP / Chromium, hors C freestanding).

### 6.6 Gardes guest 0-4 (parallele, jamais sacrifiees)

Detail operationnel : [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md). Resume :

| Garde | Travail | Nature |
|---|---|---|
| 0 | Budget CI QEMU 25 min, sept contrats | Tenir |
| 1 | ACL prefixee, preuves negatives | Tenir |
| 2 | Topologie locale partagee | Optionnel, bloque PS/2 simultane |
| 3 | Latence GGUF materiel / KVM | Item ouvert README |
| 4 | Pilote stockage hors noyau | Increment US-001, pas refonte |
| * | Reseau public | Sous condition, hors CI |
| * | Identite / capabilities / evenements / montages | Petits pas Foundation |

Aucune tranche OS-UI ne relache ces gardes.

### 6.7 Futurs hors proche

- ASSIST-090 corps physique (reutiliser le modele de droits, pas de materiel)
- ASSIST-053 facturation reelle / Stripe / multi-tenant payant
- US-016 TFLite, NLU, federe
- Phase 3 complete (US-031 moteur, PWA, FS web unifie)
- Phases 4 a 8 (PromptMessage, P2P, mobile, points, production)
- Microkernel US-001 "termine"

## 7. Mapping des IDs vers les tranches OS-UI

### 7.1 ASSIST vers OS-UI

| ASSIST | Tranche | Note |
|---|---|---|
| 000 | OS-UI-000 | Cadrage ; ce plan |
| 010, 011, 012, 013 | OS-UI-1 | Chat, session, KB, origine |
| 030, 031, 040, 041 | OS-UI-1 | Droits, escalade, admin, handoff |
| 020, 021, 022 | OS-UI-2 | Gestes, MCP, facture |
| 060, 061 | OS-UI-2 | Browser-OS bootstrap, FS |
| 050, 051, 052 | OS-UI-0 (boot surface) puis OS-UI-3 (parite packaging) | Docker/PC/HV = instance, pas sidecar |
| 053 | Futur optionnel | Scaffold ok ; billing non |
| 090 | Futur | Hors proche |

Les IDs `ASSIST-xxx` **restent**. On ne les renumerote pas. Les epiques `OS-UI-*` ordonnent le portage. Detail stories : [../US/mohhdy_os_ui_migration.md](../US/mohhdy_os_ui_migration.md).

### 7.2 AOS vers gardes

| AOS / lot | Garde |
|---|---|
| AOS-012, AOS-022, `make ci` | 0 |
| AOS-026, syscalls 124-126, preuves VFS | 1 |
| AOS-025, NE2000 multi-pairs, PS/2 simultane | 2 (option) |
| AOS-020, lots GGUF, item README latence | 3 |
| AOS-007/023/026, worker stockage | 4 |
| AOS-001..011, 021, 024 | Socle verifie, pas de nouvelle tranche |

### 7.3 US vision vers voisinage honnete

| US (sens retenu) | Voisinage | Tranche | Interdit |
|---|---|---|---|
| US-001 | Increments Foundation + garde 4 | petits pas guest | "microkernel termine" |
| US-003 / US-012 / US-013 | Droits, ABI, IPC | Foundation + OS-UI-1 | capabilities completes declarees |
| US-015 | Docker/PC/HV | OS-UI-0/3 | orchestrateur phase 6 |
| US-016 | aucun proche | futur | TFLite sprint |
| US-021 / US-028 | Assistant OS | OS-UI-1+ | phase 2 complete |
| US-031 **phase 3** | Navigateur-OS | OS-UI-0 chrome, OS-UI-2 actes | marquer livre |
| US-032 / US-033 | Apps web, FS web | apres OS-UI-2 | sprint OS-UI-0 |
| US-034 | MCP allowliste | OS-UI-2 | ERP generique |
| US-048 | Sessions = donnee perso | gate OS-UI-1 | journal JS public |
| US-031 **fichier individual_us** | Centre de distribution | spec | confondre avec navigateur-OS |
| Maitre US-046..120 | Phases 4-8 | futur | sprint unique |

## 8. Criteres de sortie globaux et definition of done

### 8.1 Sortie globale du plan (quand le SE est "le produit")

- Une instance Docker/PC/hyperviseur boot un shell graphique du SE
- Chat, sessions, admin, droits, escalade, handoff sont dans cette UI
- Les actes passent par le navigateur-OS de l'instance (pas un sidecar comme recit)
- `agent/` n'est plus la facade (OS-UI-3 fait)
- Stub ou LLM clairement etiquete ; US-031 seulement si le moteur web est reel
- Gardes guest 0-1 tenues ; 2-4 selon leur propre DoD
- ETAT_REEL toujours borné au guest

Ce n'est **pas** la sortie de OS-UI-0.

### 8.2 DoD par tranche

| Tranche | DoD court |
|---|---|
| OS-UI-000 | Plan maitre + liens ; pas de code `agent/` |
| OS-UI-0 | Chrome OS a chat central, scene IA et shell Multiboot (`osui/`) ; hors QEMU CI ; pas US-031 |
| OS-UI-1 | Parite chat/admin/droits/escalade/handoff/origine dans le shell ; stub honnete |
| OS-UI-2 | Premiere tranche : actes + FS dans le pane Browser-OS ; preuves negatives ; pas Chromium |
| OS-UI-C | Registre guest + hook live honnete + scene IA multi-etapes ; pas VGA HTML guest |
| OS-UI-3 | Facade Python retiree ; Docker = QEMU ; preuves guest C |
| Garde 0 | 7 contrats, < 25 min, smoke multi-pairs CI |
| Garde 1 | Preuves ACL prefixe, diagnostic sans prefixe |
| Garde 2 | PS/2 simultane d'abord, sinon pas de topologie partagee |
| Garde 3 | Campagne KVM/materiel distincte des chiffres TCG |
| Garde 4 | Increment stockage mesure, ACL intacte, pas "US-001 fini" |

### 8.3 Parite avant retrait Python (OS-UI-3)

Minimum a cocher, comportement contre comportement :

- Sante instance, embed sans secret, deux sessions isolees
- KB ou refus honnete, origine document refusee
- Grant/revoke, outil non declare 403, escalade, takeover
- Geste allowliste, MCP demo, facture meme `session_id`
- Vue navigateur instance + FS sandbox (lecture, traversal refuse)
- Packaging Docker/PC/HV, secret au run, self-host
- Aucune regression garde 0-1

## 9. Risques et interdits

| Risque | Mitigation |
|---|---|
| Tout mettre dans QEMU i386 | Interdit. Guest = laboratoire. Instance = Docker/PC/HV/metal |
| Falsifier ETAT_REEL | Faits guest seulement. Scaffold et OS-UI ailleurs |
| Etendre un sidecar Python comme produit | OS-UI-3 a retire `agent/` ; surface = guest C |
| Declarer US-031 ou un LLM livres | Drapeaux et ce plan ; stub visible |
| Allonger `integration-qemu` | Compensation mesuree ou hors CI |
| Confondre ASSIST-031, US-031 phase 3, US-031 fichier | Table section 7.3 |
| Attendre US-001 complet pour l'UI | OS-UI-0 peut demarrer en parallele des gardes |
| Playwright = navigateur-OS | Profil optionnel ; migrer dans OS-UI-2 |
| Secrets / OpenAI CI | Gate section 4 |
| SaaS billing trop tot | ASSIST-053 reste non-billing |
| Big-bang retrait Python | Fait (OS-UI-3) apres parite C |
| PR docs trop "vision" sans prochain pas | Prochain : gardes 0-4 + US-031 honnete |

## 10. Sources (lues, non reecrites comme livrees)

- [ETAT_REEL.md](ETAT_REEL.md), [BILAN_MASTER.md](BILAN_MASTER.md), [vocabulaire.md](vocabulaire.md)
- [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md) (gardes 0-4)
- [../README.md](../README.md), [README.md](README.md)
- [../US/README.md](../US/README.md), [../US/mohhdy_us.md](../US/mohhdy_us.md)
- [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md)
- [../US/mohhdy_os_ui_migration.md](../US/mohhdy_os_ui_migration.md)
- [../US/mohhdy_us_phase1_foundation.md](../US/mohhdy_us_phase1_foundation.md)
- [../US/mohhdy_us_phase2_ai_core.md](../US/mohhdy_us_phase2_ai_core.md)
- [../US/mohhdy_us_phase3_web_runtime.md](../US/mohhdy_us_phase3_web_runtime.md)
- [../US/mohhdy_us_phases_4_8_synthese.md](../US/mohhdy_us_phases_4_8_synthese.md)
- [../US/mohhdy_user_stories_master.md](../US/mohhdy_user_stories_master.md)
- [../US/individual_us/INDEX.md](../US/individual_us/INDEX.md)
- Guides `assist010` ... `assist060`, `assist_playwright_optional.md`, `assist051_052_053_deploy.md`
- [osui_0_1_2.md](osui_0_1_2.md), [osui_chat_desktop.md](osui_chat_desktop.md), [osui_ai_stage.md](osui_ai_stage.md), [osui_shell_live.md](osui_shell_live.md), [osui_convergence.md](osui_convergence.md)

En cas de contradiction sur le **guest**, ETAT_REEL et `mohhdy_us.md` priment. En cas de contradiction sur l'**ordre produit**, ce plan prime sur les recits "track Agent Support" anterieurs.
