# Guest-guest : chat metier AES-GCM bilaterale

**Statut : etape mesurable.** Suite apres `make qemu-ne2k-guest-tls-appdata`.
Deux invites QEMU TCG partagent le hub `127.0.0.1` avec `proxy_peer_syn_ack=False`.
Apres `TLS_COMPLETE` cote A et Finished cote B, A et B echangent au moins deux
messages metier distincts dans chaque sens (`METIER-A0`/`METIER-A1` puis
`METIER-B0`/`METIER-B1`) sur la session TLS AES-GCM existante. Le hub compte
`guest_app_data_c2s` et `guest_app_data_s2c` (records 0x17) sans fake crypto.

Sans TAP, sans hote Internet, sans secret et sans OpenAI.

## Prealable

- Ping/pong AES-GCM : `make qemu-ne2k-guest-tls-appdata`
- Serveur TLS : `make qemu-ne2k-guest-tls-server`

## Contrat

`make qemu-ne2k-guest-tls-chat-metier` :

1. Meme topologie et handshake que `qemu-ne2k-guest-tls-server`
2. A : `ai-app-chat 0` puis `ai-app-chat 1` (`guest_app_data_c2s >= 2`,
   marqueurs serial `app chat emis A0` / `A1`)
3. B : `ai-peer-tls-poll` repond `METIER-B0` / `METIER-B1`
   (`guest_app_data_s2c >= 2`, marqueurs `app chat echo B0` / `B1`)
4. A : `ai-app-recv` ouvre chaque reponse (`app chat recu B0` / `B1`)
5. `guest_app_data >= 4`, `cross_syn_ack == 0`, deux baux, les deux invites
   vivants

Injection PS/2 mutexee hote. Hors `make ci` et `make integration-qemu`.

## Commandes shell

| Commande | Effet |
|---|---|
| `ai-app-chat 0\|1` | Emmet un record AES-GCM metier distinct sur la session client |
| `ai-app-recv` | Ouvre la prochaine reponse metier AES-GCM (B0/B1) |
| `ai-peer-tls-poll` | Apres Finished, ouvre l'app data et repond pong ou B0/B1 |

## Limites

- Chat metier bilaterale multi-messages AES-GCM mesure dans chaque sens est vert.
- Pas de framing applicatif riche, POST/SSE peer, ni reseau public / OpenAI.
- Extraction pilote ATA/FAT derriere droits (tranche 4) et latence GGUF/KVM
  (tranche 3) restent ouvertes.
