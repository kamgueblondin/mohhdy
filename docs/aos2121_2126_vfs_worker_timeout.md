# AOS-2121…2126 — Expiration d’un worker VFS vivant mais bloqué

**Statut : livré localement, validation complète en cours.** Ce macro-lot traite une panne différente de la disparition du worker : `vfsvirtual` reste publié dans le registre mais ne traite plus sa boîte IPC, par exemple parce qu’il est suspendu. Le médiateur ne doit pas conserver indéfiniment la transaction privée ni laisser le client attendre jusqu’à l’épuisement de son propre budget.

> Une transaction privée active expire après **huit tours coopératifs du médiateur** sans réponse du worker. Elle est alors terminée par la même réponse locale corrélée que le repli après retrait du service.

## Budget de terminaison

| Mécanisme | Condition | Décision |
|---|---|---|
| Retrait ou remplacement du PID publié | Le registre ne retourne plus le PID mémorisé par la transaction. | Repli local immédiat au tour courant ; `recoveries` augmente. |
| Worker toujours publié mais silencieux | Huit passages coopératifs de `vfsserver` sans réponse privée correspondante. | Repli local ; `timeouts` augmente. |
| Réponse privée corrélée reçue avant la borne | PID, type et `request_id` attendus. | Chemin worker normal ; aucun compteur de repli n’augmente. |

Le budget est un compteur saturé dans l’état transactionnel statique. Il ne dépend ni d’une horloge hôte, ni de la fréquence réelle QEMU, ni d’une allocation dynamique. Huit tours restent volontairement inférieurs aux 24 tours que le shell réserve aux réponses de lecture VFS : le médiateur peut donc répondre localement avant que le client n’abandonne.

## Observabilité

La vue publique existante `vfs-read vfs-worker` complète désormais son instantané :

```text
vfsvirtual ready pid=<PID> recoveries=<N> timeouts=<M>
```

`timeouts` est volatile et augmente seulement lorsque le worker était encore publié mais silencieux au-delà du budget. `recoveries` continue de distinguer le retrait ou le remplacement du PID. Les deux compteurs sont remis à zéro au démarrage de `vfsserver` et ne constituent ni un journal persistant ni une métrique atomique.

## Contrat QEMU

Le contrat `make qemu-vfs-service` suspend explicitement le worker tout en conservant son service publié, lance le client indépendant `vfsflight`, puis laisse le médiateur franchir le budget local. Il exige les marqueurs suivants :

| Preuve | Signification |
|---|---|
| `vfsserver delegated vfs-info` | La transaction privée a bien été soumise. |
| `vfsserver virtual worker timeout local` | Le budget de huit tours a expiré malgré un PID toujours publié. |
| `vfsflight local reply ok` | La réponse locale corrélée atteint le client en attente. |
| `ready pid=<PID> recoveries=0 timeouts=1` | Le worker est toujours enregistré et seul le compteur de silence a augmenté. |
| `task-resume` puis nouvelle lecture déléguée | La reprise du worker permet de revenir au chemin IPC normal. |

## Limites explicites

Cette expiration ne tue pas, ne suspend pas et ne redémarre pas le worker. Elle ne vide pas son message privé potentiellement encore en attente : si le worker reprend tardivement, le médiateur reconnaît le PID/type mais écarte la réponse dont le `request_id` ne correspond plus à la transaction active. Elle ne peut donc pas devenir une erreur pour le client suivant. Le mécanisme fournit une terminaison publique locale, pas un protocole d’annulation distribué ou une supervision de processus.

Les backends ATA, FAT16, FAT32 et overlay restent noyau. Aucune capability, ABI publique ou allocation dynamique n’est ajoutée.

## Validation

```bash
make -s -C userspace vfsserver
make -s qemu-vfs-service
```

Le contrat QEMU complet passe avec l’expiration du worker suspendu, les récupérations après retrait, la vue de santé et les scénarios VFS historiques.

## Références internes

- [Médiateur VFS](../userspace/vfs_server.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
- [Santé observable AOS-2115…2120](aos2115_2120_vfs_worker_health.md)
- [Récupération en vol AOS-2109…2114](aos2109_2114_vfs_worker_inflight_recovery.md)

---

**Auteur : Manus AI**

**Date de validation : 22 août 2026**
