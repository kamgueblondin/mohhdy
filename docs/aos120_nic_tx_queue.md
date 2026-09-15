# AOS-120 — Mise en file TX Ethernet

Le lot AOS-120 ajoute `net_nic_queue_push_frame`, une primitive TX caller-owned qui rejette les trames inférieures à la taille Ethernet minimale de 60 octets, vérifie la capacité de l’emplacement réservé, copie les octets dans la file et publie la trame par `commit`.

Cette opération ne transmet pas encore sur le contrôleur NE2000. Elle sépare explicitement la préparation d’une trame du futur transfert matériel, évite les allocations dynamiques et garantit qu’une trame trop courte ne quitte pas le contrat Ethernet.

Le test `test_net_nic.c` couvre le rejet d’une trame de 59 octets, la copie et la restitution FIFO d’une trame de 60 octets. La validation locale est de **276 tests verts** et le build i386 complet réussit.
