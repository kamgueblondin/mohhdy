# AOS-129 — Configuration des anneaux NE2000

Le lot AOS-129 ajoute la configuration bornée des pages mémoire NE2000: page de transmission `0x40`, anneau de réception `0x46..0x60`, frontière de réception initiale `0x46`, réception broadcast et activation du contrôleur. La séquence est effectuée via les callbacks d’E/S injectables existants, sans allocation dynamique ni état global.

Le lot ne prétend pas encore effectuer le DMA distant, la lecture PROM de la MAC, l’émission matérielle ou le raccordement du périphérique QEMU au chemin de boot. Il établit le contrat d’initialisation qui précède le polling RX/TX effectif.

La validation locale est de **284 tests verts**, incluant trois scénarios NE2000, avec build i386 et initrd réussis.
