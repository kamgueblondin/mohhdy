# AOS-961 à AOS-968 — persistance inter-session minimale de la reprise SSE

Le réseau LLM dispose maintenant d’un enregistrement fixe `net_llm_sse_persisted_state_t`. Il contient une version, un fournisseur, le compteur de retries et l’identifiant `Last-Event-ID` borné à `NET_LLM_SSE_EVENT_ID_MAX`. Il ne contient aucun bearer, hôte, chemin HTTP, modèle ou secret.

`net_llm_sse_persist_save` exige un événement valide et un fournisseur connu, puis calcule une empreinte FNV-1a sur la zone persistée. `net_llm_sse_persist_load` vérifie le magic, la version, la longueur, le fournisseur et l’empreinte avant de restaurer l’ID dans les buffers SSE fournis par l’appelant.

La persistance est volontairement neutre vis-à-vis du support : le caller peut la placer dans une zone NVRAM, un fichier de boot ou une partition dédiée. Le code réseau ne réalise aucune écriture disque et ne persiste jamais de secret en clair. Une corruption retourne une erreur sans publier d’état restauré.

Le test couvre le round-trip provider/retry/ID et la détection d’un octet corrompu. Validation locale : **412/412 tests verts**.

Auteur : **Manus AI**
