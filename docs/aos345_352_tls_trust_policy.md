# AOS-345 à AOS-352 — politique TLS de confiance combinée

`x509_certificate_tls_identity_validate` compose les trois validations déjà disponibles : chaîne RSA directe vers l’ancre caller-owned, correspondance DNS du hostname et validité `notBefore`/`notAfter` contre un instant UTC caller-owned. Le succès n’est publié que lorsque les trois contrôles réussissent.

| Contrôle | Source d’entrée |
|---|---|
| Chaîne | Feuille X.509, ancre X.509 RSA et workspace RSA fournis par l’appelant. |
| Identité | Hostname DNS ASCII fourni par l’appelant. |
| Temps | Instant UTC `YYYYMMDDhhmmssZ` fourni par l’appelant. |

Le test couvre un certificat feuille signé par l’ancre de test, le SAN `api.example.test` et une date dans sa période ; il rejette séparément un hostname erroné et une date antérieure à la période.

> La politique ne fournit pas elle-même l’heure, l’ancre ni le hostname. L’orchestrateur NE2000 doit encore recevoir et transmettre l’instant UTC fiable à cette politique avant d’être considéré comme une validation TLS de production complète.

Les intermédiaires, contraintes/usage de clés, révocation, ECDSA, source RTC/NTP sécurisée et exécution automatique dans le handshake restent hors périmètre.
