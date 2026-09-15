# AOS-417 à AOS-424 — chaîne X.509 avec intermédiaire RSA

`x509_certificate_chain_validate_two` valide une chaîne ordonnée `leaf → intermédiaire → ancre`. Elle réutilise la vérification RSA PKCS#1 v1.5 SHA-256 déjà disponible deux fois, sans allouer de mémoire : la signature de la feuille est vérifiée avec la clé publique de l’intermédiaire, puis la signature de l’intermédiaire avec la clé publique de l’ancre.

| Étape | Vérification |
|---|---|
| Feuille → intermédiaire | `issuer` de la feuille égal au `subject` de l’intermédiaire, OID SHA-256/RSA, clé RSA de l’intermédiaire et signature de la feuille. |
| Intermédiaire → ancre | `issuer` de l’intermédiaire égal au `subject` de l’ancre, OID SHA-256/RSA, clé RSA de l’ancre et signature de l’intermédiaire. |
| Politique TLS | Chaîne complète, hostname de la feuille et période UTC de la feuille, de l’intermédiaire et de l’ancre. |

`x509_certificate_tls_identity_validate_two` conserve les mêmes paramètres caller-owned que la politique directe, avec un pointeur supplémentaire vers l’intermédiaire. Le workspace RSA est réutilisé séquentiellement pour les deux vérifications.

Les tests contiennent une vraie chaîne RSA 1024 bits DER générée localement pour le harnais : une racine signe un intermédiaire, qui signe une feuille avec SAN `api.example.test`. Ils valident la chaîne et la politique à trois certificats, puis rejettent un sujet d’intermédiaire incohérent et une signature intermédiaire tronquée.

> Le lot n’interprète pas encore `BasicConstraints`, `KeyUsage`, `pathLenConstraint`, `AuthorityKeyIdentifier`, CRL/OCSP, ECDSA, ni une liste d’intermédiaires arbitraire. Il ne choisit aucune chaîne automatiquement : l’ordre leaf/intermédiaire/ancre reste fourni et contrôlé par l’appelant.
