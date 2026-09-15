# AOS-473 à AOS-480 — liaison X.509 AKI/SKI

La validation de chaîne RSA prend désormais en compte les extensions `SubjectKeyIdentifier` (SKI) et `AuthorityKeyIdentifier` (AKI). Le parseur expose des pointeurs et longueurs vers les key identifiers contenus dans le DER caller-owned ; il ne copie ni certificat ni extension.

| Extension | Donnée extraite | Effet de validation |
|---|---|---|
| SubjectKeyIdentifier | OCTET STRING du certificat autorité | Identifie la clé publique de l’émetteur. |
| AuthorityKeyIdentifier | Champ contextuel `[0] keyIdentifier` de l’enfant | Lorsque présent, doit correspondre exactement au SKI de l’autorité. |
| AKI absent | Aucune comparaison additionnelle | Le contrôle d’émetteur DN et la signature RSA existants restent appliqués. |

La fonction de validation directe vérifie d’abord le DN d’émetteur, puis la cohérence AKI/SKI si l’enfant fournit un AKI, avant la validation de clé RSA et de signature. La chaîne à intermédiaire bénéficie automatiquement de ce contrôle aux deux niveaux.

Les vecteurs DER root/intermédiaire/leaf testent la présence des quatre identifiants utiles. Le test remplace temporairement le SKI de l’intermédiaire par une valeur incompatible et vérifie le rejet de la chaîne.

> Le validateur ne gère pour l’instant que le `keyIdentifier` d’AKI. Les champs `authorityCertIssuer` et `authorityCertSerialNumber`, les chaînes multiples, la révocation, ECDSA et les contraintes de nom restent hors périmètre.
