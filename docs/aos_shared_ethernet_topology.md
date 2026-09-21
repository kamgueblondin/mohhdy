# Topologie Ethernet locale partagée (tranche 2)

**Statut : harness livré.** Deux invités QEMU TCG simultanés partagent un segment
Ethernet local via un hub stream `127.0.0.1` (backend `-netdev socket`), sans TAP,
sans hôte Internet, sans secret et sans OpenAI.

## Préalable

Garde 2 — `make qemu-ps2-dual` : injection PS/2 fiable avec deux QEMU vivants et
un verrou hôte sur `sendkey`.

## Contrat

`make qemu-ne2k-shared-topology` :

1. Démarre `SharedEthernetHub` (inondation L2 + DHCP/ARP/DNS local)
2. Branche deux `ne2k_isa` (MAC distinctes) en `connect=127.0.0.1:PORT`
3. Injecte `net-status json` sur A et B (mutex hôte) → `nic=detected`
4. Injecte `ai-acquire example.com` sur A jusqu'au marqueur DHCP/DNS/SYN
5. Exige un DHCP Discover observé par le hub et la survie de B sur le segment
6. Rejoue `net-status json` sur B

Hors `make ci` et `make integration-qemu` (budget). Le smoke TLS multi-pairs
séquentiel `make qemu-ne2k-tls-multipair` reste inchangé.

## Limites

- Un seul bail déterministe (`10.32.0.15`) : un seul invité émet `ai-acquire`
- Pas de TLS multi-invités simultané, pas de conversation applicative guest↔guest
- Pas de réseau public
