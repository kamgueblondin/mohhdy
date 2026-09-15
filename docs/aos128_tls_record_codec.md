# AOS-128 — Codec TLS record caller-owned

Le lot AOS-128 ajoute le framing TLS record pour TLS 1.2: type de contenu, version `3.3`, longueur 16 bits et payload. La construction copie uniquement dans un buffer fourni par l’appelant; le parsing retourne une vue bornée sans allocation et rejette les versions incohérentes, les types nuls et les longueurs dépassant le paquet.

Cette primitive ne réalise pas de handshake, de dérivation de clés, de chiffrement, de validation X.509 ni de HTTP. Elle établit seulement un contrat de transport pour les futurs messages TLS, en complément de SHA-256 et HMAC-SHA-256.

Le test `test_net_tls_record.c` couvre la construction et le parsing d’un record Handshake ainsi que le rejet d’une version invalide. La validation locale est de **284 tests verts**, avec build i386 et initrd réussis.
