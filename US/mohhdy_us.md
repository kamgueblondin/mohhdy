# User stories MOHHDY - backlog du prototype i386

**Date :** 26 août 2026
**Source de verite runtime :** [docs/ETAT_REEL.md](../docs/ETAT_REEL.md)  
**Perimetre :** hobby OS i386 Multiboot, QEMU, shell Ring 3 et IA locale - pas une distribution Linux. Ce document ne constitue pas le plan MOHHDY a long terme. Lexique : [docs/vocabulaire.md](../docs/vocabulaire.md).

La mention **fait** signifie que le comportement est observable dans le code et couvert par une commande de verification. Les limites explicitement indiquees font partie du resultat : elles ne doivent pas etre confondues avec des fonctionnalites livrees.

## Socle deja livre

| ID | User story | Etat verifie |
|---|---|---|
| AOS-001 | Demarrer un noyau Multiboot i386 | Boot QEMU, VGA/serie, curseur bloc, historique Page Up/Down, `make run`, `make run-gui` et ISO GRUB BIOS |
| AOS-002 | Gerer memoire physique, virtuelle et tas | PMM, VMM, heap, paging et `SYS_MEMINFO` |
| AOS-003 | Recevoir timer et clavier | PIC/PIT 100 Hz/i8042 ; prefixe `0xE0` (Page Up/Down, fleches) ; EOI IRQ0 avant le gestionnaire C |
| AOS-004 | Lire un initrd | Archive TAR (ustar) en lecture seule, `SYS_LISTDIR`, `SYS_READFILE` |
| AOS-005 | Executer un shell isole | Shell ELF utilisateur et retour Ring 3 par `iret` |
| AOS-006 | Exposer une ABI de syscalls | ABI propre `int 0x80` ; aujourd’hui syscalls 0-126, `MAX_SYSCALLS = 127` |
| AOS-007 | Conserver de petits fichiers | Overlay ATA PIO persistant V2 et restauration V1/V2 |
| AOS-008 | Gerer plusieurs taches | `spawn`, `yield`, `ps`, `kill`, plus preemption IRQ0 sure |
| AOS-009 | Executer un ELF bloquant | `exec`, parent `TASK_WAITING`, reveil par `SYS_EXIT` |
| AOS-010 | Completer localement avec GPT-2 | `SYS_GPT2_GENERATE`, GPT-2 124M optionnel, cache KV et SSE2 |
| AOS-011 | Tokeniser BPE GPT-2 | Vocabulaire/fusions BPE et decodage UTF-8 brut |
| AOS-012 | Prévenir les régressions | 506 tests C et sept contrats QEMU versionnés, dont `qemu-ne2k-status`, `qemu-ne2k-acquire`, `qemu-ne2k-tls-http`, `qemu-ne2k-tls-sse`, `qemu-ne2k-tls-close`, `qemu-ne2k-tls-next` et `qemu-vfs-service` |

## Tranche AOS-020 à AOS-025 — livrée

### AOS-020 — Compatibilité GGUF et réduction de latence

**En tant que** mainteneur du runtime local, **je veux** accepter de façon bornée la structure des checkpoints GGUF et préparer la quantification, **afin de** pouvoir évoluer au-delà du checkpoint FP32 initial.

**Livraison.** `gpt2_gguf.c` analyse GGUF v3, ses métadonnées, les descripteurs de tenseurs et leurs alignements. Le parseur a été validé contre un checkpoint GPT-2 réel contenant F32, Q3_K, Q4_K et Q6_K. `gpt2_quant.c` fournit FP16→FP32, le produit Q8_0×FP32 et les produits freestanding Q3_K/Q4_K/Q6_K × activation FP32, avec les tailles de super-blocs GGML (256 valeurs) et les contrôles de longueur associés. Le rapport GGUF expose désormais les compteurs de tenseurs K-quants supportés et `gpt2_gguf_find_tensor` recherche un descripteur borné par nom, forme, type, offset et taille calculée. Les tests Unity et les tests de robustesse couvrent les préfixes tronqués, compteurs excessifs, alignements invalides, comparaisons numériques synthétiques et recherche bornée de tenseurs Q4_K.

**Limite.** Les lots suivants ont ajoute un index caller-owned, un mapping de roles et de couches, un chargement FAT16 et un forward quantifie. `ai-model use gpt2.gguf` puis `ai <texte>` / `ai-continue` selectionnent ce chemin local. Sous QEMU TCG sans KVM, le premier jeton reste de l'ordre de 48 s et la continuation de l'ordre de 23 s : ce n'est pas l'objectif inferieur a une seconde, et ce n'est pas une mesure materielle.

