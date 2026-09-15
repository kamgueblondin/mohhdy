# AOS-2076…2080 — Contrat QEMU NE2000 : ServerHello et ACK TCP

**Statut : livré localement, en validation de non-régression.** Ce macro-lot étend le pair Ethernet QEMU isolé au premier message de serveur TLS. Après le ClientHello déjà observé, le pair renvoie un record TLS 1.2 `ServerHello` minimal ; le noyau l’accepte, maintient la session dans `TLS_STARTED` et émet l’ACK TCP correspondant.

> Cette étape prouve la réception et le décodage d’un premier record serveur, ainsi que la progression TCP réelle dans QEMU. Elle ne valide pas l’identité du pair, un certificat, une clé éphémère, le chiffrement applicatif ou HTTPS.

## Vol TLS contrôlé

Le pair local emploie toujours le backend QEMU `-netdev socket`, sans TAP, bridge ni accès réseau hôte. Dès qu’il reçoit le ClientHello à destination de `203.0.113.20:443`, il renvoie un segment TCP `ACK|PSH` qui porte un record TLS de type handshake et version `03 03`. La charge handshake est un `ServerHello` de 42 octets : TLS 1.2, aléa serveur déterministe, identifiant de session vide, suite `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` (`0xc02b`) et compression nulle.

| Étape | Paquet contrôlé | Résultat exigé |
|---|---|---|
| Client | ClientHello TLS après SYN-ACK | Le pair incrémente `client_hello`. |
| Pair | `ACK|PSH` TCP contenant un ServerHello TLS 1.2 | Le pair incrémente `server_hello`. |
| Noyau | `ai-tls-poll` lit et valide le message de handshake | La commande publie une progression non erronée. |
| Client | ACK TCP sans payload vers le pair | Le pair incrémente `server_hello_ack`. |
| Session | Phase publique | `ai-runtime` reste `TLS_STARTED`, car certificat et fin de handshake n’ont pas été reçus. |

Le pair parse désormais la longueur IPv4 déclarée et ignore le padding Ethernet éventuel. Ce point est nécessaire pour distinguer un ACK TCP sans payload des octets de remplissage ajoutés aux trames courtes par la NIC ou QEMU.

## Contrat public

```bash
make qemu-ne2k-acquire
```

Le contrat saisi successivement `ai-acquire example.com`, puis deux appels à `ai-tls-poll`. Il exige les compteurs DHCP, ARP, DNS, SYN, SYN-ACK, ClientHello, ServerHello et ACK tous non nuls. En cas de défaut, les compteurs du pair sont affichés avec l’erreur afin de localiser le premier maillon manquant.

## Limites explicites

Le ServerHello minimal n’est pas accompagné d’un certificat ni d’une signature. Il ne peut donc pas satisfaire la validation X.509 intégrée au noyau, et aucun vol X25519, ChangeCipherSpec, Finished, record chiffré, HTTP ou SSE ne doit être déduit de ce test. Les prochaines étapes exigent un jeu de certificats locaux, une clé privée de test explicitement isolée et un serveur TLS déterministe capable de construire les messages authentifiés restants.

Aucun appel à `malloc`, `calloc`, `realloc` ou `kmalloc` n’est ajouté. Les buffers de record, TCP, Ethernet et les compteurs du pair restent bornés et statiques ou appartenant à l’appelant.

## Références internes

- [Pair Ethernet et TLS contrôlé](../tests/scripts/qemu_ne2k_controlled_peer.py)
- [Contrat QEMU d’acquisition](../tests/scripts/test_qemu_ne2k_llm_acquire.py)
- [Poller TLS NE2000](../kernel/ne2k.c)
- [Codec TLS et vecteurs unitaires](../kernel/net_tls_record.c)
- [SYN-ACK et ClientHello précédents](aos2069_2075_ne2k_qemu_synack_clienthello.md)
- [État réel](ETAT_REEL.md)
- [Backlog](todo.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
