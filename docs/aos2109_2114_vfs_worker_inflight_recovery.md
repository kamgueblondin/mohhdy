# AOS-2109…2114 — Récupération VFS après disparition du worker en vol

**Statut : livré localement, validation complète en cours.** Ce macro-lot complète la résilience de cycle de vie du worker `vfsvirtual`. Il traite désormais le cas où le médiateur `vfsserver` a déjà soumis une requête IPC privée mais où le worker disparaît avant de répondre.

> Une requête publique ne demeure pas bloquée sur un worker disparu : au prochain tour coopératif du médiateur, l’absence ou le remplacement du PID publié termine la transaction par une réponse locale corrélée.

## Modèle de récupération

| État observé par `vfsserver` | Action bornée | Réponse au client |
|---|---|---|
| PID publié identique au PID de la transaction | Le médiateur conserve l’attente de la réponse privée. | Aucune réponse anticipée. |
| Service retiré, absent ou republié par un autre PID | Le médiateur reconstruit la vue locale, envoie la réponse avec le `request_id` d’origine, puis réinitialise l’état. | Lecture virtuelle réussie et corrélée. |
| Échec d’envoi de la prochaine ligne `vfs-mounts` au worker | Le médiateur abandonne l’assemblage privé et reconstruit la vue locale complète. | Lecture virtuelle locale cohérente avec l’état courant. |
| Réponse privée déjà reçue avant la purge | Le chemin normal corrélé gagne. | Réponse produite par le worker ; aucune seconde réponse locale. |

La structure statique `vfs_virtual_pending` enregistre désormais la **vue logique** (`vfs-info`, `vfs-stats` ou `vfs-mounts`) en plus du PID, du client et de l’identifiant corrélé. Cette information suffit à reconstruire la réponse depuis le générateur local existant. Aucun buffer dynamique, retry automatique, nouvelle capacité ou nouvelle ABI IPC n’est ajouté.

## Garantie de terminaison

La détection ne dépend ni d’un timeout hôte ni d’un compteur de ticks : elle intervient lors du prochain passage dans la boucle coopérative de `vfsserver`. Le retrait d’une tâche purge déjà son entrée `vfs-virtual` du registre ; le médiateur compare ensuite la publication courante au PID mémorisé. Le coût est constant, l’état temporaire reste unique, et toute récupération termine par `vfs_virtual_reset()`.

Cette propriété est différente d’une reprise transactionnelle distribuée : le worker ne reçoit pas de redémarrage transparent, et une réponse privée perdue n’est pas rejouée. Le service public est cependant terminé localement avec la corrélation originale, ce qui évite de laisser un client attendre un processus mort.

## Contrat QEMU déterministe

Le contrat `make qemu-vfs-service` utilise le nouveau client Ring 3 interne `vfsflight`. Ce client ne détient aucune capacité backend ; il soumet une seule lecture publique `vfs-info` et attend une réponse dont le PID, le type et `request_id` sont vérifiés.

La campagne suspend le worker afin de conserver son message privé dans sa boîte, lance `vfsflight`, laisse le médiateur ouvrir la transaction, puis termine le worker depuis le shell. Après deux passages coopératifs, le contrat vérifie que `vfsflight` reçoit la réponse locale exacte. Il nettoie le client, redémarre `vfsvirtual`, confirme sa republication et valide une nouvelle lecture déléguée.

| Preuve QEMU | Assertion |
|---|---|
| Transaction ouverte | `vfsserver delegated vfs-info` est journalisé. |
| Worker arrêté avant réponse | Le client ne reçoit aucune réponse du worker suspendu. |
| Repli en vol | `vfsserver virtual worker fallback local` puis `vfsflight local reply ok`. |
| Reprise | Nouveau PID publié par `vfs-virtual`, puis `vfsvirtual read vfs-info`. |

## Portée et limites

La récupération couvre les trois vues virtuelles appartenant déjà au worker. `vfs-stats` est régénéré avec les compteurs au moment de la récupération, et `vfs-mounts` avec la table de montages courante : aucune promesse d’instantané historique n’est ajoutée. Si la boîte du client est pleine, l’IPC local reste best-effort comme le reste du médiateur ; la transaction est néanmoins réinitialisée afin de ne pas bloquer les demandes suivantes.

Les backends ATA, FAT16, FAT32 et overlay ne changent pas de privilège et restent noyau. Le worker demeure non supervisé ; ce lot détecte sa disparition, mais ne planifie pas lui-même son redémarrage.

## Validation

```bash
make -s -C userspace vfsserver vfsflight
make -s qemu-vfs-service
```

Le contrat QEMU passe après l’arrêt en vol, le repli local et la relance du worker. Aucune modification ABI ne justifie un nouveau test Unity : la preuve appropriée est la campagne inter-processus qui exerce réellement le registre de services, la boîte IPC et l’ordonnancement coopératif.

## Références internes

- [Médiateur VFS](../userspace/vfs_server.c)
- [Client de contrat en vol](../userspace/vfs_flight_client.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Résilience au retrait et redémarrage AOS-2103…2108](aos2103_2108_vfs_worker_resilience.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
