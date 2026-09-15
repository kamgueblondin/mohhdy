# AOS-401 à AOS-408 — source UTC RTC i386

Le module `rtc` lit l’horloge CMOS i386 par les ports `0x70` et `0x71`, au moyen d’une interface de ports injectée pour les tests et d’un adaptateur matériel réel disponible uniquement dans le noyau i386. `rtc_read_utc` écrit dans un buffer caller-owned de 16 octets la chaîne canonique `YYYYMMDDHHMMSSZ`, directement acceptable par la politique de dates X.509 existante.

| Contrôle | Comportement |
|---|---|
| Mise à jour CMOS | Au plus trois photographies sont tentées ; le bit UIP ou une seconde modifiée invalide la photographie. |
| Encodage | Les registres BCD et binaires sont acceptés. |
| Heure | Les modes 12 h avec PM et 24 h sont convertis en UTC 24 h. |
| Calendrier | Le mois, le jour, les heures, minutes, secondes et les années bissextiles sont vérifiés. |
| Mémoire | Aucun état global, allocation dynamique ni buffer caché. |

Pour la validation TLS, l’appelant alloue `char utc[RTC_UTC_BUFFER_LENGTH]`, appelle `rtc_read_utc`, puis transmet cette chaîne à `ne2k_tls_client_poll` ou à `x509_certificate_tls_identity_validate`. Un échec RTC doit faire échouer la politique de temps ; il ne doit pas être remplacé par une date arbitraire.

> Le module suppose que le RTC fournit UTC et fixe le siècle à `2000 + année CMOS`. Il ne vérifie pas la batterie RTC, le fuseau configuré par firmware, le siècle CMOS, la dérive d’horloge, NTP ou une attestation temporelle réseau.

Les tests injectent des registres CMOS et couvrent une date BCD en 12 h, une date binaire en 24 h pendant une année bissextile, un BCD invalide, une capacité insuffisante et un bit UIP persistant.
