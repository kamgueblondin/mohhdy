# AOS-833 à AOS-840 — réponses LLM fragmentées et propagation Unicode

## Objectif

Ce macro-lot prolonge AOS-809 à AOS-832 en faisant passer la sortie texte du service LLM interactif de **512 à 2 048 octets**. La limite reste une capacité fixe de l’ABI Ring 3 ; elle n’introduit ni pointeur utilisateur, ni allocation dynamique, ni conservation de secret.

Le chemin HTTP `Content-Length` existant accumule déjà les fragments TLS dans un buffer noyau borné de 8 192 octets. Une fois le corps complet publié, l’extracteur JSON Unicode décode les séquences UTF-8 brutes et les échappements `\\uXXXX`, y compris les paires surrogate. Le changement rend cette sortie suffisamment large pour transmettre des réponses LLM fragmentées contenant un texte volumineux sans tronquer le résultat à 512 octets.

## Contrat de capacité

| Élément | Valeur | Propriété |
|---|---:|---|
| Buffer HTTP noyau | 8 192 octets | Statique, privé au noyau |
| Sortie texte Ring 3 | 2 048 octets | POD caller-owned, copié par valeur |
| Prompt Ring 3 | 256 octets | Inchangé dans ce lot |
| Réponse de test | 1 015 octets JSON | Reçue en deux fragments |
| Texte extrait de test | 1 000 octets | 996 caractères ASCII et un emoji UTF-8 |

La publication reste transactionnelle : un corps incomplet retourne l’état d’attente sans publier de réponse complète ; une capacité insuffisante ou un JSON invalide retourne une erreur sans sortie partielle exploitable.

## Validation

Le test `test_llm_large_fragmented_unicode_response` construit une réponse Ollama de 1 015 octets, la transmet à l’accumulateur HTTP en deux fragments, vérifie le statut `200`, puis extrait 1 000 octets de texte. Les quatre derniers octets correspondent à l’emoji `U+1F600` encodé en UTF-8 (`F0 9F 98 80`).

La validation locale du macro-lot donne **397 tests réussis sur 397**, avec compilation complète et `git diff --check` propres. Les smokes QEMU doivent encore confirmer le build i386 et le plan de contrôle fournisseur avant l’ouverture de la PR.

## Limites conservées

Ce lot ne transforme pas le codec en parseur JSON général, ne prend pas en charge les tableaux structurés ou les tool calls, et ne fournit pas de pagination applicative au-delà de la capacité fixe de 2 048 octets. Le streaming SSE conserve son contrat borné existant. Les réponses supérieures aux buffers HTTP noyau ou à la sortie ABI sont refusées plutôt que tronquées.

## Référence de conception

Le choix d’une capacité POD accrue préserve les invariants du noyau i386 freestanding : toutes les zones restent statiques, caller-owned ou noyau-privées, et aucune invocation de `kmalloc` n’est introduite.
