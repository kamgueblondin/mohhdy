# AOS-115 — File NIC RX/TX statique

## Objectif

Le lot AOS-115 ajoute le contrat mémoire entre un futur pilote de carte réseau et la pile Ethernet/ARP. La file est circulaire, de capacité fixe et entièrement alimentée par des buffers fournis par l’appelant.

## Contrat

`net_nic_queue_init` reçoit une zone de stockage contiguë pour huit trames. Chaque emplacement est borné par `frame_capacity`, avec une limite maximale de 1536 octets adaptée à une trame Ethernet standard avec marge de traitement. Aucune fonction ne réalise d’allocation dynamique.

| Opération | Garantie |
|---|---|
| `acquire` | Retourne le prochain emplacement libre sans copier de données. |
| `commit` | Publie une longueur vérifiée et avance l’index producteur. |
| `pop` | Retourne la vue de la plus ancienne trame publiée et avance l’index consommateur. |
| `reset` | Vide la file en O(1) et conserve les buffers de l’appelant. |
| `count` | Retourne le nombre d’emplacements publiés, borné à 8. |

La séparation `acquire`/`commit` permet au pilote RX de remplir directement un buffer et au pilote TX de préparer une trame sans copie intermédiaire. Une file pleine ou une longueur supérieure à la capacité sont rejetées.

## Validation

Le test `tests/unit/kernel/test_net_nic.c` couvre l’initialisation, la propriété caller-owned des buffers, l’ordre FIFO, la saturation logique et le reset. Le runner autonome lie explicitement `kernel/net_nic.c` au test.

La validation locale du groupe est :

```text
make test-all                  270 tests, 270 passés, 0 échec, 0 ignoré
make all                       build i386 et initrd réussis
```

Cette file ne détecte ni ne programme encore un contrôleur PCI/NIC. Elle constitue le contrat stable nécessaire avant l’implémentation du pilote matériel et de ses opérations RX/TX.
