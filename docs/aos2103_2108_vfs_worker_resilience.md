# AOS-2103…2108 — Résilience du worker VFS Ring 3

**Statut : livré localement, en validation de non-régression.** Ce macro-lot vérifie et durcit le comportement du médiateur VFS lorsque le processus `vfsvirtual` disparaît puis est relancé. La délégation reste une optimisation d’isolation pour les vues virtuelles ; elle ne doit pas rendre ces vues indisponibles.

> Le médiateur ne met pas en cache le PID du worker entre deux requêtes. Il redécouvre le service `vfs-virtual` au départ de chaque transaction et n’accepte qu’un PID Ring 3 strictement positif.

## Comportement livré

| Situation | Décision de `vfsserver` | Résultat client |
|---|---|---|
| Worker publié et disponible | Délégation IPC corrélée. | Vue formatée par `vfsvirtual`. |
| Worker absent ou PID de service non positif | La soumission privée échoue avant toute transaction active. | Générateur local historique de `vfs-info`, `vfs-stats` ou `vfs-mounts`. |
| Worker relancé et service republié | Nouvelle recherche de service à la requête suivante. | Délégation automatiquement rétablie. |
| Worker meurt pendant une transaction déjà soumise | Hors périmètre de cette livraison. | La transaction demeure bornée mais n’est pas rejouée. |

## Correction de structure

`vfs_virtual_lookup()` centralise la recherche de service et normalise tout PID nul, négatif ou absent en échec de découverte. Les trois voies de délégation existantes (`vfs-info`, `vfs-stats`, `vfs-mounts`) conservent leur repli local parce qu’elles ne créent l’état transactionnel qu’après une découverte et un envoi valides.

Aucune allocation dynamique, capacité backend, mémoire partagée ou privilège de stockage n’est ajouté.

## Contrat QEMU

Le contrat `make qemu-vfs-service` exécute désormais le cycle suivant sur une vraie séquence de tâches Ring 3 :

1. lancement de `vfsvirtual` et vérification de sa publication ;
2. vérification des vues VFS déléguées ;
3. arrêt explicite du worker et vérification que `service-find vfs-virtual` devient indisponible ;
4. lecture de `vfs-info` avec trace de repli local du médiateur ;
5. relance de `vfsvirtual`, découverte de son nouveau PID et lecture à nouveau déléguée.

Cette campagne QEMU valide le chemin de vie de processus et le registre de services, au-delà de la seule simulation unitaire.

## Validation

```bash
make -s -C userspace vfsserver vfsvirtual
make -s qemu-vfs-service
```

Le contrat de service VFS local passe après le retrait et la relance du worker. La suite complète reste attendue sans changement de nombre de tests Unity, car cette livraison renforce la couverture QEMU inter-processus et non l’ABI sérialisée déjà couverte.

## Limites explicites

La reprise ne couvre pas le décès du worker au milieu d’une transaction en vol : aucune file persistante ni mécanisme de retry transparent n’est introduit. Les backends ATA, FAT16, FAT32 et overlay restent noyau. Le macro-lot garantit le repli avant soumission et la redécouverte à la requête suivante ; il ne transforme pas encore le worker en service supervisé.

## Références internes

- [Médiateur VFS Ring 3](../userspace/vfs_server.c)
- [Worker VFS Ring 3](../userspace/vfs_virtual_worker.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [État réel](ETAT_REEL.md)
- [Délégation de montages AOS-2095…2102](aos2095_2102_vfs_ring3_mounts_worker.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
