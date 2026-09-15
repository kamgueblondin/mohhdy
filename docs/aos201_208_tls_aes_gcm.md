# AOS-201 à AOS-208 — AES-128-GCM et records TLS protégés

Ce macro-lot ajoute la protection effective des records TLS 1.2 pour la suite AES-128-GCM. Il implémente AES-128, GHASH, GCM, le format de record TLS avec nonce explicite de huit octets, le tag d’authentification de seize octets et les séquences de lecture/écriture caller-owned. Toutes les primitives sont freestanding et n’utilisent aucune allocation dynamique.

| Lot | Capacité livrée | Contrat mémoire |
|---|---|---|
| AOS-201 | Expansion de clés AES-128 | Contexte de 176 octets fourni par l’appelant |
| AOS-202 | GHASH et AES-GCM | Entrées, sorties et tag caller-owned |
| AOS-203 | Construction de record TLS AEAD | Buffer record caller-owned |
| AOS-204 | Ouverture authentifiée de record | Plaintext publié uniquement après tag valide |
| AOS-205 | Session de séquences TLS | Vues de clés/IV et compteurs caller-owned |
| AOS-206 | Émission TCP transactionnelle | Rollback de séquence TLS si TCP échoue |
| AOS-207 | Réception TCP transactionnelle | Rollback TCP/TLS si tag ou déchiffrement échoue |
| AOS-208 | Harness et build freestanding | AES-GCM lié aux tests et au noyau i386 |

Le record chiffré suit la forme TLS 1.2 GCM : en-tête de cinq octets, nonce explicite de huit octets, ciphertext puis tag de seize octets. L’AAD associe le numéro de séquence TLS, le type de contenu, la version et la longueur du plaintext. Le nonce explicite est construit à partir de la séquence en écriture; la lecture utilise la séquence attendue dans l’AAD et n’avance son compteur qu’après authentification réussie.

Les deux chemins TCP sont transactionnels. Lors de l’émission, la séquence TLS est restaurée si l’encapsulation TCP échoue. Lors de la réception, un tag invalide restaure l’état TCP et la séquence TLS, de sorte qu’aucun record rejeté ne consomme de fenêtre ou de nonce. Le déchiffrement AES-CTR ne démarre qu’après la vérification du tag GCM.

| Validation | Résultat |
|---|---|
| Vecteur AES-128 FIPS | Validé |
| Vecteur AES-GCM NIST | Validé |
| Test de record TLS AES-GCM | Round-trip et tag falsifié couverts |
| Test TCP ↔ TLS AES-GCM | Séquences et plaintext couverts |
| Suite Unity complète | 330/330 tests verts |
| Build i386 | Réussi |
| Smokes QEMU IA et NE2000 | Réussis |

> Les records peuvent désormais être chiffrés et authentifiés avec des clés AES-128-GCM dérivées par le macro-lot précédent. Il reste néanmoins indispensable d’achever ECDHE réel, X.509, la vérification de signature `ServerKeyExchange`, la validation des suites négociées et l’orchestration de la connexion de production avant de qualifier les appels LLM comme TLS sécurisés de bout en bout.
