# Guest-guest : ACK final + ClientHello TLS 1.2 (pas de proxy hub)

**Statut : etape mesurable.** Suite apres `make qemu-ne2k-guest-tls-peer`.
Deux invites QEMU TCG partagent le hub `127.0.0.1` avec `proxy_peer_syn_ack=False`.
Apres SYN-ACK guest, A complete le handshake TCP (ACK final) et emet un
ClientHello TLS 1.2 construit par la crypto invite (`ai-tls-poll`). B consomme
l ACK et passe `ESTABLISHED` (`ai-peer-accept ... established`).

Sans TAP, sans hote Internet, sans secret et sans OpenAI. Pas encore de role
serveur TLS sur B, ni d echange applicatif chiffre bilaterale (POST/SSE metier).

## Prealable

- Peer SYN-ACK : `make qemu-ne2k-guest-tls-peer`
- Guest-app : `make qemu-ne2k-guest-app-traffic`

## Contrat

`make qemu-ne2k-guest-tls-chat` :

1. Meme topologie que `qemu-ne2k-guest-tls-peer`
2. B : `ai-peer-listen` puis `ai-peer-accept` → SYN-ACK guest
3. A : `ai-tls-poll` jusqu a ClientHello guest (`guest_client_hello`, phase `TLS_STARTED`)
4. B : `ai-peer-accept N established` → `ESTABLISHED`
5. Exige `peer_dns`, `cross_arp` + `cross_arp_reply`, `cross_syn`,
   `guest_syn_ack >= 1`, `guest_final_ack >= 1`, `guest_client_hello >= 1`,
   `cross_syn_ack == 0`, deux baux, les deux invites vivants

Injection PS/2 mutexee hote. Hors `make ci` et `make integration-qemu`.

## Commandes shell

| Commande | Effet |
|---|---|
| `ai-peer-listen [port]` | Ecoute TCP passive (defaut 443) |
| `ai-peer-accept [attempts]` | SYN-ACK guest (`SYN_RECEIVED`) |
| `ai-peer-accept [attempts] established` | Attend ACK final → `ESTABLISHED` |
| `ai-tls-poll` | Cote A : consomme SYN-ACK peer, ACK final + ClientHello TLS 1.2 |

## Limites

- Role serveur TLS + Finished : voir [aos_guest_guest_tls_server.md](aos_guest_guest_tls_server.md)
- Pas d application data AES-GCM bilaterale metier ni POST/SSE peer dans ce lot
