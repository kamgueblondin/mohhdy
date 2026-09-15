# AOS-121 — Codec DHCP DISCOVER/OFFER

Le lot AOS-121 introduit un codec DHCP caller-owned pour préparer la configuration IPv4 sans secret intégré. `net_dhcp_build_discover` construit un message DISCOVER avec XID, adresse MAC, cookie magique et option de type. `net_dhcp_parse_offer` vérifie le type BOOTP, le XID attendu, le cookie, le type OFFER et la présence d’un identifiant serveur avant de copier l’adresse offerte et celle du serveur.

Les longueurs d’options sont contrôlées avant chaque lecture. Les fonctions n’allouent pas de mémoire et écrivent uniquement dans les structures ou buffers fournis par l’appelant. Le codec ne transporte encore aucun paquet UDP: il constitue la couche de représentation avant le raccordement au pilote NE2000 et à IPv4/UDP.

Le test `test_net_dhcp.c` couvre la construction, le cookie, le XID, le parsing d’un OFFER et le rejet d’un XID inattendu. La validation locale est de **277 tests verts**, avec build i386 et initrd réussis.
