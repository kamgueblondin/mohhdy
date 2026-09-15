# AOS-2133…2138 — Pages observées `vfs-mounts` formatées par worker Ring 3

**Statut : livré localement, validation complète en cours.** Ce macro-lot complète la délégation des pages ordinaires de montages en traitant `vfs-list-observe vfs-mounts <départ> <génération-attendue>`. Le worker `vfsvirtual` formate les lignes sélectionnées ; le médiateur reste seul responsable de la validité de génération, de l’obsolescence, des limites publiques et de la réponse corrélée.

> La génération reste une politique de `vfsserver`. Elle est contrôlée avant toute soumission au worker et publiée par le médiateur ; le worker ne reçoit que le préfixe et le bit lecture/écriture d’une ligne.

## Déroulement

| Étape | Responsable | Garantie |
|---|---|---|
| Parse de `start` et de la génération attendue | Médiateur | ABI publique inchangée |
| Détection d’une génération obsolète | Médiateur | Réponse `stale` immédiate, sans IPC worker |
| Choix de chaque montage | Médiateur | Table locale et index détenus par `vfsserver` |
| Formatage `prefixe ro|rw` | Worker Ring 3 | IPC privé `OS_IPC_VFS_WORKER_MOUNT` réutilisé |
| Agrégation, limite de 4 entrées et 64 octets | Médiateur | Réponse `LIST_OBSERVE` bornée |
| `next_start`, statut et génération publiée | Médiateur | Valeurs publiques corrélées au client |

La réponse observée conserve ses 64 octets de données. Les quatre échanges privés séquentiels sont couverts par le budget shell de 24 tours déjà appliqué aux autres vues VFS worker. Aucun message IPC, structure ABI ou droit backend nouveau n’est introduit.

## Repli et résilience

Une soumission impossible, la disparition du worker, son expiration après huit tours ou un échec d’envoi de ligne réutilisent le générateur local de page observée. Le client reçoit alors une `OS_IPC_VFS_LIST_OBSERVE_REPLY` complète avec la génération courante du médiateur. Les réponses worker tardives à `request_id` discordant restent écartées et ne peuvent pas terminer la transaction suivante.

## Contrat QEMU

La campagne `make qemu-vfs-service` vérifie :

| Cas | Preuve exigée |
|---|---|
| Page observée de départ 0, génération 0 | `vfsserver delegated mount observe`, quatre `vfsvirtual format mount`, réponse `partiel count 4 next 4 generation` |
| Même page avec génération obsolète | `vfs-list-observe obsolete generation` ; le médiateur ne délègue pas cette décision |
| Scénarios worker existants | Retrait, redécouverte, transaction interrompue, worker silencieux puis réponse tardive, et reprise normale |

## Limites explicites

La génération est volatile et l’instantané n’est pas atomique. Une mutation entre la vérification initiale et la réponse peut changer la table : le protocole annonce une génération médiateur, sans réservation ni verrou global. Le worker ne possède aucune capacité de stockage, ne voit pas les backends ATA/FAT/overlay et n’interprète jamais une demande publique brute.

## Validation

```bash
make -s -C userspace vfsserver shell vfsvirtual
make -s qemu-vfs-service
```

Le contrat QEMU passe avec la page observée déléguée et le rejet local de la génération stale.

## Références internes

- [Médiateur VFS](../userspace/vfs_server.c)
- [Worker virtuel Ring 3](../userspace/vfs_virtual_worker.c)
- [Shell Ring 3](../userspace/shell.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Pages ordinaires AOS-2127…2132](aos2127_2132_vfs_mount_page_worker.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
