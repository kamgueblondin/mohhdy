# AOS-130 — Adaptateur d’E/S i386 NE2000

Le lot AOS-130 ajoute `ne2k_i386_io`, qui fournit les callbacks `inb/outb` réels du noyau i386 aux primitives NE2000 existantes. L’assembleur inline est encapsulé dans le pilote et l’API retourne un refus hors i386; les tests Unity continuent d’utiliser des callbacks simulés et ne touchent jamais les ports matériels.

Cette étape permet le raccordement ultérieur du probe et de la configuration des anneaux à un `ne2k_isa` QEMU. Elle ne modifie pas encore le chemin de boot, ne lit pas la PROM MAC et n’envoie aucune trame.

La validation locale est de **284 tests verts**, avec build i386 et initrd réussis.
