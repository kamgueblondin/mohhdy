# AOS-179 — Parsing de ServerHelloDone TLS 1.2

AOS-179 ajoute le parsing strict du message `ServerHelloDone` TLS 1.2. Ce message est représenté par un en-tête de handshake de quatre octets : type `14` et longueur de corps nulle. Toute longueur non nulle, tout type différent ou toute entrée tronquée est rejeté avant publication.

L’automate caller-owned accepte `ServerHelloDone` uniquement après réception d’un `Certificate` valide et passe alors à `SERVER_HELLO_DONE_RECEIVED`. Aucun octet n’est copié et aucune allocation dynamique n’est réalisée. Cette transition prépare les étapes client suivantes du handshake, mais ne signifie pas encore que le serveur est authentifié ou que le canal est chiffré.

| Élément | Statut |
|---|---|
| Validation du message `ServerHelloDone` | Implémentée |
| Contrôle du type et de la longueur nulle | Validé |
| Transition après Certificate | Implémentée |
| Rejet des transitions hors ordre et répétées | Validé |
| Échange de clés et dérivation de secrets | Non implémentés |
| Validation X.509 et chaîne de confiance | Non implémentées |
| Chiffrement TLS et message Finished | Non implémentés |
| HTTP et appels LLM sécurisés de bout en bout | Non fonctionnels |

La validation ciblée AOS-179 produit **10 tests TLS verts**. La non-régression complète produit **316 tests verts sur 316**, avec build i386 réussi, smoke `qemu-ai-provider` réussi et smoke `qemu-ne2k-status` réussi. Le harnais IA réessaie une seule fois une commande lorsque l’injection clavier QEMU perd ponctuellement un caractère; il conserve un échec définitif si la sortie attendue n’apparaît pas.
