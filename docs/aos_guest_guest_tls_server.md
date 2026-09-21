# Guest-guest : role serveur TLS B (ServerHello..Finished)

**Statut : etape mesurable.** Suite apres `make qemu-ne2k-guest-tls-chat`.
Deux invites QEMU TCG partagent le hub `127.0.0.1` avec `proxy_peer_syn_ack=False`.
Apres ESTABLISHED + ClientHello guest, B joue le role serveur TLS 1.2 **in-tree**
(`ai-peer-tls-poll`) : ServerHello, Certificate (feuille de test), ServerKeyExchange
ECDHE_RSA/X25519 signe, ServerHelloDone, puis CCS + Finished AES-GCM. A complete
via `ai-tls-poll` jusqu a `TLS_COMPLETE`. Le hub compte `guest_server_hello` et
`guest_server_finished` sans fake crypto.

Sans TAP, sans hote Internet, sans secret et sans OpenAI.

## Prealable

- Chat TLS : `make qemu-ne2k-guest-tls-chat`

## Contrat

`make qemu-ne2k-guest-tls-server` :

1. Meme topologie que `qemu-ne2k-guest-tls-chat`
2. B : `ai-peer-listen` / `ai-peer-accept` → ESTABLISHED
3. A : `ai-tls-poll` → ClientHello (`guest_client_hello`)
4. B : `ai-peer-tls-poll` jusqu a Finished (`guest_server_hello`, `guest_server_finished`)
5. A : `ai-tls-poll` → `TLS_COMPLETE`
6. Exige aussi `peer_dns`, `cross_arp`, `guest_syn_ack`, `guest_final_ack`,
   `cross_syn_ack == 0`, deux baux, les deux invites vivants

Injection PS/2 mutexee hote. Hors `make ci` et `make integration-qemu`.

## Commandes shell

| Commande | Effet |
|---|---|
| `ai-peer-tls-poll` | Avance le role serveur TLS guest d un cran (SH / Cert / SKE / SHD / flight / CCS / Finished) |

## Limites

- Handshake TLS 1.2 complet (ServerHello..Finished, A TLS_COMPLETE) est vert.
- Compteur hub `guest_app_data` reste a 0 dans ce harness : pas encore de
  record applicatif AES-GCM mesure dans chaque sens (echo B pret si A emet).
- Pas de reseau public ni OpenAI.
- Suite logique : ping/pong AES-GCM puis chat metier sur la session etablie.
