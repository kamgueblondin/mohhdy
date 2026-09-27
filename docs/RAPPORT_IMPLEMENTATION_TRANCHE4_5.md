# Rapport d'implémentation : Tranche 4 suite et Tranche 5 suite

**Date :** 27 septembre 2026  
**Base :** `main` @ `b56ac7b` ; Tranche 4 suite commitée en `220d0ae`, Tranche 5 suite dans le commit qui ajoute ce rapport.  
**Hôte de validation :** Debian 12 arm64, chaîne croisée `i686-linux-gnu` (GCC 12.2), NASM 2.16.01, QEMU 7.2 (TCG, pas de KVM). Build : `make CC=i686-linux-gnu-gcc LD=i686-linux-gnu-ld AS=i686-linux-gnu-as all`. Les ELF de tests Unity i386 sont exécutés par `qemu-i386 -L /usr/i686-linux-gnu` (pas de binfmt sur l'hôte), les contrats QEMU par leurs cibles `make` d'origine.

## 1. Résumé

| Tranche | Objectif | Statut |
|---|---|---|
| 4 suite | La logique FAT16/FAT32/overlay est exécutée par le pilote Ring 3 `atadriver`, et plus seulement le PIO secteur | **Fait** : `fskernel 0` pendant que le pilote est vivant, repli sans rejeu |
| 5 suite | La NE2000 est pilotée depuis Ring 3 par `networker` via le bitmap I/O du TSS (ports 0x300-0x31F) + IRQ3 | **Fait** : 7/7 trames émises par le pilote Ring 3, IRQ3 servie par le worker, 0 accès noyau, reprise après mort du worker |
| 5 (pile) | Framing ARP/IPv4/TCP, TCP, TLS hors Ring 0 | **Ouvert**, non revendiqué |

Zéro régression : les 588 tests de référence passent toujours, et 17 tests ont été ajoutés (605/605) ; tous les contrats QEMU exigés passent.

## 2. Tranche 4 suite : ATA/FAT hors du noyau (commit `220d0ae`)

- `SYS_ATA_FS` (145), réservé au `ata-driver` vivant : passage du store (`READY`), requête/réponse, notes de progression, publication de l'image overlay, sondes initrd, statut public.
- `kernel/ata_fsop.c` (logique pure) : propriété du store, un slot d'opération `FREE → SUBMITTED → FETCHED → DONE/ABORTED`, miroir overlay (30224 octets), compteurs.
- `kernel/fs/fsop_exec.c` : règles de nommage FAT et codec/dispatch des opérations, partagés entre le repli noyau et le pilote.
- `atadriver` lie ses propres objets FAT16/FAT32/overlay, monte les volumes par son PIO, prend le store (un passage) et persiste les LBA 0-63 de façon incrémentale ; le noyau ne fait que copier des octets.
- Repli : à la mort du pilote, le store revient en Ring 0 depuis le miroir. Une opération en vol n'est refaite que si aucun secteur n'était commis (sinon `OS_ATA_FS_ABORTED`). Un store détenu mais indisponible répond `OS_ATA_FS_UNAVAILABLE` (-137), sans exécuter la copie noyau périmée.
- Restent Ring 0 par conception : montage et chargement au boot (aucune tâche n'existe encore), plus la copie de repli sans pilote.

## 3. Tranche 5 suite : NE2000 en Ring 3 via l'IOPB

| Élément | Implémentation |
|---|---|
| Propriété | `kernel/net_nic_owner.c` (pur) : propriétaire 0 (noyau) ou PID du `net-driver` vivant ; claim réservé à ce PID, carte sondée obligatoire (-143), idempotent, pas de vol par un autre worker |
| ABI | `SYS_NET_NIC` (146) : `CLAIM`, `PUMP`, `IRQ` réservés au propriétaire, `STATUS` public (`os_net_nic_status_t`) ; codes -141 `OS_NET_NIC_WORKER_OWNED`, -142 `OS_NET_WIRE_PENDING`, -143 `OS_NET_NIC_ABSENT` |
| Ports | `io_bitmap_apply_ne2k()` ouvre exactement 0x300-0x31F ; `schedule()` → `tss_set_nic_io()` à chaque commutation, ouvert seulement pour le propriétaire encore `net-driver` ; #GP ailleurs (seule la tâche fautive meurt) ; indépendant de la fenêtre ATA |
| Noyau hors-jeu | Accès NE2000 noyau via une porte `kernel_nic_inb/outb` qui refuse et compte (`kernel_refused`) ; LLM 91-98 et peer 128-130 refusés (-141, `kernel_gated`) ; maintenance DHCP suspendue |
| IRQ3 | Le handler noyau compte seulement (`nic_owner_irq`), l'EOI reste dans le stub ; le worker active PRX\|PTX dans l'IMR, lit et acquitte l'ISR lui-même, collecte le compte par `OS_NET_NIC_IRQ` |
| Pilote Ring 3 | Cœur matériel extrait de `ne2k.c` vers `kernel/ne2k_hw.c` (sonde, PROM/MAC, anneaux, TX DMA distant, poll RX), compilé aussi dans `networker` (`-DMOHHDY_RING3`) ; le worker re-sonde la carte à CPL 3 et relit la PROM (`prom-match 1`) |
| Moteur reprenable | `net_wire.c` : boucles bloquantes de la slice 3 transformées en machine à états, une opération à la fois (`net_wire_op_*`, `net_wire_op_step`), avec les trames sortantes via un puits `emit` ; les points d'entrée Ring 0 bloquants restent pour le mode sans worker |
| Pompe | Appels fil → -142, puis boucle `PUMP` du worker : jusqu'à 4 trames à émettre sont copiées, il les émet par son DMA et réinjecte la trame reçue ; en ronde inactive, il attend un tick **sans céder le CPU** (comme `wire_pause()` en Ring 0) |
| Perte du worker | Fenêtre fermée, opération annulée (sans rejeu), carte réinitialisée en Ring 0 (IMR off) ; un nouveau worker peut réclamer |

### Correctifs trouvés pendant la validation

1. **Timeout du relais (-87) sur `connect`** : la ronde inactive du worker faisait `yield()`, ce qui ne donnait qu'une ronde par tour du shell et dépassait le délai de 5 s. Corrigé : attente du tick sans céder la main, comme le chemin Ring 0. Après correction, le `connect` tient en 4 pompes.
2. **IRQ3 jamais délivrée** : le `ne2k_isa` de QEMU est par défaut en **IRQ 9**, alors que l'OS (et `OS_NET_NIC_IRQ_LINE`) suppose IRQ3. Le contrat `qemu-net-wire` déclare désormais `irq=3` et exige que l'IRQ3 soit servie par le worker. `qemu-net-worker` garde la carte par défaut ; son contrat ne dépend pas de l'IRQ.

## 4. Tests ajoutés

| Binaire | Tests | Contenu |
|---|---:|---|
| `test_ata_fsop` (T4) | 7 | slot, store, miroir, abort/redo, compteurs |
| `test_net_nic_owner` (nouveau) | 6 | règles de claim, ports ↔ propriétaire, drop/reclaim, IRQ forwarding, compteurs, ABI |
| `test_io_bitmap` | +2 (6) | fenêtre NE2000 seule ; indépendance NE2000/ATA |
| `test_net_wire` | +2 (8) | moteur via puits réel : ARP, une opération à la fois, réponse ARP apprise puis SYN adressé à la MAC apprise, cancel libère la liaison, timeout borné, puits refusant = 0 trame comptée, points d'entrée Ring 0 refusent un contexte puits |

`test_qemu_net_wire.py` est étendu sans assertion retirée : il vérifie la remise des ports, `prom-match 1`, les compteurs du pilote Ring 3 et la cohérence TX worker = RX pair, puis tue le worker et vérifie la reprise noyau et le re-claim par un nouveau PID.

## 5. Résultats de validation (27 septembre 2026, même arbre que le commit)

| Porte | Référence (`b56ac7b`) | Résultat |
|---|---|---|
| Unity (`make -C tests all` + exécution i386) | 48 binaires, 588/588 | **50 binaires, 605/605, 0 échec** (noyau 45/546, userspace 4/56, robustesse 1/3) |
| Contrôles de `make test-all` | registre 198 OK, façade retirée OK | **OK / OK** |
| `make qemu-smoke` | 6/6 | **6/6** (core, extras, persist, spawn, syscalls, exec) |
| `make integration-qemu` | 7/7, 813,1 s | **7/7, 820,9 s** (vfs-service 526,4 s, service-grant 82,4 s, core 40,2 s, ipc 17,0 s, ai-provider 139,3 s, irq0 3,2 s, ne2k-status 12,3 s) |
| `make qemu-net-wire` | passé | **passé** : trames invité→pair 7, pair→invité 4, ARP 1, 4 appels relayés, écho 16 octets, 9 accès bruts refusés, 0 erreur de checksum ; Ring 3 : TX 7 = sortie 7 = RX pair 7, RX 4 = entrée 4, IRQ3 9 servies (10 comptées), 10 pompes, **0 accès noyau**, reprise + re-claim OK |
| `make qemu-net-worker` | passé | **passé** : relais fwd 16 / done 16 / refusés 4, timeout du worker bloqué 1, abort du worker tué 1 ; claim puis reprise noyau observés |
| `make qemu-ata-driver` | passé | **passé** : `fat via driver rd=65 wr=4 kfat=0`, PIO FAT noyau 17→17, repli 86, rechargement au reboot, crash en plein job : aborts 0→1, resets 0→1 |
| `make qemu-ne2k-tls-multipair` | passé | **passé** (chemin NE2000 Ring 0 sans worker, après extraction de `ne2k_hw.c`) |
| `make osui-smoke`, `make qemu-osui-runtime` | passé | **passés** |

Non exécuté ici, et non revendiqué : benchmarks GGUF KVM (pas de `/dev/kvm`) et scénarios avec vrais poids GPT-2. Ce périmètre est inchangé depuis `BILAN.md`.

## 6. Contrats satisfaits

- **T4 :** ports ATA à CPL 3 pour le seul pilote ; logique FAT/overlay servie par le pilote (`fskernel 0`) ; repli sans rejeu ; reprise par un pilote relancé ; persistance au reboot. README passé en `[x]`.
- **T5 suite :** ports 0x300-0x31F ouverts pour le seul worker propriétaire via l'IOPB ; E/S NE2000 exécutées à CPL 3 ; IRQ3 servie par le worker ; aucun accès noyau pendant la détention (mesuré 0) ; chemins noyau concurrents refusés ; reprise et re-claim. Nouvel item README en `[x]`.
- **Zéro régression** sur les 588 tests et sur tous les contrats QEMU exigés.

## 7. Limites et suite

- La pile reste en Ring 0 : framing ARP/IPv4/TCP, demux, machine TCP et registre de sockets, TLS. L'item README « Tranche 5 isolation réseau (pile) » reste `[~]`. Prochaine étape : déplacer le framing et le demux de `net_wire.c` dans le worker, le noyau ne gardant que le relais et le registre.
- Tant que le worker détient la carte, les chemins LLM/TLS/peer noyau sont refusés (-141) au lieu d'être re-routés. `networker` n'est pas lancé au boot, donc le comportement par défaut est inchangé.
- Le rythme reste en polling (un tick par ronde inactive) : l'IRQ3 est comptée et acquittée, mais le worker ne se bloque pas encore dessus. Une opération fil à la fois, 4 trames par pompe au plus.
- T4 : montage et chargement au boot en Ring 0, et copie de repli FAT/overlay conservée dans le noyau.
