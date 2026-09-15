# AOS-2061…2068 — Bootstrap LLM NE2000 validé sur segment Ethernet QEMU contrôlé

**Statut : livré et validé localement.** Ce macro-lot transforme le bootstrap réseau LLM précédemment couvert surtout par des codecs et tests unitaires en un contrat QEMU exécutable de bout en bout sur une NIC `ne2k_isa`. Il valide que la commande Ring 3 `ai-acquire example.com` fait partir et progresser la séquence DHCP, ARP, DNS A et SYN à travers le pilote PIO NE2000 réel.

> Cette validation utilise un pair Ethernet local déterministe. Elle ne démontre ni un bail obtenu sur un réseau tiers, ni une connexion TLS/HTTP vers OpenAI ou Ollama, ni une connectivité Internet publique.

## Objectif et périmètre

Le sandbox ne fournit pas `/dev/net/tun` et ne peut donc pas construire le TAP/bridge initialement envisagé. Le contrat utilise à la place le backend QEMU `-netdev socket`, relié à un serveur TCP local. Chaque trame Ethernet est encapsulée par un préfixe de longueur big-endian de 32 bits. Cette topologie reste entièrement isolée, ne demande aucun privilège réseau et permet de vérifier le chemin matériel du noyau sans dépendre d’un DHCP, DNS ou routeur externe.

| Élément | Valeur contrôlée | Preuve du contrat |
|---|---|---|
| NIC invitée | `ne2k_isa` sur backend socket QEMU | QEMU démarre avec `-device ne2k_isa,netdev=n0`. |
| Entropie TLS | `RDRAND` matériel QEMU disponible | `ai-runtime` doit afficher le diagnostic positif avant acquisition. |
| DHCP | DISCOVER, OFFER, REQUEST, ACK | Le pair compte une émission de chaque message et fournit un bail `10.32.0.15/24`, routeur et DNS `10.32.0.2`. |
| ARP | Résolution du prochain saut `10.32.0.2` | Le pair observe la requête ARP et renvoie une réponse Ethernet unicast. |
| DNS A | `example.com` | Le pair observe la requête et répond déterministiquement `203.0.113.20` (TEST-NET-3). |
| TCP | SYN sortant | Le pair constate le SYN vers l’adresse retournée par DNS. |
| Publication noyau | Bail et phase socket | `ai-runtime` doit confirmer un bail présent et `SYN_SENT (NE2000 pret)`. |

## Corrections apportées au pilote

Les tests QEMU ont révélé deux écarts impossibles à déduire d’un codec isolé. D’abord, `ne2k_tx_submit()` effaçait le bit d’interruption `RDC` après les écritures DMA distantes : l’événement d’achèvement était donc effacé avant l’attente, et le DHCP DISCOVER échouait avant même de sortir de la NIC. L’acquittement est désormais effectué **avant** la commande `REMOTE_WRITE`; la copie PIO est suivie de l’attente RDC puis de la commande de transmission.

Ensuite, le filtre d’adresse physique de la NIC ne recevait pas les réponses ARP unicast. La lecture de PROM lisait auparavant le port DATA sans initialiser l’adresse ni la taille du DMA distant. `ne2k_read_mac()` configure maintenant `RBCR=12`, `RSAR=0` et `REMOTE_READ`, puis récupère les six octets pairs de la PROM avant de programmer `PAR[0..5]`. Les passages `STOP` vers les pages 0 et 1 ajoutent explicitement le mode `NODMA`; la réception conserve ainsi le filtre strict `RCR=0x04` (broadcast DHCP et unicast correspondant à PAR), sans mode promiscue persistant.

| Chemin | Mesure de robustesse livrée |
|---|---|
| Initialisation | Ordre noyau `prepare → read_mac → configure_rings`, afin que PAR contienne une MAC valide. |
| TX PIO | Ordre RDC fixé et test unitaire de trace `RDC → REMOTE_WRITE → DATA → TRANSMIT`. |
| PROM/PAR | Lecture DMA ROM explicite, suivi d’un test de `RBCR/RSAR/REMOTE_READ`. |
| RX | Polling de l’anneau fondé sur `CURR/BNRY`, donc résilient à l’acquittement préalable de `PRX` par IRQ3. |
| Pollers | Pause bornée entre tentatives DHCP, DNS et ARP pour laisser QEMU publier les réponses asynchrones ; elle est nulle dans les fixtures sans tick noyau. |
| Diagnostics Ring 3 | Sous-codes d’acquisition distinguant l’entropie TLS, DISCOVER, OFFER, REQUEST et ACK, puis message explicite lorsque le bootstrap DNS/ARP/SYN ne peut pas commencer. |

Aucun de ces chemins n’introduit `malloc`, `calloc`, `realloc` ou `kmalloc`. Les buffers Ethernet, DHCP, ARP, DNS et TCP restent statiques ou fournis par l’appelant.

## Contrat exécutable

La cible publique est la suivante :

```bash
make qemu-ne2k-acquire
```

Elle construit le noyau et l’initrd, démarre le pair `tests/scripts/qemu_ne2k_controlled_peer.py`, puis lance `tests/scripts/test_qemu_ne2k_llm_acquire.py`. Le script saisit les commandes par le moniteur QEMU, ferme le processus et son socket Unix même en cas d’échec, et expose les compteurs protocolaires dans son message d’erreur. Aucun service, port ou fichier de configuration externe n’est requis.

| Vérification | Résultat de référence du macro-lot |
|---|---|
| `make -s test-all` | **484/484** tests réussis. |
| Test Unity NE2000 | **47/47** cas réussis, dont les régressions ROM DMA et ordre RDC/TX. |
| `make -s kernel-only` | Noyau i386 freestanding compilé. |
| `make -s qemu-ne2k-acquire` | Contrat QEMU réussi, avec compteurs DHCP, ARP, DNS et SYN non nuls. |
| Audit ciblé | Aucun appel d’allocation dynamique ajouté ; `git diff --check` réussi. |

## Limites et suite

Le pair contrôlé répond uniquement aux échanges nécessaires au bootstrap et ne simule pas de SYN-ACK, de handshake TLS, de certificat X.509, de HTTP ou de flux SSE. Il ne remplace pas une campagne matérielle avec une vraie carte NE2000, un bridge/TAP, un DHCP réel ni un fournisseur LLM. Le prochain macro-lot réseau doit donc étendre ce même pair isolé avec SYN-ACK puis un serveur TLS/HTTP contrôlé, avant toute revendication de connexion externe effective.

## Références internes

- [Contrat QEMU d’acquisition](../tests/scripts/test_qemu_ne2k_llm_acquire.py)
- [Pair Ethernet local contrôlé](../tests/scripts/qemu_ne2k_controlled_peer.py)
- [Pilote NE2000](../kernel/ne2k.c)
- [Bootstrap transactionnel DHCP/DNS/ARP/SYN précédent](aos1449_1460_socket_bootstrap.md)
- [État réel du prototype](ETAT_REEL.md)
- [Backlog technique](todo.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
