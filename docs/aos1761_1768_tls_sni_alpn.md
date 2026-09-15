# AOS-1761…1768 — ClientHello TLS 1.2 avec SNI et ALPN

## Objet

Ce macro-lot étend le ClientHello TLS 1.2 du noyau avec deux extensions optionnelles construites dans un buffer caller-owned : **Server Name Indication (SNI)** et **Application-Layer Protocol Negotiation (ALPN)**. L’API historique reste inchangée et construit le même ClientHello minimal lorsqu’aucune extension applicative n’est demandée.

| Élément | Garantie |
|---|---|
| SNI | nom ASCII `A–Z`, `a–z`, `0–9`, point et tiret, borné à 253 octets |
| ALPN | identifiant ASCII imprimable, borné à 32 octets |
| Buffer | taille calculée avant écriture, échec atomique si insuffisante |
| Mémoire | tableau local borné, aucune allocation dynamique |

## Conception

`net_tls_client_hello_sni_alpn_build()` garde les extensions TLS déjà annoncées pour X25519 et les algorithmes de signature. Elle ajoute, quand les paramètres sont fournis, l’extension SNI de type `0x0000` et l’extension ALPN de type `0x0010`. Les longueurs du handshake, du bloc d’extensions, de la liste SNI et de la liste ALPN sont recalculées à partir des entrées validées.

La validation refuse les valeurs vides, les contrôles, les caractères hors ASCII imprimable, les séparateurs non autorisés dans le nom d’hôte et les capacités insuffisantes. Aucune chaîne n’est copiée en dehors du record fourni par l’appelant.

## Validation

Le vecteur `api.example.test` / `http/1.1` vérifie le record TLS produit, la longueur du ClientHello, les extensions SNI et ALPN et les octets de leurs charges. Les cas de nom d’hôte invalide, de buffer insuffisant et de nom vide sont rejetés.

| Vérification | Résultat |
|---|---|
| ClientHello historique minimal | Compatible |
| SNI `api.example.test` | Sérialisé |
| ALPN `http/1.1` | Sérialisé |
| Rejets de bornes et caractères | Confirmés |
| `make -s test-all` | **464/464 tests réussis** |

## Références

[1] [RFC 6066 — Server Name Indication](https://www.rfc-editor.org/rfc/rfc6066#section-3)  
[2] [RFC 7301 — Application-Layer Protocol Negotiation](https://www.rfc-editor.org/rfc/rfc7301)  
[3] [RFC 5246 — TLS 1.2 ClientHello](https://www.rfc-editor.org/rfc/rfc5246#section-7.4.1.2)
