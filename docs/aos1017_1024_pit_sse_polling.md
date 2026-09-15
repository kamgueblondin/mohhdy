# AOS-1017 à AOS-1024 — adaptateur PIT pour le polling SSE

L’API `ne2k_llm_connection_poll_sse_or_resume_now` fournit une façade non bloquante au poller SSE. Elle lit le tick courant via `timer_get_ticks()` puis délègue à l’orchestrateur existant, qui conserve la logique de reprise, les buffers et les séquences caller-owned.

Le timer n’exécute aucun traitement réseau dans l’IRQ0 : l’IRQ ne fait qu’incrémenter le compteur matériel et l’appelant effectue le polling dans son contexte normal. La référence vers `timer_get_ticks` est faible afin que les tests unitaires du module NE2000 restent autonomes ; dans le noyau i386, l’implémentation PIT réelle est utilisée lorsqu’elle est liée.

Cette façade ne conserve ni pointeur d’endpoint, ni secret, ni buffer, et n’introduit aucune allocation dynamique. En cas de timer absent dans un environnement de test, le tick déterministe `0` est utilisé.

Auteur : **Manus AI**

## Validation

La suite locale `make test-all` reste à **413/413 tests verts**. Le contrôle `git diff --check` est propre.

## Intégration

L’appel recommandé depuis le poller réseau est `ne2k_llm_connection_poll_sse_or_resume_now(...)`. Le chemin reste non bloquant : il retourne immédiatement lorsque le délai de reprise n’est pas atteint.

> Le timer matériel fournit l’horloge ; il ne devient pas propriétaire de la machine d’état SSE.

Références : [`kernel/ne2k.h`](../kernel/ne2k.h), [`kernel/ne2k.c`](../kernel/ne2k.c), [`kernel/timer.h`](../kernel/timer.h).

Tableau de comportement :

| Contexte | Source du tick | Traitement réseau dans IRQ | Allocation |
|---|---|---:|---:|
| Noyau i386 | PIT via `timer_get_ticks()` | Non | Aucune |
| Tests unitaires | Valeur de repli `0` si symbole absent | Non | Aucune |
| Appel direct existant | Tick fourni par l’appelant | Non | Aucune |

## Non-régression

Le wrapper délègue à `ne2k_llm_connection_poll_sse_or_resume` sans recopier les structures. Les erreurs, le budget de reconnexion, le fournisseur actif et l’identifiant SSE suivent donc exactement le chemin déjà validé.

Auteur : **Manus AI**

