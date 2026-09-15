# AOS-136 — Polling RX PIO NE2000 caller-owned

Le lot AOS-136 ajoute `ne2k_rx_poll`, une primitive de réception par polling PIO. Elle vérifie `PRX`, calcule la page suivante à partir de `BNRY`, lit l’en-tête de quatre octets par remote-read, valide le statut, la page suivante et la longueur Ethernet, puis lit uniquement la charge utile dans le buffer fourni par l’appelant. Après succès, `BNRY` est avancé sans allocation ni pointeur conservé.

La primitive remet la longueur de sortie à zéro avant tout traitement. Elle retourne `1` lorsqu’aucun paquet n’est disponible, rejette une charge utile qui dépasse la capacité fournie et borne la longueur maximale à 1514 octets. Le service IRQ reste séparé : AOS-136 fournit le polling PIO, mais ne prétend pas encore exposer une réception Ethernet observable de bout en bout dans QEMU.

| Élément | État AOS-136 |
|---|---|
| Détection `PRX` et lecture remote-read | Implémentée. |
| Validation header/page/longueur | Implémentée. |
| Copie vers buffer caller-owned | Implémentée sans allocation. |
| Avancement BNRY | Implémenté après copie réussie. |
| Test Unity sans paquet | Ajouté ; longueur de sortie remise à zéro. |
| Test QEMU d’une trame RX réelle | Non déclaré dans ce lot. |
| ARP/IP/DHCP/DNS/TLS/HTTP/OpenAI | Toujours non raccordés au transport matériel. |

Le build i386 et les **292 tests Unity** restent la porte de non-régression du lot.
