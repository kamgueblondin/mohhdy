# AOS-119 — Extraction RX NE2000 bornée

Le lot AOS-119 ajoute `ne2k_rx_extract`, une primitive qui lit l’en-tête de réception fourni par le DMA NE2000, valide le statut et la longueur little-endian, copie uniquement la charge utile dans une file `net_nic_queue_t`, puis publie la longueur par `commit`.

La fonction rejette les buffers trop courts, les paquets dont le bit de succès est absent, les longueurs inférieures à l’en-tête ou supérieures au buffer DMA et les files saturées. Elle ne lit jamais au-delà des bornes fournies et n’alloue aucune mémoire.

Le test `test_ne2k.c` couvre la publication FIFO d’une trame de cinq octets et le rejet d’une réception en erreur. La validation locale est de **275 tests verts**, avec compilation complète de la suite Unity. Le build i386 devra être relancé avant publication de la PR.

Cette primitive prépare le chemin RX mais ne programme pas encore les registres de ring NE2000, ne traite pas les interruptions et ne fournit pas de transmission TX autonome.
