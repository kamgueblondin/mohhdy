# AOS-385 à AOS-392 — requête LLM HTTPS unifiée via NE2000

`ne2k_https_llm_request` réunit la construction JSON, le framing HTTP POST, le chiffrement TLS et l’émission NE2000 dans un seul appel après un handshake TLS 1.2 complet. L’appelant fournit l’intégralité des buffers : JSON, requête HTTP, record TLS, trame Ethernet ainsi que les états TCP, TLS et ARP.

| Fournisseur | Body construit | Bearer |
|---|---|---|
| `NE2K_LLM_PROVIDER_OLLAMA` | `model` et `prompt`, non-streaming | Optionnel |
| `NE2K_LLM_PROVIDER_OPENAI` | `model` et un message utilisateur, non-streaming | Obligatoire et non vide |

La façade valide d’abord le fournisseur et la session TLS achevée. Elle construit ensuite le JSON borné, sélectionne le POST HTTP avec ou sans header `Authorization: Bearer`, chiffre le tout en AES-128-GCM et n’avance les séquences TCP/TLS qu’après émission réussie. Une erreur postérieure à la construction restaure la connexion TCP et le compteur de séquence TLS.

`ne2k_https_llm_poll_text` enveloppe le polling HTTP chiffré déjà fragmenté. Il retourne `1` lorsque le body n’est pas complet, refuse les statuts hors `2xx`, puis délègue l’extraction du champ de réponse au format Ollama ou OpenAI. Le texte et sa longueur sont caller-owned.

> Cette intégration ne réalise ni DNS, ni SYN, ni handshake TLS de manière autonome. Elle ne prend en charge que les réponses HTTP `Content-Length` déjà supportées par le polling sous-jacent ; la gestion structurée des statuts 4xx/5xx, le retry borné, SSE, Unicode JSON, tools, messages multiples et les tests contre un serveur externe restent à implémenter.

Le test NE2000 initialise une session AES-GCM TLS factice complète, émet un POST OpenAI chiffré, vérifie le JSON, le header Bearer, l’avancement des séquences, l’absence de Bearer OpenAI et le rejet d’un fournisseur invalide.
