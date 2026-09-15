# Incrément Foundation 31 — Capacité backend VFS de mutation seule

## Objet

L’incrément 31 complète les profils backend VFS de moindre privilège avec une délégation **mutation seule**. Elle utilise le masque déjà transporté par l’octroi scoped ; aucun syscall ni type IPC supplémentaire n’est nécessaire.

| Élément | Contrat |
|---|---|
| Commande shell | `vfs-backend-grant-mutate <pid>` |
| Masque transporté | `OS_VFS_BACKEND_RIGHT_MUTATE` (`2`) |
| Opérations autorisées | écriture, suppression, renommage, `mkdir` et `rmdir` backend |
| Opérations refusées | lecture, métadonnées et listages backend |
| Protocole | `OS_IPC_VFS_BACKEND_GRANT_SCOPED` / réponse corrélée |
| Propriété publique | le nom `vfs` reste détenu par `vfsserver` |

Le shell délègue le profil mutation seule par l’IPC corrélé existant. Le médiateur appelle le syscall scoped déjà livré ; le registre applique le même contrôle par opération, et la révocation explicite ainsi que les purges de cycle de vie continuent de retirer l’entrée entière.

## Preuve et isolation

Le nouveau programme Ring 3 `vfsmutateclaim` attend le droit scoped. Après l’octroi, il vérifie qu’une lecture backend de `hello.txt` est refusée, écrit `mutate.txt`, puis le supprime immédiatement par `SYS_VFS_OVERLAY_UNLINK`. Il annonce `mutate-only enforced` uniquement si ces trois conditions sont respectées.

> Le nettoyage de `mutate.txt` évite de modifier le contrat de contenu de l’overlay. Le scénario QEMU termine explicitement chaque client de preuve — capacité complète révoquée, lecture seule et mutation seule — avant de poursuivre les tests historiques, afin de ne pas épuiser les emplacements de tâches limités de l’OS.

## Vérification

Le test Unity de registre couvre maintenant les trois profils : lecture seule, mutation seule et complet. Il confirme que le profil mutation seule refuse `READ` mais accepte `MUTATE`, et que les masques hors contrat sont rejetés. `make test-all` valide **213/213** tests.

Le contrat QEMU VFS valide successivement l’octroi complet et sa révocation, l’octroi lecture seule, l’octroi mutation seule et la purge des trois PID de preuve. Il vérifie ensuite l’overlay vide, les mutations corrélées, les statistiques finales `reads=13`, `writes=3`, `removes=2`, `renames=2`, puis le transfert et la purge du service `vfs`. Le contrat réussit intégralement.
