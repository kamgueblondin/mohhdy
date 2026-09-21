# Guest-guest : application data AES-GCM (ping/pong)

**Statut : etape mesurable.** Suite apres `make qemu-ne2k-guest-tls-server`.
Deux invites QEMU TCG partagent le hub `127.0.0.1` avec `proxy_peer_syn_ack=False`.
Apres `TLS_COMPLETE` cote A et Finished cote B, A emet un record applicatif
AES-GCM (`ai-app-ping`, plaintext `ping`). B l'ouvre avec sa session serveur
et repond (`ai-peer-tls-poll` -> `app echo emis`, plaintext `pong`). Le hub
compte `guest_app_data_c2s` et `guest_app_data_s2c` (records 0x17) sans fake
crypto.

Sans TAP, sans hote Internet, sans secret et sans OpenAI.

## Prealable

- Serveur TLS : `make qemu-ne2k-guest-tls-server`

## Contrat

`make qemu-ne2k-guest-tls-appdata` :

1. Meme topologie et handshake que `qemu-ne2k-guest-tls-server`
2. A : `ai-app-ping` apres `TLS_COMPLETE` (`guest_app_data_c2s >= 1`, marqueur
   serial `app ping emis`)
3. B : `ai-peer-tls-poll` jusqu a echo (`guest_app_data_s2c >= 1`, marqueur
   serial `app echo emis`)
4. `guest_app_data >= 2`, `cross_syn_ack == 0`, deux baux, les deux invites
   vivants

Injection PS/2 mutexee hote. Hors `make ci` et `make integration-qemu`.

## Commandes shell

| Commande | Effet |
|---|---|
| `ai-app-ping` | Emmet un record AES-GCM applicatif `ping` sur la session client |
| `ai-peer-tls-poll` | Apres Finished, ouvre l'app data et repond `pong` |

## Limites

- Ping/pong AES-GCM mesure dans chaque sens est vert.
- Chat metier bilaterale (messages utiles, framing applicatif) reste ouvert.
- Pas de reseau public ni OpenAI.
