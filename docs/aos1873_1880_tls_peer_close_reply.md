# AOS-1873…1880 — Réponse `close_notify` après fermeture TLS distante

## Objet

Lorsqu’un poller SSE reçoit puis valide une alerte TLS distante `warning/close_notify`, le segment reçu a déjà été acquitté par le chemin de polling. Ce lot envoie ensuite un `close_notify` local chiffré par le même client NE2000, puis termine la session LLM dans l’état `RESPONSE_READY`.

| Étape | Comportement |
|---|---|
| Réception distante | Le poller HTTP/TLS renvoie `NET_HTTP_TLS_STATUS_CLOSE_NOTIFY`. |
| ACK TCP | Le segment TLS distant est acquitté avant toute réponse locale. |
| Réponse TLS | `ne2k_https_llm_close_notify()` construit et transmet une alerte AES-GCM locale. |
| Fin SSE | La phase devient `NE2K_LLM_CONNECTION_RESPONSE_READY`, sans reconnexion SSE. |

## Sémantique de disponibilité

L’émission locale est **best-effort**. Après une fermeture ordonnée initiée par le pair, le transport peut ne plus être disponible ; l’échec de cette réponse ne réactive donc ni le retry ni la reprise SSE. L’émetteur garde ses garanties transactionnelles propres : en cas d’échec, il restaure la connexion TCP et la séquence d’écriture AES-GCM qu’il avait instantanées avant construction du record.

> La clôture applicative reste terminale même lorsque la réponse TLS ne peut plus atteindre un pair déjà fermé.

Cette politique sépare correctement l’état reçu, déjà engagé et acquitté, de la tentative locale de courtoisie TLS. Elle ne réalise aucune allocation dynamique.

## Validation

Le nouveau vecteur NE2000 initialise un client et un serveur AES-GCM, émet `close_notify` par la façade HTTPS/NE2000, récupère le segment dans la RAM distante simulée, puis le déchiffre côté serveur. Il vérifie le type Alert, la charge utile exacte `warning/close_notify` (`1, 0`) et l’avancement de la séquence d’écriture client.

| Contrôle | Résultat |
|---|---:|
| `make -s test-kernel` | 38/38 suites noyau réussies |
| Record TLS émis | 31 octets, type Alert |
| Payload déchiffré | `1, 0` |
| Allocation dynamique | Aucune |

## Référence

[1] [RFC 5246 — TLS 1.2, §7.2.1](https://www.rfc-editor.org/rfc/rfc5246#section-7.2.1)
