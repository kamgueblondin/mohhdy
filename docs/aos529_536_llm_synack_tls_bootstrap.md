# AOS-529 à AOS-536 — SYN-ACK automatique et démarrage TLS LLM

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** SYN-ACK TCP, ClientHello TLS 1.2, NE2000, transition LLM de préconnexion

## Objectif

Ce macro-lot ferme la jonction entre la préconnexion DNS/ARP/SYN et le handshake TLS. Après un SYN émis par le bootstrap précédent, le pilote peut désormais recevoir un SYN-ACK, valider les numéros de ports et d’acquittement, établir la connexion TCP et envoyer immédiatement le ClientHello TLS déjà disponible.

> Le handshake TCP emploie un échange de trois messages autour du drapeau SYN [1]. Le premier segment TLS applicatif, ici le ClientHello, porte l’acquittement final de manière implicite dans le segment TCP avec données ; aucun ACK pur redondant n’est émis.

## API ajoutée

| API | Rôle |
|---|---|
| `ne2k_tls_client_accept_syn_ack_start` | Accepte une vue SYN-ACK déjà extraite, puis construit et émet le ClientHello de manière transactionnelle. |
| `ne2k_llm_syn_ack_tls_start` | Poll NE2000, extrait la vue TCP et délègue automatiquement à la transition SYN-ACK→ClientHello. |

## Transition de state machine

| État avant | Événement | État après succès | Publication |
|---|---|---|---|
| `SYN_SENT`, TLS `IDLE` | SYN-ACK exact | TCP `ESTABLISHED`, TLS `CLIENT_HELLO_SENT` | Connexion, client TLS, transcript et séquence TCP. |
| `SYN_SENT`, TLS `IDLE` | SYN-ACK invalide | Inchangé | Aucune. |
| TCP établi, TLS `IDLE` | Échec de construction ou TX ClientHello | Inchangé | Aucune. |
| RX sans trame | Polling NE2000 | Inchangé | Retour `1`, aucune émission. |

La primitive de vue travaille sur des copies locales de `net_tcp_connection_t` et `ne2k_tls_client_t`. Elle valide le SYN-ACK sur les copies, appelle ensuite `ne2k_tls_client_start`, puis publie les deux contextes uniquement si l’envoi et le commit du ClientHello réussissent.

## Contrat mémoire et rollback

Aucune allocation dynamique n’est introduite. Les buffers RX, TX et ClientHello, le cache ARP, la connexion TCP et le client TLS restent tous fournis et possédés par l’appelant. Les capacités des buffers ne sont ni remplacées ni conservées dans un état global.

Le ClientHello utilise le chemin TLS existant, qui ajoute le handshake au transcript, suit le pending TCP et commit la séquence après transmission. En cas d’échec du SYN-ACK ou du démarrage TLS, les copies locales sont abandonnées : la connexion et le client exposés à l’appelant restent strictement dans leur état antérieur.

## Tests et validation locale

Le test NE2000 ajouté couvre un SYN-ACK dont l’acquittement est erroné, puis un SYN-ACK valide. Il vérifie que le premier cas laisse TCP en `SYN_SENT` et TLS en `IDLE`, tandis que le second passe TCP à `ESTABLISHED`, TLS à `CLIENT_HELLO_SENT` et avance la séquence selon la taille exacte du ClientHello. Le polling vide est également vérifié comme non bloquant.

| Vérification | Résultat |
|---|---|
| Suite noyau | **32/32** exécutables réussis. |
| Suite complète | **373/373** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi au second lancement ; le premier a subi une absence transitoire de détection NIC QEMU. |

## Limites connues

Le lot ne fusionne pas encore le bootstrap DNS/ARP/SYN, l’attente du SYN-ACK et le handshake TLS complet dans une machine d’état unique. Il ne fournit ni temporisation, ni retransmission SYN, ni backoff, ni sélection d’adresse DNS, ni timeout matériel. L’authentification X.509, les dates RTC, l’échange X25519, le Finished et l’HTTP restent délégués aux étapes TLS existantes appelées ultérieurement. Le défaut de détection NIC intermittent observé dans QEMU est signalé mais non attribué à la logique ajoutée, car la relance isolée réussit.

## Références

[1] [RFC 793 — Transmission Control Protocol](https://datatracker.ietf.org/doc/html/rfc793)  
[2] [AOS-521 à AOS-528 — bootstrap DNS, ARP et SYN](aos521_528_llm_dns_syn_bootstrap.md)
