# AOS-2069…2075 — Contrat QEMU NE2000 : SYN-ACK routé et démarrage ClientHello

**Statut : livré localement, en validation de non-régression.** Ce macro-lot prolonge le bootstrap NE2000 contrôlé par la réception d’un `SYN-ACK` TCP valide et l’émission observable du premier record TLS `ClientHello`. Il vérifie donc le passage de la session noyau de `SYN_SENT` à `TLS_STARTED` depuis la commande Ring 3 `ai-tls-poll`, sans dépendance à Internet, à un fournisseur LLM ou à un certificat externe.

> Le test s’arrête volontairement après le `ClientHello`. Il ne prouve ni l’authentification X.509, ni l’échange X25519, ni l’établissement TLS complet, ni HTTP/SSE ou une requête OpenAI/Ollama réelle.

## Problème résolu : prochain saut après le SYN

Le DHCP simulé attribue le sous-réseau `10.32.0.0/24` et la passerelle `10.32.0.2`, tandis que le DNS contrôlé retourne `203.0.113.20` pour `example.com`. Le SYN initial était correctement émis vers la MAC du routeur grâce à `ne2k_tcp_syn_via()`. Les étapes TLS suivantes recherchaient toutefois la MAC avec l’IP logique distante, ce qui déclenchait une résolution ARP erronée vers un hôte hors sous-réseau et empêchait l’envoi du ClientHello.

La correction conserve donc, après le SYN routé, la MAC du **prochain saut** résolu sous la clé de destination TCP logique dans le cache de voisinage utilisé par les phases TLS/HTTP. Le paquet garde toujours `203.0.113.20` comme destination IPv4 et TCP ; seule la destination Ethernet reste la MAC de la passerelle `10.32.0.2`. Aucun buffer dynamique n’est introduit.

| Couche | Avant | Après |
|---|---|---|
| DNS | `example.com → 203.0.113.20` | Inchangé. |
| SYN | IPv4 distante, Ethernet routeur | Inchangé. |
| ClientHello | Recherche ARP de l’IP distante et échec | IPv4 distante, MAC de prochain saut mémorisée, émission réussie. |
| ABI `SYS_LLM_POLL_TLS` | Retour positif de longueur interprété comme attente | Retour `0` lorsque la phase `TLS_STARTED` est effectivement publiée. |

## Contrat QEMU exécuté

La cible existante reste unique afin de préserver un scénario de bout en bout reproductible :

```bash
make qemu-ne2k-acquire
```

Le pair `tests/scripts/qemu_ne2k_controlled_peer.py` utilise le backend QEMU `-netdev socket` et ne laisse sortir aucune trame sur un réseau externe. Après DHCP, ARP, DNS et SYN, il renvoie un `SYN-ACK` IPv4/TCP checksumé depuis `203.0.113.20:443` vers `10.32.0.15:49152`, avec un acquittement correspondant à la séquence du SYN du noyau. Il compte ensuite un record TLS de type handshake dont les trois premiers octets sont `16 03 03`, signature attendue du ClientHello TLS 1.2 produit par le noyau.

| Événement contrôlé | Attente du contrat |
|---|---|
| DHCP | `DISCOVER`, `OFFER`, `REQUEST` et `ACK` non nuls. |
| Réseau local | ARP de la passerelle et DNS A non nuls. |
| TCP | SYN sortant et `SYN-ACK` entrant non nuls. |
| TLS initial | ClientHello observé par le pair non nul. |
| Shell | `ai-tls-poll` publie la progression et `ai-runtime` affiche `TLS_STARTED (NE2000 pret)`. |

Le script conserve les compteurs du pair dans tout message d’échec. Ainsi, une divergence permet de distinguer l’absence de SYN, l’absence de `SYN-ACK`, un rejet du paquet TCP ou une absence de ClientHello sans inspection manuelle du trafic.

## Non-régression et limites

La correction est transactionnelle : si le voisin du prochain saut ne peut pas être récupéré ou inscrit, le bootstrap ne publie ni IP distante ni connexion TCP. Le `ClientHello` garde les garde-fous existants de RDRAND, de buffers TLS statiques, d’état socket et de capacité de transmission.

| Élément volontairement hors périmètre | Suite nécessaire |
|---|---|
| Fragments `ServerHello`, certificat et `ServerFinished` | Ajouter un serveur TLS contrôlé et des vecteurs de certificat locaux. |
| Vérification X.509 et X25519 sous QEMU réel | Injecter les records TLS valides, puis observer les ACK et le vol de clé du client. |
| HTTP, SSE et fournisseur LLM | Émuler une réponse HTTPS locale une fois `TLS_COMPLETE` atteint. |
| Réseau physique ou Internet | Campagne distincte avec bridge/TAP ou matériel, hors sandbox isolé. |

## Références internes

- [Pair Ethernet contrôlé](../tests/scripts/qemu_ne2k_controlled_peer.py)
- [Contrat QEMU d’acquisition et TLS](../tests/scripts/test_qemu_ne2k_llm_acquire.py)
- [Pilote NE2000](../kernel/ne2k.c)
- [Orchestrateur noyau LLM](../kernel/kernel.c)
- [Bootstrap DHCP/DNS/ARP/SYN précédent](aos2061_2068_ne2k_qemu_controlled_bootstrap.md)
- [État réel du prototype](ETAT_REEL.md)
- [Backlog technique](todo.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
