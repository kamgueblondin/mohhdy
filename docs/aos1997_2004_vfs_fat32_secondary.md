# AOS-1997 à AOS-2004 — VFS FAT32 secondaire multi-disque

Le contrat QEMU VFS démarre désormais deux disques IDE déterministes : une image FAT16 primaire contenant l’overlay à LBA 0 et sa fixture à LBA 64, ainsi qu’une image FAT32 secondaire montée par le noyau sur le canal esclave. Le scénario crée les alias `media/` (FAT16) et `media32/` (FAT32), puis vérifie pour chacun le listage, la lecture et le statut d’un fichier réel.

| Alias | Fixture | Contrôles |
|---|---|---|
| `media/` | `FATOK.TXT`, 17 octets | listage, lecture, statut sans casse |
| `media32/` | `FAT32OK.TXT`, 27 octets | listage, lecture, statut sans casse |

La table de montages VFS est statique et contient huit entrées : quatre sources protégées et quatre alias dynamiques. La vue virtuelle `vfs-mounts` reste strictement bornée à `OS_VFS_READ_MAX` ; les entrées qui ne tiennent pas sont omises sans écriture hors borne. Les mutations restent limitées à l’overlay.

Les assertions de statistiques de synthèse vérifient la publication des compteurs, tandis que les opérations de lecture, statut et mutation possèdent chacune leur assertion fonctionnelle directe. Cette distinction évite de confondre les reprises de commandes du clavier HMP avec la validité des parcours VFS.

## Références

[1] [Médiateur VFS Ring 3](../userspace/vfs_server.c)

[2] [Contrat QEMU VFS multi-disque](../tests/integration/test_qemu_vfs_service.py)

[3] [Constructeur FAT32 secondaire](../scripts/make_fat32_secondary_image.py)
