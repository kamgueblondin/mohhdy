# AOS-337 à AOS-344 — validation de dates X.509

`x509_certificate_valid_at` valide la période d’un certificat contre un instant UTC fourni explicitement par l’appelant sous la forme `YYYYMMDDhhmmssZ`. La fonction accepte les dates ASN.1 `UTCTime` et `GeneralizedTime` avec suffixe `Z`, contrôle les chiffres, mois, jours, heures, minutes, secondes et années bissextiles, puis impose `notBefore <= instant <= notAfter`.

Aucune horloge système, timer PIT ou date compilée n’est utilisée. Un client qui ne dispose pas d’une heure fiable doit considérer cette validation comme indisponible et ne pas transformer son absence en succès de confiance.

Les tests couvrent une date dans la période de validité, une date antérieure, une date postérieure et une date calendairement invalide.

> Le lot ne fournit ni lecture RTC/NTP, ni synchronisation sécurisée de l’heure, ni politique de tolérance d’horloge. Les dates doivent donc être fournies par une source approuvée par la couche appelante.
