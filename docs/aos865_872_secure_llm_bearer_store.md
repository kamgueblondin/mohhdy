# AOS-865 à AOS-872 — store bearer sécurisé pour les appels LLM

## Objectif

Ce macro-lot ajoute un store fixe de **128 octets** pour un bearer OpenAI-compatible. Le secret est copié dans une structure caller-owned bornée, validé comme chaîne ASCII imprimable non vide, puis utilisé uniquement par un builder POST qui fabrique une copie locale terminée par `NUL`. L’API ne fournit aucune fonction de lecture du token.

L’effacement réécrit l’intégralité du tableau de 128 octets, remet la longueur à zéro et désactive l’état `provisioned`. Les erreurs de taille ou de caractères invalide n’écrasent pas un credential déjà provisionné.

| Garantie | Implémentation |
|---|---|
| Capacité | 128 octets fixes, token utile inférieur à 128 pour le terminateur local |
| Validation | Octets ASCII 33 à 126 uniquement |
| Exposition | Aucun getter ; le builder reçoit seulement le store opaque |
| Effacement | Zéroisation de tout le tableau et invalidation de l’état |
| Mémoire | Structure fixe, copie locale fixe, aucun `kmalloc` |
| ABI | Aucun champ credential ajouté à l’ABI Ring 3 |

Cette façade ne prétend pas être un coffre matériel : le store est caller-owned et doit être instancié dans un contexte privilégié. Le raccordement à une source de secret de boot, à l’entropie matérielle et à une politique de rotation reste un lot distinct.

## Validation

La suite HTTP/TLS ciblée passe à **19/19 tests**. Les scénarios vérifient le provisionnement, la construction d’un POST Authorization, le rejet d’un saut de ligne et d’une taille égale à la capacité, puis l’effacement complet du store.

## Limites restantes

Le lot ne lit aucun secret depuis Ring 3, le shell, l’image initrd ou le dépôt. Il ne fournit pas encore de stockage persistant chiffré, de TPM, de rotation, de limitation d’usage, ni de raccordement automatique au chemin HTTPS ECDHE_ECDSA.
