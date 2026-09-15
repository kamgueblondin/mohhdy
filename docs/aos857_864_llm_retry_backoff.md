# AOS-857 à AOS-864 — backoff borné des retries HTTP LLM

## Objectif

Ce macro-lot ajoute `net_llm_http_retry_schedule`, une primitive caller-owned qui combine la classification HTTP retryable, la consommation d’un budget limité et le calcul d’un prochain instant d’essai. La fonction ne bloque jamais, ne lit aucune horloge implicite et ne réémet aucun paquet : l’orchestrateur réseau reste responsable de l’attente et de la retransmission.

Le délai suit un backoff exponentiel borné : `base_delay`, puis le double à chaque tentative, jusqu’à `max_delay`. L’addition à l’instant courant est saturante afin d’éviter un retour dans le passé lors d’un dépassement de `uint32_t`.

| Paramètre | Garantie |
|---|---|
| Statut HTTP | Seuls 408, 425, 429 et 5xx sont retryables |
| Budget | Aucun dépassement de `retry_limit` |
| Délais | `base`, `2*base`, puis plafond `max` |
| Horloge | Instant caller-owned en unités abstraites |
| Overflow | Publication saturée à `UINT32_MAX` |
| Mémoire | Aucun état global ni allocation dynamique |

Les appels non-retryables et les paramètres invalides ne modifient ni le compteur ni l’instant de reprise.

## Validation

Les vecteurs vérifient les délais 10, 20 et 25, l’épuisement d’un budget de trois tentatives, le refus d’un statut `200` et le rejet transactionnel d’un délai de base nul. La suite complète doit confirmer la non-régression TCP/TLS et QEMU.

## Limites

Ce lot ne fournit pas encore de temporisation matérielle, de timer IRQ, de jitter, de circuit breaker ni de réémission automatique. Il fournit uniquement une décision déterministe et bornée que les couches supérieures pourront intégrer.
