# AOS-393 à AOS-400 — erreurs HTTP LLM et retry borné

La couche HTTP/TLS expose désormais une politique déterministe pour les réponses LLM. `net_llm_http_status_classify` ne conserve aucun état et classe un code HTTP dans les catégories suivantes.

| Catégorie | Codes | Décision |
|---|---:|---|
| Succès | 200–299 | Lire et extraire la réponse du fournisseur. |
| Authentification | 401, 403 | Ne pas réessayer ; renouveler ou corriger l’identifiant hors pile. |
| Retryable | 408, 425, 429, 500–599 | Une nouvelle émission peut être demandée dans la limite caller-owned. |
| Permanent | 300–499 restants | Ne pas réessayer automatiquement. |
| Protocole | Hors intervalle HTTP | Rejeter la réponse. |

`net_llm_http_retry_consume(status, retry_limit, &retries_used)` autorise une relance seulement pour la catégorie retryable et seulement lorsque `retries_used < retry_limit`. Un retour `1` incrémente le compteur fourni par l’appelant ; `0` signifie qu’aucune relance n’est autorisée ; une entrée de compteur absente est rejetée.

> La fonction n’attend pas, ne dort pas, ne lit pas `Retry-After`, ne réémet pas un POST d’elle-même et ne modifie aucun état TCP/TLS. L’appelant décide du délai puis crée une nouvelle requête via l’API NE2000. Cette séparation évite de masquer le caractère potentiellement non idempotent d’un POST LLM.

Les tests couvrent les classes succès, authentification, permanent, protocole et retryable, puis vérifient l’arrêt strict après deux tentatives et le rejet d’un compteur nul.
