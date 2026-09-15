# AOS-135 — Raccordement IRQ3 NE2000

Le lot AOS-135 raccorde le contrôleur NE2000 ISA de QEMU à l’IRQ3 du PIC maître. Le noyau installe un stub assembleur dédié au vecteur 35, enregistre le handler C `ne2k_irq_handler`, démasque IRQ3 avec IRQ0 et IRQ1, puis attache le périphérique après une sonde et une configuration d’anneaux réussies.

Le pilote conserve uniquement des références statiques vers le périphérique et ses callbacks d’E/S. `ne2k_irq_service` lit l’ISR, acquitte les bits observés par écriture miroir et incrémente un compteur volatil borné par la largeur `uint32_t`. En l’absence de périphérique, le handler reste inoffensif ; aucune allocation et aucun buffer de trame ne sont introduits dans le chemin d’interruption.

| Élément | État AOS-135 |
|---|---|
| Stub assembleur IRQ3 / vecteur 35 | Implémenté. |
| Enregistrement handler et démasquage PIC | Implémenté. |
| Acquittement ISR NE2000 | Implémenté et testé par callbacks injectés. |
| Compteur d’événements IRQ | Implémenté. |
| Extraction RX depuis la RAM distante | Non raccordée à l’IRQ dans ce lot. |
| DMA distant, DHCP, DNS, TLS et HTTP/OpenAI | Non implémentés comme transport effectif. |

La validation locale reste à **292 tests verts** et le build i386 réussit. Le smoke QEMU vérifie la stabilité du boot avec et sans `ne2k_isa`, mais ne prétend pas observer une trame Ethernet reçue par le noyau.
