# AOS-457 à AOS-464 — `pathLenConstraint` X.509

La vue de certificat X.509 conserve désormais la présence et la valeur de `pathLenConstraint` issue de `BasicConstraints`. Le parseur DER accepte uniquement un entier positif canoniquement encodé, de taille compatible avec un entier 32 bits, et refuse une contrainte présente lorsque `CA` n’est pas vrai.

| Élément | Contrôle appliqué |
|---|---|
| INTEGER DER | Non vide, non négatif, longueur bornée et sans zéro de tête superflu. |
| BasicConstraints | `pathLenConstraint` impose `CA=true`. |
| Chaîne leaf → intermédiaire → ancre | Une ancre ayant `pathLenConstraint=0` est refusée, car la chaîne contient un certificat CA intermédiaire. |
| Intermédiaire | Sa contrainte est conservée ; pour la profondeur actuellement prise en charge, il n’a aucun CA sous-jacent. |

La vérification de chaîne à deux niveaux applique donc la profondeur d’ancre avant les vérifications RSA. Les tests forcent la valeur de l’ancre à zéro, vérifient le refus, puis restaurent la vue afin de préserver les autres vecteurs de chaîne.

> La pile ne gère encore qu’un seul intermédiaire. L’application générique de `pathLenConstraint` à une liste arbitraire de CA, les certificats auto-émis en cours de chaîne, les contraintes de nom, AKI/SKI, les extensions critiques, ECDSA et la révocation restent hors périmètre.
