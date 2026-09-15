# AOS-1857…1864 — Parsing de `close_notify` distant

## Objet

`net_tls_close_notify_parse()` valide un record TLS déjà déchiffré afin de reconnaître exclusivement l’alerte de fermeture propre `warning/close_notify` reçue du pair.

| Contrôle | Résultat |
|---|---:|
| Record Alert, deux octets `1, 0` | `0` |
| Record nul, type non Alert ou payload absent | `-1` |
| Longueur différente de deux octets | `-2` |
| Niveau ou description différents | `-3` |

La fonction ne modifie ni session AES-GCM, ni numéro de séquence, ni buffer de plaintext. Elle peut donc être appelée après `net_tls_aes_gcm_session_open()` sans modifier les garanties transactionnelles de déchiffrement.

Le vecteur TLS couvre la fermeture distante valide, un niveau fatal, une description différente, une longueur invalide et un record applicatif. La suite ciblée est verte à **29/29**.

## Référence

[1] [RFC 5246 — TLS 1.2, §7.2.1](https://www.rfc-editor.org/rfc/rfc5246#section-7.2.1)
