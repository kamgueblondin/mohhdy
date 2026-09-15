# AOS-177 — Extensions du ServerHello TLS 1.2

AOS-177 complète le parseur minimal de `ServerHello` en exposant le bloc d’extensions TLS sous forme de vue **caller-owned**. Après le random de 32 octets, le session ID, la suite cryptographique et la méthode de compression, le parseur accepte soit la fin exacte du message, soit un champ `extensions_length` de deux octets suivi d’un bloc de longueur exactement égale.

Le pointeur `extensions` désigne directement la mémoire d’entrée fournie par l’appelant. Le codec ne copie pas les extensions, ne conserve pas de buffer interne et n’appelle aucune primitive d’allocation dynamique. Les longueurs sont contrôlées avant chaque accès afin de rejeter les extensions tronquées, les longueurs excédentaires et les messages incohérents.

Le test unitaire couvre le ServerHello sans extension, le bloc d’extensions vide et une longueur annoncée incohérente. Le dépassement de pile introduit lors de l’écriture de ce dernier cas a été corrigé dans le test en utilisant un buffer de 44 octets et des longueurs d’entrée explicites.

| Élément | Statut |
|---|---|
| Vue caller-owned du bloc d’extensions | Implémentée |
| ServerHello sans extensions | Validé |
| Extensions vides (`length = 0`) | Validé |
| Contrôles de bornes et de longueur | Validés |
| Décodage sémantique des types d’extensions | Non implémenté |
| Négociation cryptographique et dérivation de clés | Non implémentées |
| X.509, chiffrement des records et Finished | Non implémentés |
| HTTP et appels LLM sécurisés de bout en bout | Non fonctionnels |

La validation finale AOS-177 produit **312 tests Unity réussis sur 312**, avec compilation i386, smoke `qemu-ai-provider` et smoke `qemu-ne2k-status` réussis. Cette validation confirme le framing et l’automate minimal, mais ne constitue pas une implémentation TLS 1.2 sécurisée tant que la cryptographie, les certificats, la dérivation de clés et le chiffrement ne sont pas livrés.
