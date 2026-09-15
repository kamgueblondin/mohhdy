# AOS-189 à AOS-192 — PRF TLS 1.2, master secret et Finished caller-owned

Ce macro-lot introduit les primitives de dérivation TLS 1.2 nécessaires après l’établissement futur d’un premaster secret. Il repose sur la primitive HMAC-SHA256 déjà disponible et exige que tous les buffers de sortie et de travail soient fournis par l’appelant. Aucun état global, aucune allocation dynamique et aucune zone de mémoire non bornée ne sont introduits.

`net_tls_prf_sha256` implémente `P_hash` avec HMAC-SHA256. Il forme explicitement `label || seed_a || seed_b`, itère `A(i)` et produit exactement le nombre d’octets demandé. Le workspace doit contenir les graines, `A(i)` et le message HMAC concaténé; toute capacité insuffisante est rejetée avant l’écriture de sortie.

| Lot | Primitive | Entrées caller-owned | Sortie |
|---|---|---|---|
| AOS-189 | `P_hash` HMAC-SHA256 | Secret, label, graines, workspace | PRF de longueur variable |
| AOS-190 | Hash SHA-256 du transcript | Transcript borné | Digest de 32 octets |
| AOS-191 | Master secret TLS 1.2 | Premaster, random client et serveur, workspace | 48 octets |
| AOS-192 | `verify_data` client Finished | Master secret, transcript, workspace | 12 octets |

Les tests utilisent des vecteurs déterministes générés indépendamment pour le PRF court, le master secret, le hash du transcript et le `verify_data` Finished. Les dépendances SHA-256 ont été ajoutées aux deux couches du harness Unity, y compris les binaires TCP et NE2000 qui transitent maintenant par le codec TLS.

| Validation | Résultat |
|---|---|
| Vecteurs PRF, master secret et Finished | Validés |
| Suite Unity complète | 322/322 tests verts |
| Allocation dynamique | Absente |
| ECDHE réel et génération du premaster secret | Non implémentés |
| Validation X.509 et signature ServerKeyExchange | Non implémentées |
| Dérivation des clés de trafic et protection AEAD | Non implémentées |
| Finished serveur et HTTP sécurisé | Non implémentés |

> Le master secret et `verify_data` sont calculés seulement à partir de secrets fournis par l’appelant. Le macro-lot ne crée pas encore un secret ECDHE, ne valide aucun certificat et ne chiffre aucun record : il ne fournit donc pas une connexion TLS sécurisée de bout en bout.
