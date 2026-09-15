# AOS-025 — Stub réseau bare-metal et profil OpenAI

**Statut :** jalon initial **stub OpenAI controle**. Un pilote NE2000 ISA et des codecs caller-owned existent (AOS-113...154). Les lots suivants ont ajoute sockets utilisateur, `ai-acquire` et un TLS/HTTP local sur pair QEMU ; OpenAI public reste hors livraison. Voir [ETAT_REEL.md](ETAT_REEL.md).

**Date du jalon :** 17 aout 2026. **Constat courant :** 23 aout 2026.

## Objet

Le shell historique possédait un profil `ai-provider openai`, mais le noyau ne disposait d’aucun transport pour réaliser une requête. Cette situation pouvait laisser croire qu’une sélection de fournisseur correspondait à une intégration OpenAI. Le jalon AOS-025 rend ce contrat explicite et testable.

| Élément | État dans cette livraison |
|---|---|
| `ai-provider openai` | Sélection de profil seulement |
| `ai <texte>` avec profil OpenAI | Refus explicite, aucune requête émise |
| `net-status` / `net-status json` | Diagnostic : NIC absente ou NE2000 détectée ; ARP/IPv4/DHCP/DNS/TCP/TLS affichés absents |
| Pilote Ethernet | NE2000 ISA sondé au boot (`0x300`) ; smoke `make qemu-ne2k-status` |
| ARP / IPv4 / DHCP / DNS / TCP | Codecs et TX/RX caller-owned unit-testés ; **pas** de configuration live ni de commande shell |
| TLS / HTTP | Le jalon initial n'offrait que le framing. L'etat courant ajoute un handshake TLS 1.2 authentifie et un POST HTTP local ; voir [ETAT_REEL.md](ETAT_REEL.md). |
| Clé API dans l’image | Interdite |

> Le résultat attendu n’est pas une connexion simulée : l’utilisateur doit voir qu’aucune requête OpenAI ne peut partir du noyau actuel.

## Validation

Le contrat `make qemu-ai-provider` démarre l’image QEMU, vérifie le profil local, appelle `net-status`, sélectionne `openai`, exécute `ai hello` et exige le message d’indisponibilité. Il vérifie donc que le profil est visible sans produire un faux succès réseau.

Le contrat agrégé est :

```bash
make integration-qemu
```

Il comprend AOS-022 (boot et shell), AOS-024 (préemption IRQ0) et AOS-025 (profil fournisseur).

## Relation avec QEMU

QEMU peut émuler des cartes réseau ISA ou PCI et les relier à un backend hôte. Le backend utilisateur fournit notamment DHCP sans privilèges et réserve typiquement `10.0.2.2` au routeur virtuel, `10.0.2.3` au DNS et des baux à partir de `10.0.2.15` [1]. Cette fonction appartient à l’émulateur : elle ne donne pas au noyau invité un pilote NIC ou une pile TCP/IP.

Une expérimentation future pourra démarrer QEMU avec un contrôleur ISA compatible NE2000 et le backend utilisateur :

```bash
qemu-system-i386 \
  -kernel build/mohhdy.bin -initrd my_initrd.tar -m 1024M \
  -netdev user,id=n0 -device ne2k_isa,netdev=n0
```

Cette ligne est le contrat de `make qemu-ne2k-status`. Le noyau sonde désormais `ne2k_isa` ; elle ne démarre pas DHCP, TCP, TLS ni HTTP dans le guest.

## Critères de sortie d’un client OpenAI effectif

Le passage du stub au client réseau demande encore, au-delà des primitives déjà livrées :

| Ordre | Composant | État | Critère de sortie restant |
|---|---|---|---|
| 1 | Pilote NIC | Partiel | Chemin live utilisé par le shell, pas seulement la sonde |
| 2 | Ethernet/ARP | Codecs + TX/RX | Résolution et cache raccordés à une config IPv4 live |
| 3 | IPv4/UDP/DHCP | Codecs + Discover/ACK | Bail automatique et route sans secret |
| 4 | DNS/TCP | Codecs + SYN/ACK/data | Flux TCP utilisateur, temporisations |
| 5 | TLS | Record framing | Handshake, certificats, secrets hors image |
| 6 | HTTP OpenAI | Absent | Requête autorisée, secret injecté hors image |

Aucune clé API ne doit être committée, placée dans l’initrd, écrite dans l’overlay par défaut, copiée sur un futur volume FAT ou reproduite dans la sortie série. Une future interface de configuration devra demander le consentement de l’utilisateur avant toute requête externe.

Le volume FAT ([aos_fat_volume.md](aos_fat_volume.md)) est **orthogonal** à cette suite réseau : il persiste des fichiers sur IDE, il ne crée pas de transport.

## Références

[1] [QEMU, *Network emulation*](https://www.qemu.org/docs/master/system/devices/net.html)
