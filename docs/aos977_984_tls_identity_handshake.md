# AOS-977 à AOS-984 — rattachement de l’identité X.509 au handshake TLS

`net_tls_handshake_validate_server_identity` raccorde explicitement l’état TLS à la validation d’identité X.509 déjà disponible. Le caller fournit le trust anchor, le hostname, l’instant UTC et le workspace fixe. La fonction refuse un handshake nul, un trust anchor absent, des paramètres temporels absents ou un certificat serveur non parsé.

La validation réutilise le contrôle de chaîne, de nom DNS et de période de validité. Elle ne copie ni certificat, ni hostname, ni secret. La vérification cryptographique ECDSA du message `ServerKeyExchange` demeure dans le chemin authentifié existant et n’est pas contournée par ce wrapper.

Ce lot fournit le point d’intégration nécessaire au fournisseur HTTPS sans introduire d’allocation dynamique. Les vecteurs X.509 et ECDSA déjà présents restent la source de non-régression.

Auteur : **Manus AI**
