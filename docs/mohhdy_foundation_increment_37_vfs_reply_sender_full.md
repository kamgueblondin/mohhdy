# Lot Foundation 37 — Émetteur de toutes les réponses VFS corrélées

## Objet

Ce lot étend le contrôle d’émetteur livré pour les capacités backend à l’ensemble des commandes VFS corrélées du shell. Chaque commande résout le PID public de `vfs` avant l’envoi, puis n’analyse une réponse différée ou directe que si son `sender_pid` correspond à ce PID.

| Famille couverte | Commandes |
|---|---|
| Fichiers | `vfs-read`, `vfs-write`, `vfs-remove`, `vfs-rename`, `vfs-stat` |
| Répertoires | `vfs-mkdir`, `vfs-rmdir`, `vfs-list`, `vfs-list-page`, `vfs-list-observe` |
| Montages | `vfs-mount-add`, `vfs-mount-remove` |
| Capacités backend | octroi complet ou scoped, révocation, statut, inventaire, observation |

Un message du bon type et du bon `request_id` issu d’un autre PID ne peut donc pas être décodé comme une réponse VFS valide. Le helper des montages reçoit explicitement le PID attendu ; tous les autres chemins de réception appliquent le même contrôle avant le codec de réponse.

> Le mécanisme vérifie l’origine locale déclarée par le noyau IPC. Il n’est pas une identité cryptographique, une capability non forgeable, ni une protection contre un noyau compromis.

## Vérification

La compilation Ring 3 et `make test-all` sont verts avec **217/217** tests. Le contrat QEMU VFS complet termine avec `rc ok 0` après les opérations de lecture, écriture, métadonnées, listage, pagination, observation, montages, capacité backend, transfert et purge.

Le lot ne modifie ni l’ABI, ni le protocole IPC, ni les droits backend. Les réponses non corrélées et les notifications best-effort conservent leur sémantique existante.
