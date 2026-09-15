# AOS-873 à AOS-880 — raccordement HTTPS du store bearer

## Objectif

Ce macro-lot raccorde le store bearer fixe au chemin HTTPS applicatif existant. `net_http_tls_build_post_json_bearer_store` construit le POST Authorization à partir du store opaque, puis réutilise le même encapsulage TLS AES-128-GCM et le même transport TCP que le POST JSON historique.

Le token n’est pas passé comme argument textuel à l’appelant TLS. Si le store n’est pas provisionné, la fonction échoue avant toute émission. Le test déchiffre le record côté serveur de fixture et compare l’intégralité de la requête HTTP, y compris `Authorization: Bearer`, `Content-Length` et le corps JSON.

| Élément | Garantie |
|---|---|
| Source du bearer | Store fixe provisionné et validé |
| Transport | TCP existant et record TLS AES-128-GCM existant |
| Exposition | Pas de getter ni d’argument token dans l’API TLS store |
| Échec | Store absent refusé avant émission |
| Mémoire | Buffers caller-owned et copies fixes uniquement |
| ABI | Aucun credential exposé à Ring 3 |

## Validation

Le test HTTP/TLS ciblé passe à **20/20**. La fixture ouvre une paire TLS, construit le POST store, parse le segment TCP, déchiffre le record serveur et vérifie la requête complète. La suite globale atteint **402/402 tests** avec build i386 et smokes QEMU verts.

## Limites restantes

Le store reste caller-owned et ne constitue pas encore un coffre matériel. La provision depuis un secret de boot privilégié, la rotation, la persistance chiffrée, le TPM, la limitation d’usage et l’intégration à la sélection dynamique du fournisseur restent des fonctionnalités distinctes.
