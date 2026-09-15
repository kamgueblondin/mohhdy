# AOS-151 — Émission du premier ACK TCP via NE2000

AOS-151 raccorde l’état de connexion TCP caller-owned au chemin d’émission NE2000. `ne2k_tcp_ack` recherche la MAC distante dans le cache ARP fourni par l’appelant, construit une trame Ethernet IPv4/TCP de 40 octets de protocole, calcule le checksum TCP sur pseudo-en-tête IPv4, calcule le checksum IPv4 puis soumet la trame au contrôleur NE2000 en PIO.

La primitive exige un périphérique préparé, une MAC locale valide, une entrée ARP déjà résolue, une connexion `ESTABLISHED` et un buffer de trame appartenant à l’appelant. Aucun cache interne, aucune allocation et aucun état global TCP ne sont ajoutés.

| Élément | Statut réel |
|---|---|
| Recherche MAC distante via cache ARP | Implémentée. |
| Trame Ethernet IPv4/TCP ACK | Implémentée. |
| Checksum TCP pseudo-en-tête IPv4 | Implémenté. |
| Checksum IPv4 | Implémenté. |
| Transmission NE2000 PIO | Réutilise `ne2k_tx_submit`. |
| Retransmission, timer, contrôle de congestion | Non implémentés. |
| Données TCP et TLS | Lots suivants ; non fonctionnels de bout en bout. |

Le test unitaire NE2000 vérifie les adresses MAC, le protocole IPv4, les ports, les numéros de séquence/acquittement et le drapeau ACK.
