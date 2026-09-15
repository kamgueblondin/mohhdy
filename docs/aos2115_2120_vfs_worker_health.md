# AOS-2115…2120 — Santé observable du worker VFS

**Statut : livré localement, validation complète en cours.** Ce macro-lot ajoute une vue virtuelle publique, `vfs-worker`, qui rend observable l’état du worker `vfsvirtual` depuis le médiateur VFS. Il s’appuie exclusivement sur le registre de services déjà utilisé pour déléguer les vues virtuelles et sur un compteur statique de récupérations effectivement réalisées.

> La vue de santé décrit ce que le médiateur peut observer au moment de la lecture ; elle ne transforme pas `vfsserver` en parent du worker et ne promet aucun redémarrage automatique.

## Interface publique

La commande `vfs-read vfs-worker` est traitée localement par `vfsserver` et produit une seule ligne de taille bornée.

| État du registre `vfs-virtual` | Réponse publique | Interprétation |
|---|---|---|
| Worker publié avec PID positif | `vfsvirtual ready pid=<PID> recoveries=<N>` | Le PID est résolu au moment de la lecture. |
| Service absent, purgé ou PID invalide | `vfsvirtual missing recoveries=<N>` | Aucun worker ne peut recevoir une nouvelle délégation. |

`recoveries` est un compteur 32 bits volatil, remis à zéro au démarrage de `vfsserver`. Il augmente seulement lorsqu’une transaction privée active est terminée par le mécanisme de repli local parce que le PID publié n’est plus celui mémorisé par la transaction. Une simple lecture locale due à l’absence du worker avant soumission ne l’incrémente pas.

## Construction sans allocation

Le médiateur réemploie le générateur local déjà borné pour composer la ligne de santé. Le PID provient de `service_lookup("vfs-virtual")` et le compteur est une variable statique. Ni la charge IPC de 96 octets, ni le protocole de réponses publiques, ni les permissions backend ne changent.

La lecture de santé reste volontairement locale. Elle ne réserve pas le PID lu, n’empêche pas une purge immédiatement postérieure et ne répare pas une transaction. Elle est un instantané d’observabilité destiné à accompagner les mécanismes de repli AOS-2103…2114.

## Contrat QEMU

La campagne `make qemu-vfs-service` vérifie successivement les cinq observations suivantes dans une image i386 réelle :

| Étape | Valeur exigée |
|---|---|
| Worker initial publié | `ready`, PID courant, `recoveries=0` |
| Worker arrêté hors transaction | `missing`, `recoveries=0` |
| Worker relancé après arrêt hors transaction | nouveau PID `ready`, `recoveries=0` |
| Worker arrêté pendant la requête de `vfsflight` | `missing`, `recoveries=1` après réponse locale corrélée |
| Worker relancé après récupération en vol | nouveau PID `ready`, `recoveries=1` |

Le contrat vérifie donc le registre, la réponse locale corrélée et la persistance du compteur dans le même `vfsserver`, tout en confirmant que la délégation normale reprend avec le PID ultérieur.

## Limites explicites

Le shell reste parent direct de `vfsvirtual` dans les scénarios QEMU actuels. Les syscalls de supervision d’enfants directs ne peuvent donc pas être attribués honnêtement au médiateur. La vue `vfs-worker` n’est ni un superviseur, ni un watchdog, ni un lanceur de processus. Elle ne fournit pas de journal persistant, de notification push, de protection contre l’enroulement 32 bits ni de garantie atomique entre la recherche du PID et la lecture affichée.

Les pilotes ATA, FAT16, FAT32 et l’overlay restent noyau ; aucune capability ni allocation dynamique n’est introduite.

## Validation

```bash
make -s -C userspace vfsserver
make -s qemu-vfs-service
```

Le contrat QEMU passe avec les cinq assertions de santé, en plus des tests VFS de capacités, montages, FAT16, FAT32 et mutations overlay déjà présents.

## Références internes

- [Médiateur VFS et vue `vfs-worker`](../userspace/vfs_server.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Récupération en vol AOS-2109…2114](aos2109_2114_vfs_worker_inflight_recovery.md)
- [Résilience au retrait AOS-2103…2108](aos2103_2108_vfs_worker_resilience.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
