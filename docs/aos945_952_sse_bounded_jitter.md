# AOS-945 à AOS-952 — jitter SSE borné et tick caller-owned

Le scheduler de reconnexion SSE dispose maintenant d’une variante jitterisée. `net_llm_sse_reconnect_schedule_jittered` utilise une graine fournie par l’appelant et une LCG déterministe pour produire un décalage symétrique autour du délai de base. Le délai final reste strictement positif et ne dépasse jamais `max_delay`.

La fonction réutilise le budget de retry, la classification HTTP et la réinitialisation transactionnelle de l’accumulateur. Elle ne dort pas et ne manipule aucun état global. Le poller peut fournir `timer_get_ticks()` comme instant courant ; l’IRQ ne fait qu’incrémenter le compteur et ne lance aucune reconnexion directement.

Le test vérifie la fenêtre de délai, la programmation du statut 503 et le rejet d’un délai de base nul. Le prochain axe concerne la persistance ou la reprise multi-fournisseur après épuisement du budget.

Auteur : **Manus AI**
