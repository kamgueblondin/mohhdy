# Rapport d'Exécution, Validation et Feuille de Route MOHHDY OS

## 1. Contexte et Validation de l'Exécution

Le projet **MOHHDY OS** a été récupéré, compilé et validé sur l'environnement cible.

### Compilation
- `make all` génère l'image binaire du noyau 32-bit Multiboot (`build/mohhdy.bin`), l'archive initrd (`my_initrd.tar`) et le disque overlay d'extension (`build/overlay.img`).

### Suite de Tests
- `make test-all` rejoue la suite de non-regression sur l'arbre courant :
  - **902 tests executes / 902 tests reussis** (0 echec, 0 ignore).
  - 54 modules kernel, 9 modules userspace, 1 performance, 1 robustness. Couverture hote mesuree par `make coverage` : 79.55% de lignes (24434/30716). Pas l'image guest. Pas dans `make test-all`.
- Les compteurs plus anciens de ce fichier (563/563 et autres) decrivent la livraison de leur paragraphe, pas l'arbre courant.
- `make strict-image-check`, `make secrets-check` et `make tree-clean-check` sont les gardes d'image et d'arbre.

### Contrats d'Intégration QEMU OS-UI
- `python3 tests/scripts/test_qemu_osui_runtime.py` : **PASSED** (Contrat Ring 3, origin checking, IPC VFS, MCP invoice).
- `python3 tests/scripts/test_qemu_osui_gui.py` : **PASSED** (Contrat bureau VBE, injection d'événements tablette USB via UHCI QMP, switch console/gui).
- `make gguf-kvm-benchmark-check` : **PASSED** (Protocole de benchmark de latence KVM/GGUF).

---

## 2. Capture d'Écran VBE Desktop (Preuve Visuelle)

La capture d'écran du framebuffer VBE QEMU 1024x768 (`test_logs/qemu-osui-gui-desktop.png`) a été générée et convertie depuis le screendump PPM produit par `test_qemu_osui_gui.py`.

Elle confirme visuellement :
- Le bureau graphique autonome VBE piloté par le guest C (`userspace/osui_runtime.c` & `userspace/osui_gui.c`).
- L'affichage de la fenêtre centrale Mohhdy avec invite de commande et barre de statut.
- La scène IA graphique d'affichage réflexif/action/résultats.
- Les icônes du dock latéral (`Browser-OS`, `Shell`, `Admin`, `Support`, `Statut`, `Fichiers`).
- Le curseur de la tablette USB UHCI positionné dynamiquement.

---

## 3. Descriptif des Prochaines Tranches d'Implémentation (Priorités 1 à 3)

### Etape 1 : Tranche 4, ce qui reste Ring 0
- Lot 6 part de `ca04007` (merge #154), PR #155. `make test-all` : 848/848. US-001 : souris PS/2 prouvee en QEMU, Shift garde a la mort du pilote. Phase 3 : moteur web minimal en C guest (US-031), `web-perf` (US-043), `web-dom`/`web-inspect`/`web-console` (US-045), pipeline composants/`@media`/`var()`/a11y. Phase 4 : cle CA scellee par le noyau (cle de scellement par boot, simule). Phase 5 : gRPC sur TCP entre deux invites (`make qemu-grpc-guests`), lissage QoS, cache via Raft, anomalie attribuee par pair.
- Lot 7 part de `a44e95c` (merge #155), PR #156. `make test-all` : 867/867. US-001 : console texte VGA peinte par le pilote Ring 3 `bin/vgadriver` (repli Ring 0 au debut du boot, a la mort, au blocage, en panique) ; US-001 reste ouvert (tablette UHCI en Ring 0 ; PIC/PIT noyau assume). US-039 : assainisseur dans le pipeline web (partiel, `phase3_complete=false`). US-067 : plusieurs appels par connexion et controle de flux. Phases 6-8 : sorties figees remplacees (US-086, 094, 099, 101, 106, 108, 112, 115, 116, 119) ou marquees simule (US-078, 097), inventaire `docs/lot7_inventory.md`.
- Lot 5 part de `c4fb0c8` (merge #153), PR #154. `make test-all` : 830/830. US-001 : PS/2 et COM1 dans des pilotes Ring 3 supervises (repli Ring 0 honnete au boot, a la mort et au blocage d'un pilote, et en panique), `make qemu-input-drivers`. Phase 3 partiels US-038 a US-042 et US-044 (`userspace/webrt.c`, commandes `web-*`). Phase 4 : `pm-cross` (C99), `pm-assist` (regles), revocation CA locale. Phase 5 : `grpc-wire` (trames HTTP/2 et HPACK en memoire, US-067 partiel).
- Lot 4 part de `0254a81` (merge #152), PR #153. `make test-all` : 801/801. Reprise sur perte TCP/TLS du client guest (doublons, segments futurs, retransmission du ClientHello et du vol Finished avec recul), prouvee par `make qemu-net-loss-recovery` (pertes et doublon de trames voulus).
- US-001 reste ouvert : enumeration PCI en worker Ring 3 `bin/pcidriver` supervise (relance paresseuse, budget 4), lectures de config Ring 0 fermees apres la remise. Restent Ring 0 : sondes de debut de boot, PIC, PIT, PS/2, VGA texte, serie.
- Phase 5 : US-063 deux relais et US-069 crash/rejoin prouves en QEMU (`make qemu-p2p-mesh`, 4 invites), US-074 vrai codec STUN sur NAT simule. Phase 4 : US-050 coloration, completion, volet de sortie.
- Lot 3 part de `7cfcf0b` (merge #151), PR #152. `make test-all` : 783/783. `make coverage COVERAGE_MIN=78` tourne en CI (79.82% mesure).
- US-001 reste ouvert, plus etroit : journaux de montage et d'evenements lus par `bin/atadriver` Ring 3 (`jkread 0`), image legacy sans montage FAT Ring 0 (elle garde seulement le NIC Ring 0, noyau de comparaison). Restent Ring 0 : PIC, PIT, PS/2, VGA, serie, PCI, `usb_tablet`.
- Phase 5 (partiels, plus avances) : US-063 multi-relais (unitaire 4 noeuds), US-068/073 controleur adaptatif (QEMU), US-070 cache versionne a invalidation acquittee (QEMU), US-075 base EWMA (pas de ML). Phase 4 : US-050 IDE plein ecran dans le bureau OS-UI (partiel). Phase 3 : vrai moteur de regles CSS (cascade, specificite, heritage).
- Lot 2 part de `708917b` (merge #149). `make test-all` : 770/770. Le compteur du lot 1 (768/768) reste dans `docs/ETAT_REEL.md`.
- US-001 reste ouvert. IDENTIFY Ring 0 et le PIO FAT de repli ne sont lies dans aucune image. L'image legacy garde le NIC et `kernel_ring0_boot_fat_mount`. Le respawn de `bin/atadriver` (budget 4) rend le budget quand le pid publie le store, et ne rearme le canal que s'il est BSY ou DRQ. Le journal de regime permanent passe par le pilote quand l'appelant peut attendre. Restent Ring 0 : lecture du journal au boot, chargement overlay au boot, PIC, PIT, PS/2, VGA, serie, PCI, `usb_tablet`, montage FAT de boot legacy. Preuve : `make strict-image-check`, `make qemu-ata-driver` (269 s).
- US-064 est ferme pour le journal Raft du guest (pas Paxos). InstallSnapshot, lease de leader 10 s, pre-vote, fenetre 16, compaction. Le scenario QEMU 2 coupe le fil du hub (`cut_pair`, pas `p2p-block`). Le scenario 2b installe le snapshot puis commit l'index 3. Preuve : `make qemu-p2p` (183 s).
- US-068 et US-073 restent partiels (fenetre AIMD, unitaire). US-069, US-070 et US-075 restent partiels (coupure hub, invalidation de cache, regle fixe `congestion`). US-067 reste un RPC type. US-074 reste un STUN/NAT simule. US-063 reste un relais.
- Couverture hote mesuree par `make coverage` : 79.55% de lignes (24434/30716), 143 fichiers, 572 `.gcda`. Pas l'image guest. Pas dans CI.

### Etape 1b : Phase 4, ce que la pull request #147 livre vraiment
- US-050 reste partiel. `pm-ide` est l'editeur de lignes `pm-edit` (`userspace/shell.c`) : numeros (`.l`), controle syntaxique (`.c`), execution VM (`.r`), sauvegarde (`.`), abandon (`.q`). Pas un IDE plein ecran. Pas dans `userspace/osui_runtime.c`.
- US-059 reste partiel. `pm-catalog-search` filtre les fichiers locaux `/pm/*.pm`. `pm-catalog-fetch` copie un programme local qui compile (meme depot que `pm-install`). Pas de marketplace reseau.
- US-060 reste partiel. `pm-ca-init`, `pm-ca-sign`, `pm-ca-verify` : cle Schnorr locale, `/etc/pm_ca.pub`, cle privee en clair `/etc/pm_ca.key`, signature `FILE.ca`. Pas une autorite de certification publique, pas de revocation. `pm-certify` / `pm-verify` restent le registre FNV-1a et le `.sig` du noeud.
- Non livres : US-055 compilateur croise, US-058 assistance IA.
- Preuve : `make qemu-promptmessage`.

### Etape 1c : Phase 3, le rendu texte de la pull request #147
- `browser-html-render` est un imprimeur texte. Liste plate d'au plus 32 balises, pas de liens parent, pas de boites de mise en page. `osui_css_apply_styles` ignore le texte CSS et applique quelques regles fixes (h1/b, lien bleu, bouton gris, marge 8 si id `app`). `phase3_complete` reste false. Pas US-031.
- `bigint_modexp` saute les bits de poids fort a zero du membre haut. Primitive partagee, pas un changement de poignee de main TLS a lui seul. Preuve `test_bigint_modexp_leading_zeros`.
- La table fil du worker (`kernel/net_wire.c`) accepte une ecoute `0.0.0.0` port distant 0 et prefere un 4-tuple precis. La pompe `web-serve` de `kernel/net_llm_client.c` est une autre table : elle compte et jette encore les trames dont le port de destination n'est pas le sien.
- La couverture n'est pas mesuree.

### Étape 2 : Moteur de session et conteneurisation du Navigateur-OS (US-031)
- **Objectif** : Faire évoluer le simulateur DOM vers la première version du conteneur d'onglets du Navigateur-OS avec persistance des sessions par site et intégration des vues d'inspection VFS (FS-as-web).
- **Gestionnaire d'onglets et persistance** : Faire évoluer les fonctions d'onglets dans `userspace/osui_runtime.c` (`browser-tab-new`, `browser-tab-use`, `browser-tab-close`) avec une persistance du stockage local par onglet/site (`browser-storage-set`, `browser-storage-get`).
- **Vues d'inspection VFS (FS-as-web) dans l'OS-UI** : Intégrer la vue du système de fichiers sandboxé et l'inspection de statut VFS directement dans les panneaux natifs de l'OS-UI (`/browser` et `/fs`) dans `userspace/osui_gui.c`.
- **Validation** : Tests unitaires `test_osui_runtime` et `test_osui_gui` + `python3 tests/scripts/test_qemu_osui_gui.py`.

### Étape 3 : Gestionnaire de mémoire et latence GGUF sous KVM / Matériel de référence (Garde 3)
- **Objectif** : Évaluer et optimiser les gains de latence du cache KV réutilisé lors de l'inférence GGUF locale sur plateforme KVM/matériel par rapport à QEMU TCG.
- **Mesure** : un echantillon KVM, 2 octobre 2026, poids residents. Premier jeton 43,916 s, continuation 20,224 s. `sub_second_claim_allowed` faux.
- **Déploiement du harnais de mesure** : Exécuter le harnais `tests/scripts/benchmark_qemu_gguf_kvm_latency.py` sous KVM (`make gguf-kvm-benchmark`).
- **Évaluation des métriques de latence** : Établir le rapport (min, médiane, max, dispersion) isolant le temps du premier jeton et la réutilisation du cache KV multi-tours.
- **Validation** : Exécution de `make gguf-kvm-benchmark-check`.
