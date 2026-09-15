# AOS-913 à AOS-920 — émission NE2000/TLS du GET SSE de reprise

Le chemin `ne2k_https_llm_sse_resume_request` relie désormais l’état SSE aux primitives matérielles. Il exige un `Last-Event-ID` valide, réutilise le builder HTTP borné, chiffre le GET dans la session TLS AES-GCM, prépare le payload TCP caller-owned, l’émet par NE2000 et committe la séquence uniquement après confirmation de transmission.

La fonction est transactionnelle : une capacité insuffisante, un état TLS incomplet, un échec de suivi TCP ou une erreur TX restaure la connexion et la séquence d’écriture TLS. L’identifiant et les buffers restent détenus par l’appelant ; aucun `kmalloc`, secret global ou stockage caché n’est utilisé.

Le test NE2000 couvre le rejet d’un événement sans ID et le rejet d’une session TLS non terminée. Le prochain macro-lot raccordera explicitement `net_llm_sse_reconnect_schedule` et `net_llm_sse_reconnect_ready` au poller temporel afin d’émettre ce GET lorsque le délai `retry:` est arrivé à échéance.

Auteur : **Manus AI**
