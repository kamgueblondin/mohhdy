# Guest-guest : ecoute B et SYN-ACK invite (pas de proxy hub)

**Statut : premier pas mesurable.** Suite apres `make qemu-ne2k-guest-app-traffic`.
Deux invites QEMU TCG partagent le hub `127.0.0.1`. L invite B prend un bail
DHCP, ouvre une ecoute TCP passive (`ai-peer-listen`), puis accepte le SYN de A
(`ai-peer-accept`) et emet le SYN-ACK depuis le guest. Le hub ne proxy plus le
SYN-ACK pair (`proxy_peer_syn_ack=False`) ; il compte `guest_syn_ack`.

Sans TAP, sans hote Internet, sans secret et sans OpenAI. Pas encore de TLS
guest-guest ni de conversation metier chiffree.

## Prealable

- Guest-app : `make qemu-ne2k-guest-app-traffic`
- TLS multi : `make qemu-ne2k-tls-multi-guest`

## Contrat

`make qemu-ne2k-guest-tls-peer` :

1. `SharedEthernetHub(respond=True, proxy_peer_syn_ack=False)`
2. Deux `ne2k_isa` (MAC distinctes) en `connect=127.0.0.1:PORT`
3. Invite B : `ai-acquire example.com` (bail), puis `ai-peer-listen`
4. Invite A : `ai-acquire peer.local` (DNS/ARP/SYN vers IP de B)
5. Invite B : `ai-peer-accept` → `SYN-ACK guest emis`
6. Exige `peer_dns`, `cross_arp` + `cross_arp_reply`, `cross_syn`,
   `guest_syn_ack >= 1`, `cross_syn_ack == 0`, deux baux, B vivant

Injection PS/2 mutexee hote. Hors `make ci` et `make integration-qemu`.

## Commandes shell

| Commande | Effet |
|---|---|
| `ai-peer-listen [port]` | `SYS_PEER_LISTEN` : socket LISTEN (defaut 443), bail DHCP requis |
| `ai-peer-accept [attempts]` | `SYS_PEER_ACCEPT` : poll SYN, emission SYN-ACK NE2000, retour 1 si SYN_RECEIVED |

## Limites

- Pas d ACK final cote A ni ESTABLISHED mutuel dans ce lot (A reste SYN_SENT)
- Pas de TLS 1.2 guest↔guest ni POST/SSE metier
- Pas de reseau public ni OpenAI
- Suite logique : ACK final + handshake TLS peer, puis conversation chiffree
