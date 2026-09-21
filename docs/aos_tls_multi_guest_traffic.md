# TLS multi-invite sur topologie Ethernet partagee

**Statut : harness livre.** Suite de la tranche 2 apres
`make qemu-ne2k-shared-topology`. Deux invites QEMU TCG simultanés partagent le
hub `127.0.0.1` avec baux DHCP distincts (`10.32.0.15` / `10.32.0.16`) et menent
chacun un handshake TLS 1.2 local jusqu'a `TLS_COMPLETE`, sans TAP, sans hote
Internet, sans secret et sans OpenAI.

## Prealable

- Garde 2 : `make qemu-ps2-dual`
- Topologie : `make qemu-ne2k-shared-topology`

## Contrat

`make qemu-ne2k-tls-multi-guest` :

1. Demarre `SharedEthernetHub(full_tls=True)` (flood L2 + DHCP dual + TLS/HTTP)
2. Branche deux `ne2k_isa` (MAC distinctes) en `connect=127.0.0.1:PORT`
3. Invite A : `ai-runtime` / `ai-acquire` / `ai-tls-poll` jusqu'a `TLS_COMPLETE`
4. Invite B reste vivant (`net-status`) puis rejoue le meme cycle TLS
5. Exige deux baux DHCP et `TLS_COMPLETE` dans les deux journaux

Injection PS/2 mutexee hote (meme politique que Garde 2 / topologie). Hors
`make ci` et `make integration-qemu` (budget). Le smoke sequentiel
`make qemu-ne2k-tls-multipair` reste le garde-fou CI multi-pairs.

## Limites

- Les cycles TLS sont enchaines sous mutex PS/2 ; les deux VMs restent allumees
- Pas de conversation applicative guest↔guest (ARP croise / flux metier)
- Pas de reseau public ni OpenAI
