# Plan du Lot 28 - Mohhdy OS

## Objectif
Phase Majeure d'Infrastructure Systeme, Web Engine Vectoriel & P2P WAN Distributed Mesh (~5000 lignes de C Ring 3 / Freestanding) couvrant les briques de la feuille de route (US-001 / US-031 / US-040 / US-061 / US-063 / US-107 / US-115).

## Tableau des Taches et Statuts

| Tache | Description | Statut |
|---|---|---|
| 1. Subsysteme VFS & Page Cache Daemon (US-001 / US-033) | LRU/Clock virtual page cache, dirty-writeback flushing (vfs_cache_flush, vfs_cache_sync) et scheduler I/O par priorites dans userspace/ramfs.c et userspace/ramfs.h. | Termine |
| 2. Moteur Web Runtime Box Model & Vector Rasterizer (US-031 / US-040 / US-042) | Calcul complet du CSS Box Model, line wrapping, text alignment, composantes de formulaires HTML interactives (<input>, <button>, <textarea>) et rendu 2D vectoriel dans userspace/webrt.c et userspace/webrt.h. | Termine |
| 3. Reseau P2P WAN Multi-Hops, Gossip & NAT Hole Punching (US-061 / US-063 / US-074) | Protocole de Gossip epidemique, table d'etats NAT keepalive et selection de route multi-chemins EWMA dans userspace/p2p.c et userspace/p2p.h. | Termine |
| 4. Telemetrie Temps Reel & Auto-Healing (US-107 / US-110 / US-115) | Profiler CPU/memoire avec detection de fuites/boucles, runtime d'auto-guerison autonome et calcul de percentiles de latence (p50, p95, p99) dans userspace/prod.c et userspace/prod.h. | Termine |
| 5. Documentation & Verification Completes | Creation de docs/lot28_plan.md, mise a jour de docs/ETAT_REEL.md, docs/mohhdy_execution_and_roadmap.md, et docs/todo.md. Execution de make test-all et des tests QEMU OS-UI. | Termine |
