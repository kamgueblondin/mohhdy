# Plan de suite d'implémentation

**Date :** 15 septembre 2026
**Statut :** plan de travail, pas une livraison
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document ordonne les **prochaines** tranches d'implémentation à partir du backlog déjà défini. Il n'invente pas de fonctionnalité produit. En cas de contradiction, [ETAT_REEL.md](ETAT_REEL.md) et [../US/mohhdy_us.md](../US/mohhdy_us.md) priment.

## Sources lues (sans les réécrire)

- [../US/mohhdy_us.md](../US/mohhdy_us.md), section "Prochaines tranches, hors livraison actuelle"
- [../README.md](../README.md), section "Roadmap du prototype" (items ouverts ou partiels)
- [../US/README.md](../US/README.md), couches prototype vs vision, suite Foundation
- [ETAT_REEL.md](ETAT_REEL.md), état vérifié et prochaines tranches priorisées
- [todo.md](todo.md), journal de livraison (compteurs historiques)
- [BILAN_MASTER.md](BILAN_MASTER.md), snapshot `9078d5f` / 13 septembre 2026

## Deux couches distinctes

Ne pas les mélanger. Un item de la vision n'est pas un ticket de build du prototype.

| Couche | Périmètre | Rôle dans ce plan |
|---|---|---|
| **Prototype AOS** | Hobby OS i386 Multiboot, QEMU, shell Ring 3, FAT, NE2000 local, GPT-2 / GGUF local | Seule couche à implémenter ensuite |
| **Vision MOHHDY** | Specs `US/mohhdy_*.md` et `US/individual_us/` (US-001, US-016 TFLite, P2P, etc.) | Specs uniquement. Pas des cibles de build courantes |

Le prototype vérifie AOS-001 à AOS-026, plus les lots réseau / VFS / GGUF documentés dans ETAT_REEL. Ce n'est pas TensorFlow Lite, pas un microkernel abouti, pas un client OpenAI public, pas Internet.

La vision continue seulement par de **petits incréments Foundation déjà entamés** (identité vérifiée, capabilities, pilote de stockage hors noyau). Elle ne doit **pas** être lue comme "implémenter US-016 TFLite" ni comme "migrer US-001 d'un coup".

## État de départ (déjà livré, à ne pas réouvrir sans régression)

Constat courant (ETAT_REEL, 27 août 2026, complété par BILAN_MASTER) :

- Suite Unity : **522/522** documentés, **523/523** au rejeu local du 13 septembre 2026
- Sept contrats `make integration-qemu` séquentiels, mesure locale **760,9 s** (12 min 41 s), budget 25 minutes
- VFS Ring 3 : `vfsserver` / `vfsvirtual`, ACL droit-source-préfixe, diagnostic public sans préfixe
- Réseau local QEMU : `make qemu-ne2k-acquire`, `qemu-ne2k-tls-http`, `qemu-ne2k-tls-sse`, `qemu-ne2k-tls-close`, `qemu-ne2k-tls-next`, `qemu-ne2k-tls-multipair` (séquentiel, `127.0.0.1`)
- GGUF Q3_K/Q4_K/Q6_K local. Sous QEMU TCG, médiane documentée ~48,7 s (premier jeton) et ~22,8 s (continuation). Ce n'est pas une mesure matérielle.
- Axe de latence GGUF sous QEMU TCG **clôturé** avec mesures (lots AOS-1641...1648). L'item ouvert README vise une **autre** plateforme (matériel / KVM), pas un second tour TCG.

Limites FAT déjà énoncées et **hors** la table de priorité AOS : écrasement, LFN enfant, renommage inter-répertoire, remplacement atomique. Elles restent des limites du contrat livré. Elles ne deviennent des cibles que si `mohhdy_us.md` les ajoute explicitement.

## Ordre des prochaines tranches (couche prototype AOS)

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

**IDs liés.** AOS-025 (réseau minimal / profil OpenAI honnête), lots NE2000 / TLS local (`qemu-ne2k-tls-multipair`). Pas un daemon DHCP public.

**Critère de sortie.**

- Préalable bloquant : deux (ou plus) QEMU TCG simultanés reçoivent une injection PS/2 fiable, sans contention de scancodes
- Ensuite seulement : topologie locale partagée optionnelle, toujours `127.0.0.1` / pair Ethernet contrôlé
- Interdit : TAP, hôte Internet, secret, OpenAI réel
- Le contrat séquentiel actuel `make qemu-ne2k-tls-multipair` reste vert tant que le simultané n'est pas prouvé

**Commandes de vérification.**

