# AOS-465 à AOS-472 — extensions X.509 critiques

Le parseur X.509 traite maintenant explicitement le champ optionnel `critical` de chaque extension. Une extension inconnue marquée critique est rejetée, car sa sémantique est obligatoire pour accepter correctement le certificat. Une extension inconnue non critique reste ignorée conformément au comportement déjà établi.

| Extension reconnue | Traitement |
|---|---|
| Subject Alternative Name | Conserve les DNS names pour la validation de hostname. |
| BasicConstraints | Conserve `CA` et `pathLenConstraint`. |
| KeyUsage | Conserve le bit `keyCertSign` pour les intermédiaires. |
| Extension inconnue critique | Rejet du certificat. |

Le booléen DER `critical`, lorsqu’il est présent, doit avoir une longueur d’un octet et une valeur DER booléenne valide (`00` ou `FF`). Le parser ne copie aucune extension ; toutes les vues restent attachées au buffer de certificat caller-owned.

Les tests modifient un vecteur DER de confiance pour transformer une extension `KeyUsage` critique connue en OID inconnu. Le certificat est alors rejeté, tandis que le vecteur original reste accepté.

> Les extensions non critiques inconnues sont encore ignorées. La pile ne traite pas encore AuthorityKeyIdentifier, SubjectKeyIdentifier, NameConstraints, CertificatePolicies, CRL Distribution Points, AIA/OCSP, ECDSA ni la révocation.
