# AOS-777 à AOS-784 — Handshake TLS 1.2 ECDHE_ECDSA complet

**Statut : implémenté et validé.** Ce macro-lot couvre désormais, dans un même scénario de régression, le chemin TLS 1.2 complet depuis le certificat serveur P-256 et son `ServerKeyExchange` authentifié ECDSA jusqu’au flight client X25519, à la dérivation des clés AES-128-GCM et au déchiffrement du `Finished` émis par le client.

Selon RFC 8422, le serveur ECDHE_ECDSA transmet une clé ECDH éphémère dans `ServerKeyExchange`, signée avec la clé privée correspondant au certificat ECDSA ; le client produit ensuite une paire ECDH sur la même courbe et envoie sa clé publique dans `ClientKeyExchange` [1]. Le calcul du secret maître et la protection des records restent ceux de TLS 1.2 [2].

> **Garantie du lot.** Aucune clé X25519 éphémère, aucun secret maître et aucune session AES-GCM ne sont publiés tant que le certificat P-256, la signature ECDSA/SHA-256 et le `ServerHelloDone` ne sont pas acceptés dans l’ordre attendu.

## Parcours couvert

| Étape | État ou résultat contrôlé |
|---|---|
| `ServerHello` | Négociation de `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` (`0xC02B`). |
| `Certificate` | Parsing du certificat P-256 X.509 déjà couvert par AOS-753 à AOS-760. |
| `ServerKeyExchange` | Vérification ECDSA/SHA-256 de `client_random || server_random || ServerECDHParams`. |
| `ServerHelloDone` | Passage à `SERVER_HELLO_DONE_RECEIVED` ; transcript de 618 octets. |
| Flight client | `ClientKeyExchange`, `ChangeCipherSpec`, puis `Finished` protégé AES-GCM. |
| Dérivation | X25519 → premaster secret → master secret TLS → key block AES-128-GCM. |
| Session | Le `Finished` client est ouvert avec le key block côté serveur ; la séquence lecture passe à 1. |

Le flight construit 93 octets de records : 42 octets pour `ClientKeyExchange`, 6 octets pour `ChangeCipherSpec` et 45 octets pour le record `Finished` chiffré. Le transcript final mesure 671 octets, après ajout de `ClientKeyExchange` et du message `Finished` clair avant protection.

## Rejets et caractère transactionnel

Le scénario de bout en bout complète les rejets déjà présents au niveau de `ServerKeyExchange`. Il copie le record `Finished` AES-GCM, altère son dernier octet d’authentification et tente son ouverture dans une session fraîche. L’ouverture échoue et `read_sequence` demeure strictement à zéro. Cette vérification empêche qu’un paquet authentifié invalide provoque une désynchronisation du compteur de nonce.

| Rejet | Effet attendu |
|---|---|
| Signature ECDSA de `ServerKeyExchange` altérée | État conservé à `CERTIFICATE_RECEIVED`, clé éphémère non publiée. |
| Buffer de flight insuffisant | État `SERVER_HELLO_DONE_RECEIVED` et transcript préservés. |
| Tag AES-GCM du `Finished` altéré | Record refusé, `read_sequence = 0`. |
| Suite non-ECDHE | Préparation X25519 refusée. |

## Mémoire

Le scénario utilise des tableaux locaux ou statiques de taille fixe : un workspace ECDSA de 2 048 mots, un workspace X25519 de 136 mots, un workspace PRF de 256 octets, un transcript de 1 024 octets et un buffer de flight de 128 octets. Tous ces objets sont caller-owned ; aucune allocation dynamique n’est ajoutée au chemin réseau/TLS/HTTP/LLM.

## Stabilité des smokes QEMU

Le smoke de contrôle fournisseur IA attend désormais jusqu’à 30 secondes le premier marqueur série du shell, contre 15 auparavant. Le journal du cas instable montrait un boot complet atteignant le marqueur juste après cette ancienne limite. Les assertions de toutes les commandes `ai-*`, leur nombre de tentatives et leurs délais d’exécution restent inchangés ; seul le délai initial, borné, est adapté aux runners QEMU plus lents.

## Validation

| Vérification | Résultat |
|---|---|
| `test_net_tls_record` ciblé | 25/25 scénarios passés. |
| `make test-build` | Image i386 freestanding compilée avec succès. |
| `make test-all` | **394/394** tests passés. |
| `make qemu-ai-provider` | Smoke fournisseur IA réussi après délai de boot borné à 30 s. |
| `make qemu-ne2k-status` | Smoke NE2000 réussi. |

## Fichiers modifiés

| Fichier | Rôle |
|---|---|
| `tests/unit/kernel/test_net_tls_record.c` | Scénario ECDHE_ECDSA de bout en bout, déchiffrement AES-GCM et rejet de tag altéré. |
| `tests/scripts/test_ai_provider_commands.py` | Délai d’attente initial de boot porté à 30 secondes. |

## Limites et suite logique

Ce lot couvre le flight client et son `Finished` côté serveur de test. Le prolongement naturel est la réception de `ChangeCipherSpec` et de `Finished` serveur dans un scénario ECDHE_ECDSA complet, puis un échange HTTPS applicatif contrôlé sur la session établie.

## Références

[1] [IETF, *RFC 8422 — Elliptic Curve Cryptography (ECC) Cipher Suites for TLS Versions 1.2 and Earlier*](https://www.rfc-editor.org/rfc/rfc8422.html)

[2] [IETF, *RFC 5246 — The Transport Layer Security (TLS) Protocol Version 1.2*](https://datatracker.ietf.org/doc/html/rfc5246)