```text
make qemu-ne2k-status
make qemu-ne2k-acquire
make qemu-ne2k-tls-http
make qemu-ne2k-tls-sse
make qemu-ne2k-tls-close
make qemu-ne2k-tls-next
make qemu-ne2k-tls-multipair
make qemu-smoke
```

**Risques et limites.**

- ETAT_REEL : le caractère séquentiel du multi-pairs évite la contention PS/2 connue de deux QEMU TCG simultanés
- Sans solution d'injection, paralléliser est une régression, pas une fonctionnalité
- Ce n'est pas le réseau public (voir tranche sous condition plus bas)

**Ordre suggéré.** Après CI et ACL. Ne pas commencer le partage réseau tant que le préalable PS/2 simultané n'est pas démontré.

### Tranche 3. Latence GGUF sur plateforme de référence (matériel / KVM)

**But.** Mesurer, puis éventuellement optimiser, la latence GGUF sur une plateforme stable distincte de QEMU TCG. Cible backlog : matériel ou KVM. QEMU TCG reste ~48 s / ~23 s et n'est pas l'objectif "inférieur à une seconde".

**IDs liés.** AOS-020 (GGUF et réduction de latence), lots AOS-1577...1648 (clôturé TCG), AOS-2053...2060 (benchmark QEMU). Item README encore ouvert : "Optimisation supplémentaire de la latence GGUF sur une plateforme de référence stable".

**Critère de sortie.**

- Campagne reproductible sur machine de référence ou QEMU KVM, séparée des chiffres TCG
- Rapport min / médiane / max / dispersion, isolant le temps de commande du boot (même contrat que le benchmark JSON existant)
- Ne pas déclarer "moins d'une seconde" sans mesure native ou KVM
- `make qemu-gguf-smoke` et `make test-all` restent verts. Pas de régression du chemin Q3_K réel

**Commandes de vérification.**

```text
make test-all
make gguf-disk
make qemu-gguf-smoke
make gguf-benchmark
make gguf-benchmark-check
```

**Risques et limites.**

- AOS-1641...1648 : essais cache paresseux / spécialisation Q3_K top-k n'ont pas dépassé la variabilité TCG. Ne pas les réintroduire comme "prochaine optimisation QEMU"
- Les poids GPT-2 / GGUF ne sont pas dans Git. Leur absence n'est pas une régression
- Cette tranche n'est **pas** US-016 (moteur TFLite), **pas** NLU, **pas** apprentissage fédéré

**Ordre suggéré.** Après les gardes CI / ACL. Peut avancer en parallèle de la tranche 2 si le matériel / KVM est disponible, sans toucher au budget QEMU séquentiel.

### Tranche 4. Séparation du pilote de stockage (incrément Foundation, pas US-001 complet)

**But.** Pousser d'un cran la migration microkernel **déjà commencée** : aujourd'hui `vfsserver` délègue à `vfsvirtual` les vues, mutations fixes et I/O d'alias sous capacité temporaire. README : **la séparation complète du pilote de stockage reste ouverte**.

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

## Couche vision MOHHDY (specs, hors build courant)

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
- Phase 2 complète, phase 3 navigateur-OS, phases 4 à 8 (PromptMessage, P2P, économie, multi-plateforme)
- Client OpenAI public, DHCP sur réseau public, TLS vers un hôte réel, tant que la tranche sous condition n'est pas autorisée
- Annoncer que stockage, pilotes ou NIC sont "déjà hors du noyau"

## Ordre récapitulatif

| Rang | Tranche | Couche | Nature |
|---:|---|---|---|
| 0 | Budget CI QEMU | Prototype AOS | Garder, ne pas relâcher |
| 1 | ACL préfixée | Prototype AOS | Garder les preuves négatives |
| 2 | Topologie locale partagée | Prototype AOS | Optionnel, bloqué par PS/2 simultané |
| 3 | Latence GGUF matériel / KVM | Prototype AOS | Item ouvert README + priorité 3 `mohhdy_us.md` |
| 4 | Pilote de stockage hors noyau | Prototype AOS, incrément US-001 | Item partiel README `[~]` |
| - | Réseau public | Prototype AOS | Sous condition, hors CI |
| - | Identité / capabilities | Incrément Foundation | Petits pas, pas US-016, pas US-001 total |

Une PR = une tranche visible par `make ci` (code) ou un alignement de documentation. Nouveaux tickets prototype : préfixe `AOS-` dans `US/mohhdy_us.md`, sans renuméroter la vision.

## Hors périmètre de ce plan

- Changement de logique C / ABI / make, sauf si une tranche ultérieure l'exige
- Renommage de tickets `AOS-*`, de cibles `make`, de chemins `docs/aos*.md`
- Rebrand restant, magique `AIOV`, commandes `ai` / `ai-continue` / `ai-next`
