# AOS-114 — Codec Ethernet/ARP caller-owned

## Objectif

Le lot AOS-114 introduit la première brique du réseau effectif sans simuler une carte réseau. Le noyau peut désormais décoder un en-tête Ethernet, parser une trame ARP Ethernet/IPv4 et construire une requête ARP dans un buffer fourni par l’appelant.

## Contrat mémoire

Les fonctions de `kernel/net_ethernet_arp.c` n’allouent aucune mémoire. Les buffers d’entrée restent sous le contrôle de l’appelant et les fonctions de parsing copient les champs utiles dans des structures de sortie caller-owned. Toute longueur insuffisante est rejetée avant lecture.

| Fonction | Contrat |
|---|---|
| `net_ethernet_parse` | Lit exactement les 14 octets d’un en-tête Ethernet et convertit l’EtherType big-endian. |
| `net_arp_parse` | Accepte uniquement ARP Ethernet/IPv4 avec tailles matérielle 6 et protocolaire 4. |
| `net_arp_build_request` | Construit une requête ARP broadcast de 42 octets dans le buffer fourni. |
| `net_arp_is_reply_for` | Vérifie opcode, IPv4 local cible et IPv4 demandée source. |

> Le codec ne transmet encore aucune trame. Il fournit une primitive déterministe au futur pilote NIC et évite de confondre la préparation de paquet avec un transport réseau opérationnel.

## Validation

Le test `tests/unit/kernel/test_net_ethernet_arp.c` couvre la construction et le parsing d’une requête, le rejet des buffers courts et des EtherTypes non ARP, ainsi que la reconnaissance d’une réponse correspondant aux adresses demandées. Le runner `tests/scripts/run_all_tests.sh` lie explicitement l’implémentation du codec au test, comme les autres modules kernel spécialisés.

La validation locale du lot est :

```text
make test-all                  268 tests, 268 passés, 0 échec, 0 ignoré
make all                       build i386 et initrd réussis
```

Les étapes réseau suivantes restent nécessaires avant une requête effective : détection PCI/NIC, anneau RX/TX, émission matérielle, IPv4/UDP/DHCP, DNS, TCP, TLS et client HTTP borné. Aucune clé API et aucun secret ne sont intégrés à l’image.
