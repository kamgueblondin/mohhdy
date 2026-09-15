# AOS-425 à AOS-432 — contraintes CA X.509 des intermédiaires

La validation de chaîne à intermédiaire exige désormais que l’intermédiaire porte simultanément les extensions X.509 `BasicConstraints` avec `CA=true` et `KeyUsage` avec le bit `keyCertSign`. Le parseur DER expose ces quatre indicateurs directement dans la vue de certificat caller-owned, sans copie ni allocation.

| Extension | Décodage retenu | Effet sur `leaf → intermédiaire → ancre` |
|---|---|---|
| BasicConstraints | Séquence DER, booléen `CA` et entier `pathLenConstraint` optionnel correctement formés | L’intermédiaire doit présenter `CA=true`. |
| KeyUsage | BIT STRING DER avec nombre de bits inutilisés valide | L’intermédiaire doit porter `keyCertSign`. |
| SAN | Comportement inchangé | Continue de déterminer le hostname de la feuille. |

La fonction `x509_certificate_chain_validate_two` rejette l’intermédiaire avant la vérification RSA si l’une des deux autorisations manque. La vérification des signatures, émetteurs, dates UTC et hostname conserve les garanties du lot précédent.

Les vecteurs root/intermédiaire/leaf RSA existants contiennent ces extensions. Les tests vérifient leur décodage, la chaîne valide, puis le refus lorsque `CA=true` ou `keyCertSign` est retiré de la vue intermédiaire.

> Le validateur ne fait pas encore respecter la valeur numérique de `pathLenConstraint`, `critical`, `AuthorityKeyIdentifier`, `SubjectKeyIdentifier`, les contraintes de nom, l’usage de l’ancre, CRL/OCSP, ECDSA ou une chaîne d’intermédiaires arbitraire.
