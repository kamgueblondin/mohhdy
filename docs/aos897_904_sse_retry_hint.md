# AOS-897 à AOS-904 — hint `retry:` SSE borné

## Objectif

Ce macro-lot accepte le champ SSE `retry:` dans l’accumulateur caller-owned. La valeur est décodée comme un entier décimal strict en millisecondes, sans signe ni caractère parasite, et bornée à `NET_LLM_SSE_RETRY_MAX_MS`, soit 600 000 ms. Le hint est exposé par `retry_delay_ms` et `retry_valid` ; il n’est pas appliqué implicitement au scheduler, au timer IRQ ou au poller réseau.

| Cas | Comportement |
|---|---|
| `retry: 1500` | Valeur 1 500 ms mémorisée |
| Espaces après `:` | Acceptés selon le framing SSE déjà établi |
| Valeur vide | Rejetée sans mutation |
| Caractère non décimal | Rejeté sans mutation |
| Valeur supérieure à 600 000 ms | Rejetée avec saturation refusée |
| Mémoire | Champs intégrés à la structure caller-owned, aucun `kmalloc` |

Le parser continue de concaténer les lignes `data:`, de mémoriser `id:` et de différer l’extraction JSON jusqu’à l’événement complet. Le hint peut donc être reçu avec un événement qui produit effectivement un delta Unicode, sans sortie partielle.

## Validation

Le test injecte `retry: 1500` avec un delta Ollama `ok`, vérifie la valeur mémorisée et couvre le rejet d’une valeur `600001`. La suite HTTP/TLS ciblée passe à **23/23 tests**. La suite globale, le build i386 et les smokes QEMU constituent la validation de publication.

## Limites restantes

Le scheduler de reconnexion ne consomme pas encore automatiquement le hint ; l’appelant doit choisir explicitement son budget et son backoff. Le raccordement au poller NE2000, le jitter, les timers IRQ et la persistance inter-session restent hors périmètre de ce macro-lot.
