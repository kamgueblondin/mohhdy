# AOS-123 — Codec DNS A caller-owned

Le lot AOS-123 ajoute la construction bornée d’une requête DNS de type A et le parsing d’une réponse contenant un enregistrement A. Les labels sont encodés sans allocation dynamique et chaque longueur est contrôlée. Le parseur vérifie l’identifiant, le bit de réponse, le code RCODE, la section question et accepte les noms compressés dans les réponses avant de copier l’adresse IPv4 trouvée.

Le test `test_net_dns.c` couvre la requête `example.com`, l’identifiant DNS et une réponse A compressée vers `10.0.2.15`, ainsi que le rejet d’un identifiant inattendu. La validation locale est de **279 tests verts** et le build i386 réussit. Le codec ne transmet pas encore de datagramme et devra être raccordé à UDP/IPv4 et au résolveur configuré par DHCP.
