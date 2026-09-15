# AOS-178 — Parsing du message Certificate TLS 1.2

AOS-178 ajoute le parsing borné du message `Certificate` TLS 1.2. Le codec vérifie le type de handshake, la longueur 24 bits du corps, la longueur 24 bits de `certificate_list` et chaque longueur de certificat avant tout accès. La première chaîne est publiée dans `net_tls_certificate_view_t` sans copie, tandis que les certificats suivants sont parcourus pour valider l’intégrité de la liste.

L’état du handshake accepte ce message uniquement après `SERVER_HELLO_RECEIVED` et passe alors à `CERTIFICATE_RECEIVED`. Les pointeurs de certificat et leurs longueurs appartiennent à la mémoire fournie par l’appelant. Aucun buffer interne, état global ou appel à `kmalloc` n’est introduit.

Les tests couvrent une liste contenant deux certificats, la publication caller-owned du premier certificat, les longueurs de liste incohérentes, les certificats vides et les transitions invalides de l’automate.

| Élément | Statut |
|---|---|
| Parsing du type Certificate TLS 1.2 | Implémenté |
| Longueurs 24 bits et bornes | Validé |
| Publication de la première chaîne sans copie | Implémentée |
| Parcours de plusieurs certificats | Validé |
| Transition `SERVER_HELLO_RECEIVED → CERTIFICATE_RECEIVED` | Implémentée |
| Validation X.509 et chaîne de confiance | Non implémentées |
| Dérivation de clés, chiffrement et Finished | Non implémentés |
| HTTP et appels LLM sécurisés de bout en bout | Non fonctionnels |

AOS-178 est validé par le test TLS ciblé. La validation complète, le build i386 et les smokes QEMU doivent être exécutés avant publication de la prochaine pull request.