**Vérification.** `make test-all` (constat à la livraison : 257 tests ; chiffre courant dans [ETAT_REEL.md](../docs/ETAT_REEL.md)), puis `make clean && make all`. La conception et les références de layouts sont documentées dans [docs/aos020_gguf_quantization_design.md](../docs/aos020_gguf_quantization_design.md) et [docs/research_ggml_kquant_reference.md](../docs/research_ggml_kquant_reference.md).

### AOS-021 — BPE Unicode

**En tant qu’**utilisateur, **je veux** que les prompts non ASCII soient segmentés avec une logique de lettres Unicode utile, **afin de** ne pas réduire tous les textes internationaux à des octets opaques.

**Livraison.** Le tokenizer valide l’UTF-8 sur 2, 3 et 4 octets, rejette les formes overlong et les surrogates, puis reconnaît les lettres Latin étendu, Grec, Arabe, Hébreu, Devanagari, CJK et Hangul. Les emoji et symboles restent des séparateurs BPE, ce qui est testé.

**Limite.** Cette implémentation n’est pas une réimplémentation exhaustive de `\p{L}` Unicode.

### AOS-022 — Contrat d’intégration QEMU

**En tant que** contributeur, **je veux** une intégration QEMU versionnée, **afin de** vérifier un boot et un shell réels au-delà des tests C isolés.

**Livraison.** `tests/integration/test_qemu_core_contract.py` vérifie le boot, `ai-runtime`, l’overlay, la copie, `append` et le retour au shell. Son disque overlay de test est isolé de `build/overlay.img`, de sorte que le contrat est idempotent.

**Vérification.** `make integration-qemu`.

### AOS-023 — Stockage étendu

**En tant qu’**utilisateur, **je veux** des fichiers overlay plus nombreux et plus grands, **afin de** dépasser le snapshot de démonstration initial.

**Livraison.** Le format AIOV V2 porte l’overlay à 64 nœuds, 80 octets par chemin, 384 octets de données et 64 secteurs ATA. La restauration lit aussi les snapshots V1. Les tests couvrent une restauration V1 et un fichier V2 étendu.

**Limite.** L'overlay reste un petit snapshot AIOV persistant (64 noeuds, 384 octets). Le volume FAT16 (AOS-026 puis mutations 8.3 racine) est un support distinct a partir du LBA 64. Voir [docs/aos_fat_volume.md](../docs/aos_fat_volume.md).

### AOS-024 — Préemption IRQ0 sûre

**En tant qu’**utilisateur, **je veux** qu’une tâche Ring 3 qui ne coopère pas ne gèle pas le shell, **afin de** pouvoir garder une interaction clavier fiable.

**Livraison.** Le timer applique un quantum de 20 ticks seulement pour un cadre ayant `CS` et `SS` Ring 3 et lorsqu’une autre tâche utilisateur est `READY`. Cette garde interdit un `schedule()` depuis un cadre noyau incomplet. L’EOI IRQ0 reste placé avant le gestionnaire C.

**Vérification.** `make qemu-irq0-preemption` lance `spawn spin`, où `spin` ne fait aucun syscall, puis exige `echo irq0-preempt-ok` depuis le shell.

### AOS-025 — Réseau minimal et fournisseur OpenAI

**En tant qu’**utilisateur, **je veux** savoir exactement si le fournisseur en ligne peut émettre une requête, **afin de** ne pas croire qu’un profil sélectionné est déjà un client OpenAI fonctionnel.

**Livraison initiale.** `ai-provider openai` reste un profil explicite. `net-status` / `net-status json` publient la presence reelle d'une NIC NE2000 ISA (`SYS_NET_STATUS`). Sans carte, `nic=absent` ; avec `-device ne2k_isa`, `make qemu-ne2k-status` exige `nic=detected`.

**Livraison ulterieure (23 aout 2026).** Les lots suivants ont raccorde un registre TCP utilisateur (syscalls 99-108), une session LLM noyau (90-98) et les commandes `ai-acquire` / `ai-tls-poll` / `ai-request`. `net-status` annonce alors `ethernet=configured`, `arp=on-demand`, `ipv4=dhcp`, `dns=on-demand`, `tcp=socket`, `tls=authenticated` et `openai=credential-required`. `make qemu-ne2k-acquire` observe DHCP, ARP, DNS A, SYN, SYN-ACK, ClientHello, ServerHello minimal et ACK contre un pair Ethernet local controle. `make qemu-ne2k-tls-http` complete le handshake TLS 1.2 authentifie local (`example.com`, ancre de test, `TLS_COMPLETE`) puis un POST ollama et l'extraction HTTP 200 `ok`. `make qemu-ne2k-tls-sse` enchaîne `ai-stream-request` et `ai-sse-poll` sur un flux chunked local (`SSE : ok`, puis `flux SSE termine`). `make qemu-ne2k-tls-close` remplace le terminateur applicatif par un `close_notify` TLS chiffré du pair, exige son ACK TCP et vérifie la réponse `close_notify` authentifiée du noyau. `make qemu-ne2k-tls-next` valide `ai-next` puis un second POST (HTTP puis SSE) sur la même session TLS. `make qemu-ne2k-tls-multipair` exécute deux cycles TLS/HTTP indépendants et séquentiels : chaque pair utilise seulement `127.0.0.1`, une MAC et un journal distincts, et valide DHCP, ARP, DNS, TCP, TLS authentifié et HTTP 200. La saisie confirme chaque caractère avant l’entrée et ne rejoue aucune ligne réseau. Ce n’est pas Internet public, pas un TLS vers un hôte réel, et pas OpenAI.

