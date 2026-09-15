# AOS-117 — Sonde NE2000 pour QEMU

## Objectif

Le lot AOS-117 ajoute une sonde NE2000 minimale destinée au contrôleur `ne2k_isa` de QEMU. Le pilote reçoit une interface d’E/S injectable, ce qui permet de vérifier le protocole sans effectuer d’accès matériel pendant les tests Unity.

## Contrat

`ne2k_probe` positionne le contrôleur en mode arrêt, déclenche la séquence reset et vérifie l’acquittement du registre ISR. `ne2k_prepare` programme le mode mot et efface les interruptions latentes. L’état du périphérique est conservé dans une structure caller-owned; aucune allocation dynamique n’est utilisée.

Cette version ne lit pas encore la PROM MAC, ne programme pas l’anneau RX/TX et n’émet aucune trame. Elle établit le point d’intégration matériel vérifiable avant le raccordement des buffers `net_nic_queue_t`.

## Validation

Le test `tests/unit/kernel/test_ne2k.c` injecte des fonctions `inb/outb`, vérifie la séquence d’initialisation et rejette l’absence d’acquittement reset. La validation locale du groupe est :

```text
make test-all                  274 tests, 274 passés, 0 échec, 0 ignoré
make all                       build i386 et initrd réussis
```

Le prochain sous-lot devra récupérer l’adresse MAC et brancher des opérations RX/TX contrôlées sur QEMU avec `-netdev user -device ne2k_isa`.
