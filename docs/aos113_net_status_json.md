# AOS-113 — Diagnostic réseau machine-lisible

## Objectif

Le lot AOS-113 ajoute une représentation JSON déterministe à la commande `net-status`. Cette représentation permet aux scripts de démarrage, aux tests QEMU et aux outils de diagnostic de distinguer explicitement l’absence de transport réseau d’un succès OpenAI.

## Contrat

La commande interactive historique reste disponible :

```text
net-status
```

La variante machine-lisible est :

```text
net-status json
```

Elle produit une seule ligne JSON compacte, sans texte décoratif :

```json
{"nic":"absent","ethernet":"absent","arp":"absent","ipv4":"absent","dhcp":"absent","dns":"absent","tcp":"absent","tls":"absent","openai":"blocked"}
```

Avec `-device ne2k_isa`, le smoke `make qemu-ne2k-status` exige `"nic":"detected"` et `"ethernet":"configured"`. ARP, IPv4, DHCP, DNS, TCP et TLS restent `absent` : les codecs existent, aucune configuration live n’est raccordée. `openai` reste `blocked`.

| Champ | Valeurs observées | Signification |
|---|---|---|
| `nic` | `absent` ou `detected` | Carte NE2000 ISA absente, ou sondée au boot (`0x300`) |
| `ethernet` | `absent` ou `configured` | Anneaux RX/TX configurés lorsque la NIC est présente |
| `arp` | `absent` | Codec ARP unit-testé ; pas de table live |
| `ipv4` | `absent` | Codec IPv4/UDP unit-testé ; pas d’adresse attribuée |
| `dhcp` | `absent` | Discover/ACK caller-owned ; pas de bail automatique |
| `dns` | `absent` | Codec DNS A unit-testé ; pas de résolution shell |
| `tcp` | `absent` | SYN/ACK/payload caller-owned ; pas de socket utilisateur |
| `tls` | `absent` | Framing TLS record seulement ; pas de handshake |
| `openai` | `blocked` | Le fournisseur en ligne est refusé tant qu’un transport complet n’existe pas |

> Le champ `openai=blocked` est volontaire : la sélection du profil `ai-provider openai` ne doit jamais être interprétée comme l’émission effective d’une requête réseau.

## Validation

Le test `tests/scripts/test_ai_provider_commands.py` vérifie les deux formes de diagnostic **sans** carte NE2000. Il démarre l’image i386 sous QEMU, exécute `net-status`, puis `net-status json` et recherche `nic=absent` et `openai=blocked`. Le smoke avec carte est `make qemu-ne2k-status` (`nic=detected`).

La suite de non-régression courante est dans [ETAT_REEL.md](ETAT_REEL.md) (**299** tests). Le build complet doit être lancé avec `make all`. Le smoke NIC est `make qemu-ne2k-status` ; le smoke OpenAI bloqué reste :

```sh
make test-all
make all
python3 tests/scripts/test_ai_provider_commands.py
```

Ce lot ne prétend pas implémenter un transport live. Les lots AOS-114 et suivants ajoutent codecs et pilote NE2000 ; `net-status json` continue d’afficher `openai=blocked`.

## Statut

AOS-113 est une amélioration de diagnostic et de contrat de contrôle, sans allocation dynamique et sans modification du chemin critique GGUF. Il est compatible avec le mode texte historique et ne produit aucun paquet réseau.