**Limite et suite.** Un client OpenAI reel exige encore un secret hors initrd et un hote public. QEMU peut relier `ne2k_isa` a un backend utilisateur [1] ; le guest a un pilote, un TLS local et un HTTP local, pas un client HTTPS public.

**Vérification.** `make qemu-ai-provider`, `make qemu-ne2k-status`, `make qemu-ne2k-acquire`, `make qemu-ne2k-tls-http`, `make qemu-ne2k-tls-sse`, `make qemu-ne2k-tls-close`, `make qemu-ne2k-tls-next` et `make qemu-ne2k-tls-multipair`.

### AOS-026 — Volume FAT sur disque IDE (lot 68 livré)

**En tant qu’**utilisateur, **je veux** un volume FAT sur le disque IDE, **afin de** lire des fichiers plus grands que l’overlay AIOV sans adopter un système à inodes.

**Livraison lecture seule (lot 68).** Le noyau monte un volume FAT16 prepare sur le disque IDE a partir du LBA 64, sans toucher aux 64 secteurs AIOV. `fat16-list` liste la racine 8.3 et `fat16-cat <8.3>` lit les fichiers chaines par la FAT. Note : [docs/mohhdy_foundation_increment_68_fat16_volume.md](../docs/mohhdy_foundation_increment_68_fat16_volume.md).

**Livraison ultérieure (27 août 2026).** Le VFS expose `fat16/` et `fat32/` en création, suppression et renommage de fichiers 8.3 ou LFN à la racine, puis en `mkdir`, écriture, statut, listage paginé, renommage, suppression et `rmdir` vide dans **un seul** sous-répertoire 8.3. Les mutations fixes ainsi que les ajouts, retraits et I/O (`read`, `stat`, liste, pages et observation) des alias dynamiques sont validés et résolus par `vfsvirtual` Ring 3 après messages privés corrélés ; le médiateur ne conserve qu’un miroir volatil. Chaque transaction worker reçoit désormais un scope backend d’une **seule source** (`initrd`, `overlay`, `fat16` ou `fat32`) avec `read` ou `mutate`, et un préfixe relatif canonique minimal vérifié au noyau, puis il est révoqué. Les syscalls FAT bruts sont eux aussi backend-protégés ; `fat16-list` et `fat16-cat` passent donc par le médiateur VFS. Une lecture générique exige toutes les sources et ne contourne pas ce scope. Au remplacement, à la disparition ou à l’expiration incertaine du worker, le miroir est purgé, le client reçoit `INVALID` et aucune I/O ou mutation n’est rejouée. Les contrats unitaires prouvent les refus inter-sources et de frontière de préfixe ; le contrat QEMU valide les cycles FAT, la délégation d’alias, l’observation pendant une lecture suspendue du scope unique `read initrd`, le refus backend corrélé d’un renommage vers un voisin hors préfixe avant mutation, la révocation mono-source et l’absence de rejeu. Le diagnostic `vfs-backend-scope <pid>` est corrélé et n’expose que droits, sources et identifiant de requête, jamais un préfixe, chemin ou alias ; le contrat QEMU le prouve pendant une lecture enfant suspendue. Le préfixe reste interne au noyau et n’est pas divulgué par le diagnostic ; une primitive FAT sans chemin n’est utilisable que sous scope racine. L’écrasement, le second niveau, les LFN enfants, le renommage inter-répertoire et le remplacement transactionnel restent hors contrat. Un second disque IDE porte FAT32. **ext2 n’est pas une option.**

**Verification.** `make test-all` (**512/512**), `make qemu-vfs-service`, `make integration-qemu` (sept contrats séquentiels, **760,9 s** en validation locale, sans assertion retirée) puis `make qemu-ne2k-tls-multipair`. Le smoke cœur conserve sa confirmation PS/2 ; les contrats VFS et IRQ0 réconcilient désormais la ligne entière avant `ret`, effacent localement toute divergence puis ne rejouent jamais une ligne entrée.

