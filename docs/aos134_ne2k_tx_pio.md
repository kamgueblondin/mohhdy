# AOS-134 — Émission PIO NE2000 caller-owned

Le lot AOS-134 ajoute `ne2k_tx_submit`, une primitive d’émission qui configure une écriture distante NE2000 en mode octet, copie une trame fournie par l’appelant dans la page TX `0x40`, attend l’indication `RDC` avec une boucle bornée, puis programme `TPSR` et `TBCR` avant de déclencher `TXP`.

Les limites sont explicites. Les trames de moins de 60 octets sont complétées localement par des zéros jusqu’à la taille Ethernet minimale ; les trames supérieures à 1514 octets sont rejetées. Le pilote ne conserve pas le pointeur de l’appelant, n’alloue aucune mémoire et n’attend pas indéfiniment une interruption matérielle. Le chemin utilise les callbacks `inb/outb`, donc les tests Unity restent indépendants de l’hôte.

| Élément | État AOS-134 |
|---|---|
| Écriture distante PIO bornée | Implémentée. |
| Padding Ethernet minimal | Implémenté à 60 octets. |
| Limite MTU | Rejet au-delà de 1514 octets. |
| Attente RDC | Polling borné à 65535 itérations. |
| IRQ réseau | Non raccordée ; le polling ne remplace pas une gestion d’interruption complète. |
| Validation QEMU d’une trame reçue | Non déclarée dans ce lot. |
| DHCP/DNS/TLS/HTTP/OpenAI effectifs | Non implémentés sur le transport matériel. |

La suite Unity reste à **292 tests verts**. Cette primitive prépare l’émission Ethernet nécessaire à ARP et DHCP, mais ne constitue pas encore un transport IP fonctionnel de bout en bout.
