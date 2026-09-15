# AOS-929 à AOS-936 — orchestration SSE NE2000 : poll ou reprise

`ne2k_llm_connection_poll_sse_or_resume` fournit le point d’entrée unique du chemin événementiel. Pour une session `REQUEST_SENT` ou `STREAMING`, il délègue au polling SSE existant. Pour une session `TLS_COMPLETE`, il consulte la deadline caller-owned du scheduler et retourne immédiatement si le tick n’est pas atteint ; lorsque le tick est arrivé, il émet le GET TLS/NE2000 avec `Last-Event-ID` et repasse en `REQUEST_SENT`.

Cette orchestration ne dort jamais et ne crée aucun timer global. Les buffers RX, TX, HTTP, TLS et texte sont fournis par l’appelant. Le chemin « pas encore prêt » ne modifie pas les longueurs publiées et permet au scheduler IRQ ou à une boucle coopérative de rappeler la fonction au tick suivant.

Le test couvre le retour non bloquant avant échéance. La suite complète doit conserver la non-régression i386, les 408 tests existants et les smokes QEMU.

Auteur : **Manus AI**
