# AOS-985 à AOS-992 — postflight TLS ECDHE_ECDSA transactionnel

`net_tls_handshake_accept_server_postflight` regroupe les deux derniers messages serveur du handshake TLS. Il exige les buffers caller-owned de `ChangeCipherSpec`, `Finished` et `expected_verify_data`, puis délègue aux parseurs et transitions déjà validés.

L’opération copie uniquement la petite structure d’état du handshake pour fournir un rollback atomique. Si `ChangeCipherSpec` ou `Finished` échoue, l’état revient exactement à sa valeur précédente. Une réussite atteint `NET_TLS_HANDSHAKE_SERVER_FINISHED_RECEIVED`, ce qui autorise ensuite l’utilisation des records applicatifs.

Ce lot ne dérive aucune clé et ne contourne pas la vérification ECDSA de `ServerKeyExchange`; il ferme le postflight après les étapes X25519, PRF et AES-GCM existantes. Aucun `kmalloc` n’est utilisé.

Auteur : **Manus AI**
