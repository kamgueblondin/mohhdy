# AOS-131 — Sonde NE2000 au démarrage du noyau

Le lot AOS-131 raccorde la sonde et la configuration des anneaux NE2000 au chemin d’initialisation du noyau sur le port ISA `0x300`. L’initialisation est tolérante à l’absence de carte: le boot poursuit son exécution et le noyau imprime explicitement que le réseau reste désactivé. Lorsqu’un contrôleur répond, les callbacks I/O i386 existants sont utilisés pour appliquer la préparation et les pages RX/TX.

Le statut reste volontairement non déclaré dans `net-status` tant qu’aucun syscall de publication d’état n’est branché. Le lot ne lit pas encore la PROM MAC, ne configure pas d’IRQ réseau, n’effectue pas de DMA distant et ne transmet pas de trame.

La validation locale est de **284 tests Unity verts**, build i386 réussi et smoke QEMU complet réussi: core, extras, persistance, spawn et exec.
