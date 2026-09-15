# AOS-1521 à AOS-1528 — activation contrôlée du client OpenAI dans le shell

## Objet

Le noyau savait déjà stocker un bearer OpenAI borné et le chemin HTTPS pouvait déjà construire un `POST` Chat Completions ou un flux SSE. Cependant, le shell ne proposait pas de wrapper pour le syscall de provisionnement et affichait encore des diagnostics hérités indiquant que le réseau, TLS et OpenAI étaient indisponibles.

Ce lot ferme cette rupture d’interface. Il expose la commande explicite `ai-credential <bearer>`, aligne `net-status` sur les capacités réellement livrées et remplace le message historique de la commande conversationnelle `ai` lorsque le fournisseur OpenAI est sélectionné.

## Parcours d’utilisation

| Étape | Commande | Effet |
|---|---|---|
| 1 | `ai-provider openai` | sélection locale du profil OpenAI |
| 2 | `ai-credential <bearer>` | copie le bearer borné dans le noyau, sans getter |
| 3 | `ai-acquire api.openai.com` | démarre DHCP, DNS, ARP et SYN vers HTTPS |
| 4 | `ai-tls-poll` | avance le handshake TLS authentifié jusqu’à disponibilité applicative |
| 5 | `ai-request openai <modele> /v1/chat/completions <prompt>` | émet un POST Chat Completions chiffré |
| 5S | `ai-stream-request openai <modele> /v1/chat/completions <prompt>` | émet un POST SSE chiffré |
| 6 | `ai-text-poll` ou `ai-sse-poll` | lit une réponse ou les deltas streamés |

La commande conversationnelle `ai <question>` ne déclenche pas automatiquement ce parcours. Cela évite d’émettre une requête réseau, de choisir un endpoint ou d’utiliser un secret sans action explicite de l’utilisateur.

## Propriété et effacement du secret

Le wrapper `sys_llm_configure_openai` passe une structure POD bornée au syscall `SYS_LLM_OPENAI_CREDENTIAL`. Après le retour, `cmd_ai_credential` efface sa copie locale octet par octet. Le noyau reste l’unique détenteur du bearer provisionné et ne fournit aucun syscall de lecture.

Le shell masque la commande dans son historique sous la forme `ai-credential [masque]`. Le token n’est donc ni imprimé par la commande, ni ajouté à l’historique, ni inscrit dans les tests, les images de boot ou cette documentation.

> Le test réel externe exécuté lors du cadrage a été arrêté au `GET /v1/models` : la clé de test disponible a reçu HTTP 401 `invalid_api_key`. Aucun credential n’a été conservé. La campagne réelle devra être relancée avec une clé OpenAI valide fournie hors CI publique.

## Diagnostic réseau

`net-status` ne présente plus la pile comme un stub absent. Avec une carte NE2000 détectée, son format JSON signale les capacités `dhcp`, `socket`, `authenticated` et `credential-required`. Sans matériel réseau, chaque couche est explicitement `unavailable`; cela ne simule pas une connexion prête.

## Validation

Le smoke QEMU `test_ai_provider_commands.py` compile le noyau et le shell, provisionne une valeur de test sans la rendre visible dans le log attendu, vérifie que l’historique est masqué, couvre les diagnostics sans NE2000 et confirme le guidage OpenAI. La validation complète reste obligatoire avant publication.

Aucune allocation dynamique, aucun bearer en clair dans les artefacts de test et aucun travail réseau supplémentaire en IRQ0 ne sont introduits.

## Références

1. [OpenAI API Overview](https://developers.openai.com/api/reference/overview) : authentification HTTP Bearer et informations de requête.
2. [Chat Completions streaming events](https://developers.openai.com/api/reference/resources/chat/subresources/completions/streaming-events) : flux SSE des chunks `choices[].delta.content`.
