# AOS-1097 à AOS-1112 — provisionnement OpenAI sécurisé

Le noyau MOHHDY expose désormais `SYS_LLM_OPENAI_CREDENTIAL` pour provisionner un bearer OpenAI dans un buffer fixe du noyau. La requête POD est bornée à `OS_LLM_BEARER_MAX` octets et le token est validé comme chaîne ASCII terminée ; il n’est jamais renvoyé dans un résultat syscall ni imprimé par le shell.

Le bearer précédent est effacé avant tout remplacement. Le provisionnement est accepté uniquement lorsque la session LLM est inactive, ce qui empêche de modifier les identifiants pendant une connexion TLS ou une requête en cours. Les requêtes Ollama continuent d’utiliser un chemin sans bearer ; les requêtes OpenAI refusent proprement l’envoi si aucun credential n’a été provisionné.

La fermeture de session efface le buffer credential avec la même primitive de nettoyage que les clés TLS et les buffers HTTP. Le code reste freestanding, sans `kmalloc`, sans copie vers un buffer dynamique et sans modification du protocole TLS.

> Le credential est un secret de session noyau : il doit être injecté avant l’acquisition réseau et disparaît à la fermeture de la session.

| Élément | Garantie |
|---|---|
| API | `SYS_LLM_OPENAI_CREDENTIAL` |
| Taille | `OS_LLM_BEARER_MAX` octets maximum |
| Validation | ASCII, terminaison obligatoire |
| Moment d’écriture | Session LLM inactive uniquement |
| Utilisation | OpenAI seulement, jamais Ollama |
| Effacement | Remplacement et `ai-close` |
| Allocation | Aucune |

Validation locale : **415/415 tests verts**, build i386 propre et contrôle de diff propre.

Ce lot rend le chemin OpenAI provisionnable au niveau noyau. L’interface graphique ou le shell doit fournir le secret par un canal de saisie protégé ; le noyau ne propose volontairement aucune commande qui l’affiche ou le persiste en clair.

Auteur : **Manus AI**

