# AOS-1225 à AOS-1240 — syscalls socket TCP utilisateur

> **État :** implémenté sur `master` local, validation globale **418/418 tests verts**.

Le noyau expose désormais six numéros syscall pour piloter le registre TCP statique : `SYS_SOCKET_OPEN`, `SYS_SOCKET_ACCEPT_SYN_ACK`, `SYS_SOCKET_SEND`, `SYS_SOCKET_FEED`, `SYS_SOCKET_RECEIVE` et `SYS_SOCKET_CLOSE`. Les structures de requête sont des POD définies dans `include/os_syscalls.h`; les buffers restent fournis par l’appelant et aucune mémoire n’est allouée par le chemin syscall.

| Syscall | Entrée principale | Résultat |
|---|---|---|
| `SYS_SOCKET_OPEN` | ports local/distant et séquence initiale | identifiant de slot ou erreur bornée |
| `SYS_SOCKET_ACCEPT_SYN_ACK` | descripteur et vue SYN-ACK | transition vers `ESTABLISHED` |
| `SYS_SOCKET_SEND` | requête payload/segment caller-owned | segment TCP construit et longueur produite |
| `SYS_SOCKET_FEED` | segment TCP caller-owned | payload accepté dans la file RX statique |
| `SYS_SOCKET_RECEIVE` | buffer caller-owned | nombre d’octets copiés |
| `SYS_SOCKET_CLOSE` | descripteur | libération du slot |

Les wrappers refusent les appels provenant d’une tâche qui n’est pas une tâche utilisateur et rejettent les pointeurs nuls. La validation détaillée des plages d’adresses utilisateur reste alignée sur le mécanisme de vérification mémoire déjà utilisé par les syscalls du dépôt ; la prochaine durcissement devra ajouter une validation de fenêtre avant toute copie longue.

Le macro-lot ne promet pas encore l’écoute passive, la résolution DNS, l’émission NIC automatique ni la migration TLS/HTTP vers cette API générique. Ces étapes sont conservées dans le backlog afin de ne pas modifier le chemin LLM authentifié sans tests de compatibilité.

**Auteur :** Manus AI
