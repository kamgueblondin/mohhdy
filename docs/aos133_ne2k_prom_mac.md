# AOS-133 — Lecture caller-owned de la PROM MAC NE2000

Le lot AOS-133 ajoute `ne2k_read_mac`, une primitive de pilote qui lit douze octets depuis le port de données NE2000 et extrait les six octets pairs de la PROM, conformément au format historique de la carte. La MAC est copiée dans `ne2k_device_t`, puis marquée valide uniquement si elle n’est pas nulle et n’est pas multicast. Aucun pointeur vers un buffer externe n’est conservé et aucune allocation dynamique n’est effectuée.

La primitive invalide explicitement `mac_valid` lorsque la PROM expose une valeur nulle ou multicast. Cette règle évite de publier une adresse partiellement lue comme identité réseau. Les callbacks d’E/S restent injectables afin que les tests Unity n’accèdent jamais aux ports physiques.

## Portée et limites

| Élément | État AOS-133 |
|---|---|
| Lecture PROM via callback `inb` | Implémentée et testée sur faux périphérique. |
| Validation MAC unicast non nulle | Implémentée. |
| Conservation d’un buffer PROM | Interdite : tableau local borné de 12 octets uniquement. |
| Lecture PROM au boot réel | Non raccordée dans ce lot ; nécessite une validation QEMU avec lecture distante. |
| Émission TX, DMA distant et IRQ | Non implémentés. |
| DHCP, DNS, TLS et HTTP/OpenAI effectifs | Non implémentés sur le transport matériel. |

La suite Unity reste à **292 tests verts** après l’ajout du scénario PROM. Cette étape prépare l’identité Ethernet nécessaire au futur raccordement de la file TX/RX, sans surestimer l’état de la pile réseau.