## Tranche reseau AOS-113 a AOS-154 - primitives, puis raccordement local

Les lots 113-154 sont **faits** au sens caller-owned / Unity / smoke NIC. Les lots suivants ont ajoute les sockets utilisateur 99-108 et `ai-acquire`. Ils ne livrent pas un daemon DHCP public ni OpenAI. Detail : [docs/ETAT_REEL.md](../docs/ETAT_REEL.md).

| Lots | User story condensée | Vérification |
|---|---|---|
| AOS-113, AOS-132 | Diagnostic machine-lisible et bitmask `SYS_NET_STATUS` | `net-status json`, `make qemu-ne2k-status` |
| AOS-116 | Accès PCI borné pour une future NIC PCI | `test_pci` ; le boot reste ISA `0x300` |
| AOS-117–136 | Sonde NE2000, PROM MAC, anneaux, TX PIO, RX poll, IRQ3 | `test_ne2k`, boot `ne2k_boot_probe` |
| AOS-114, AOS-137–143 | Ethernet/ARP, réponse, cache, résolution active | `test_net_ethernet_arp` |
| AOS-121–123, AOS-140–146 | IPv4/UDP, DHCP Discover/Offer/Request/ACK, DNS A | `test_net_ipv4_udp`, `test_net_dhcp`, `test_net_dns` |
| AOS-124–125, AOS-147–154 | SYN, SYN-ACK, ACK, payload, séquence, retransmission bornée | `test_net_tcp` |
| AOS-126–128 | SHA-256, HMAC-SHA-256, framing TLS 1.2 record | `test_sha256`, `test_net_tls_record` |

**Limite commune des lots 113-154.** Ces lots historiques ne livraient ni bail automatique, ni socket utilisateur, ni handshake TLS. Cela a changé ensuite : voir AOS-025 ci-dessus pour le parcours TLS/HTTP/SSE authentifié sur pair QEMU local. Il reste hors livraison : DHCP sur réseau public, TLS vers un hôte réel et OpenAI réel.

## ABI observable (26 août 2026)

| Plage | Role |
|---|---|
| 0-89 | Socle historique, dont `SYS_NET_STATUS` (89) |
| 90-98 | Session LLM noyau (`SYS_LLM_*`, credential) |
| 99-108 | Sockets TCP utilisateur (`SYS_SOCKET_*`) |
| 109-110 | Generation GGUF locale et `ai-continue` |
| 111-114 | Lecture et pagination FAT32 / pages FAT16 |
| 115 | Liberation d'une capacite backend |
| 116-118 | Création, suppression et renommage FAT16 8.3 ou LFN à la racine |
| 119-121 | Création, suppression et renommage FAT32 8.3 ou LFN à la racine, ou 8.3 sous un niveau |
| 122-123 | Listage FAT16/FAT32 par chemin, avec capacité en EDX et départ de page en ESI |
| 124 | Grant backend VFS source-scopé : droits en EDX, masque de sources en ESI |
| 125 | Statut interne borné du couple droits–sources backend |
| 126 | Grant backend VFS droit–source–préfixe relatif : EDX droits, ESI sources, EDI préfixe NUL-terminé |

`MAX_SYSCALLS = 127`.

## Prochaines tranches, hors livraison actuelle

| Priorite | Sujet | Critere de sortie |
|---|---|---|
| 0 | Budget CI QEMU | Conserver les sept contrats séquentiels sous 25 minutes et le smoke multi-pairs dans GitHub Actions, sans retirer d’assertion métier ni rejouer une mutation ou I/O incertaine ; la dernière mesure locale est 760,9 s |
| 1 | Couverture de l’ACL préfixée | Conserver les preuves négatives de voisin, racine et voie sans chemin ; le diagnostic public reste droit–source et ne doit jamais divulguer le préfixe interne |
| 2 | Topologie réseau locale partagée optionnelle | Seulement après une injection PS/2 démontrée avec plusieurs QEMU simultanés ; sans TAP, clé, Internet public ni OpenAI |
| 3 | Latence locale | Mesure sous matériel/KVM sur une plateforme de référence ; QEMU TCG reste ~48 s / ~23 s |

La vision MOHHDY (microkernel, P2P, économie, multi-plateforme, etc.) reste une collection de spécifications dans `US/`. Elle ne doit pas être utilisée comme indicateur d’implémentation du prototype.

## Références

[1] [QEMU, *Network emulation*](https://www.qemu.org/docs/master/system/devices/net.html)
