# AOS-1401 à AOS-1412 — ClientHello TLS actif sur socket

## Objectif

Ce macro-lot retire la première étape active TLS du chemin qui exigeait une connexion TCP privée. Après un SYN-ACK, l’appelant fournit seulement un identifiant de socket, les buffers TLS caller-owned et le contexte TLS déjà initialisé. Le ClientHello est construit, suivi dans le transcript, emballé dans le segment TCP du socket, puis émis par le pont NE2000.

## Contrat livré

| Primitive | Responsabilité | Rollback |
|---|---|---|
| `net_socket_send_limit` | Émet un payload TCP avec budget de retransmission caller-owned. | Le comportement historique de `net_socket_send` reste fixé à 3. |
| `ne2k_socket_tls_start` | Construit et transmet un ClientHello sur un socket `ESTABLISHED`. | Restaure le TCP du socket si le TX NE2000 échoue. |
| `ne2k_socket_tls_accept_syn_ack_start` | Accepte le SYN-ACK, puis lance ClientHello dans une transaction unique. | Restaure socket et contexte TLS à la valeur antérieure si une étape échoue. |

Le wrapper respecte l’ordre TLS 1.2 existant : construction du record, parsing de contrôle, notification `ClientHello` dans la machine d’état, ajout du message de handshake au transcript, transmission et publication du contexte TLS. Le record et le segment TCP restent intégralement fournis par l’appelant.

> Le pilote NE2000 ne reçoit aucun pointeur vers `net_tcp_connection_t`. Le slot socket statique reste la seule autorité sur les numéros de séquence TCP. Aucune allocation dynamique n’est introduite.

## Validation

Un test Unity couvre le chemin complet `SYN-ACK → ESTABLISHED → ClientHello` : un SYN-ACK invalide laisse le socket en `SYN_SENT` et le TLS à l’état `IDLE`, tandis qu’un SYN-ACK valide publie l’état `CLIENT_HELLO_SENT`, avance la séquence TCP de la longueur transmise et place un record TLS Handshake dans la trame NE2000. La compilation i386 est valide ; la suite noyau passe à **36/36** et la suite complète à **438/438**.

## Limites restantes

La migration concerne le démarrage actif TLS. Le polling authentifié des messages serveur, la validation X.509, le flight X25519 et le post-flight Finished sont encore fournis par l’orchestrateur historique à connexion TCP privée. Le prochain incrément doit les rattacher au socket grâce aux snapshots/restaurations déjà disponibles, puis connecter cette phase aux requêtes HTTP et SSE socket.

## Références

[1]: aos1393_1400_socket_active_syn.md "SYN actif socket vers NE2000"
[2]: aos1373_1384_llm_socket_http_sse.md "Réception HTTP et SSE LLM sur sockets TLS"

[1] [2]
