# Guest-guest ARP croise et flux applicatif (suite tranche 2)

**Statut : harness livre.** Suite apres `make qemu-ne2k-tls-multi-guest`. Deux
invites QEMU TCG simultanés partagent le hub `127.0.0.1` avec baux DHCP
distincts. L invite B acquiert `example.com` ; l invite A resout `peer.local`
vers l IP de B, emet un ARP croise puis un SYN TCP vers B:443. Le hub proxy la
reponse ARP (MAC du titulaire) et un SYN-ACK minimal (flux applicatif simple).
Sans TAP, sans hote Internet, sans secret et sans OpenAI.

## Prealable

- Garde 2 : `make qemu-ps2-dual`
- Topologie : `make qemu-ne2k-shared-topology`
- TLS multi : `make qemu-ne2k-tls-multi-guest`

## Contrat

`make qemu-ne2k-guest-app-traffic` :

1. Demarre `SharedEthernetHub(respond=True)` (flood L2 + DHCP dual + peer DNS/ARP)
2. Branche deux `ne2k_isa` (MAC distinctes) en `connect=127.0.0.1:PORT`
3. Invite B : `ai-runtime` / `ai-acquire example.com` (bail + DNS/ARP/SYN locaux)
4. Invite A : `ai-acquire peer.local` → DNS pair, ARP croise, SYN vers IP de B
5. Exige `peer_dns`, `cross_arp` + `cross_arp_reply`, `cross_syn` + `cross_syn_ack`,
   deux baux, et B encore vivant (`net-status`)

Injection PS/2 mutexee hote (meme politique que Garde 2 / topologie / TLS multi).
Hors `make ci` et `make integration-qemu` (budget).

## Note hub

Les trames emises par les invites restent inondees (segment L2 partage). Les
reponses de controle du hub (DHCP/DNS/ARP/TCP) restent unicast vers le client
source, pour eviter qu un second invite au meme xid DHCP ne consomme le bail du
pair.

## Limites

- Le SYN-ACK vers l IP du pair est un proxy hub (B n ecoute pas encore en Ring 3)
- Pas de TLS guest-guest ni de conversation metier chiffree
- Pas de reseau public ni OpenAI

## Suite

Premier pas sans proxy : `make qemu-ne2k-guest-tls-peer` /
[aos_guest_guest_tls_peer.md](aos_guest_guest_tls_peer.md) (B LISTEN + SYN-ACK guest) puis [aos_guest_guest_tls_chat.md](aos_guest_guest_tls_chat.md) (ACK final + ClientHello TLS).
