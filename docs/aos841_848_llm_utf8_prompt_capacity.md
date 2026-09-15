# AOS-841 à AOS-848 — capacité de prompt UTF-8 du service LLM

## Objectif

Ce macro-lot étend `OS_LLM_PROMPT_MAX` de **256 à 1 024 octets**. La requête Ring 3 demeure un POD borné : le prompt est copié par valeur dans la structure ABI, puis validé par les builders JSON existants avant émission. Aucun pointeur utilisateur, secret, buffer global supplémentaire ou appel à `kmalloc` n’est introduit.

L’extension permet au service LLM interactif de transmettre des prompts UTF-8 plus longs, notamment des textes composés de caractères multi-octets. Elle reste compatible avec les buffers JSON et TLS existants, qui disposent d’une capacité supérieure au nouveau plafond de prompt.

## Contrat et bornes

| Élément | Avant | Après | Garantie |
|---|---:|---:|---|
| `OS_LLM_PROMPT_MAX` | 256 octets | 1 024 octets | Copie par valeur, sans pointeur |
| `OS_LLM_TEXT_MAX` | 2 048 octets | 2 048 octets | Inchangé depuis AOS-833 |
| Prompt testé | — | 1 020 octets | 1 016 `a` et un emoji UTF-8 |
| Prompt au-delà de la capacité JSON | Rejet | Rejet | Aucune sortie partielle |

Le shell conserve sa garde de capacité avant de copier les arguments dans `os_llm_request_t`. Les builders refusent toujours les séquences UTF-8 tronquées, les contrôles interdits et toute capacité de sortie insuffisante.

## Validation

Le test `test_llm_large_utf8_prompt_builder` construit un prompt de 1 020 octets contenant un emoji `U+1F600` encodé en `F0 9F 98 80`, vérifie que le builder Ollama accepte cette entrée dans un buffer suffisant et rejette une capacité JSON trop courte.

Validation locale : **398 tests réussis sur 398**, build des tests complet et non-régression de tous les modules noyau et utilisateurs. Les smokes QEMU fournisseur et NE2000 seront exécutés avec le build final avant la PR.

## Limites conservées

Le lot ne fournit pas de pagination, de prompt multi-segment dans l’ABI, de pièces jointes, de multimodalité ou de transmission de secrets. Les champs `model` et `path` restent ASCII imprimable selon le contrat actuel, tandis que le prompt accepte uniquement l’UTF-8 canonique déjà pris en charge par le codec JSON.
