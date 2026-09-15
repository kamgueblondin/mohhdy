# AOS-197 à AOS-200 — Expansion des clés de trafic AES-128-GCM TLS 1.2

Ce macro-lot transforme un master secret TLS 1.2 déjà calculé en un bloc de clés de trafic pour la suite AES-128-GCM. Il conserve le modèle sans allocation dynamique : le bloc de sortie, le workspace du PRF et la structure de vues sont fournis par l’appelant.

La fonction `net_tls_derive_aes128_gcm_key_block` applique le PRF HMAC-SHA256 avec le label `key expansion` et l’ordre de seed `server_random || client_random`. Le bloc de 40 octets est découpé en deux clés de 16 octets et deux IV fixes de 4 octets.

| Plage du bloc | Vue publiée | Taille |
|---|---|---:|
| `0..15` | `client_write_key` | 16 octets |
| `16..31` | `server_write_key` | 16 octets |
| `32..35` | `client_fixed_iv` | 4 octets |
| `36..39` | `server_fixed_iv` | 4 octets |

Le test unitaire compare les 40 octets du résultat à un vecteur HMAC-SHA256 déterministe et vérifie les quatre pointeurs de vue ainsi que le rejet d’un buffer de sortie inférieur à 40 octets.

| Validation | Résultat |
|---|---|
| Vecteur key expansion AES-128-GCM | Validé |
| Suite Unity complète | 325/325 tests verts |
| Allocation dynamique | Absente |
| AES-128-GCM, nonce explicite, tag et chiffrement | Non implémentés |
| ECDHE réel, X.509 et signature ServerKeyExchange | Non implémentés |
| HTTP et appels LLM TLS sécurisés | Non fonctionnels |

> Les clés de trafic sont dérivées mais ne sont pas encore utilisées pour protéger des records. Le code ne doit pas être considéré comme un transport TLS chiffré tant qu’AES-GCM, l’authentification des tags et le déchiffrement des records ne sont pas implémentés.
