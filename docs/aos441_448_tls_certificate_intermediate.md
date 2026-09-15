# AOS-441 à AOS-448 — premier intermédiaire de la liste Certificate TLS

Le parseur TLS 1.2 `Certificate` conserve maintenant deux vues caller-owned : le premier certificat de la liste, interprété comme la feuille serveur, et le deuxième, interprété comme le premier intermédiaire. Les vues pointent dans le message TLS assemblé ; aucun certificat n’est copié et aucune allocation dynamique n’est introduite.

| Donnée extraite | Source | Usage prévu |
|---|---|---|
| `certificate` | Premier élément de `certificate_list` | Feuille serveur, hostname et signature de `ServerKeyExchange`. |
| `intermediate` | Deuxième élément de `certificate_list` | Validation `leaf → intermédiaire → ancre` par le polling NE2000 à chaîne. |
| Éléments suivants | Vérifiés pour le framing, non conservés | Hors périmètre de cette version. |

`net_tls_handshake_t` conserve aussi les longueurs et les vues X.509 de la feuille et de l’intermédiaire. Le parsing X.509 de l’intermédiaire est déclenché seulement lorsqu’un second certificat est présent ; un DER intermédiaire invalide invalide la feuille déjà parsée pour empêcher toute utilisation partielle.

Les tests utilisent une liste TLS à deux certificats, vérifient les références et les longueurs du second élément, puis conservent les rejets de liste et de taille invalides.

> Le lot ne sélectionne pas une chaîne parmi plusieurs intermédiaires, ne vérifie pas l’ancre transmise par le serveur et ne traite pas une profondeur arbitraire. L’intégration automatique de la vue extraite dans la politique NE2000 à chaîne reste le prochain pas.
