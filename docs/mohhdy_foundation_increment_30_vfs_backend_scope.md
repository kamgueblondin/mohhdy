# Incrément Foundation 30 — Capacité backend VFS de moindre privilège

## Objet

L’incrément 30 remplace le caractère nécessairement global des nouvelles délégations backend VFS par un masque de droits limité par opération. Le propriétaire public de `vfs` conserve l’ensemble des primitives backend. Un bénéficiaire peut désormais recevoir un profil **lecture seule**, qui ne permet pas de muter l’overlay.

| Élément | Contrat |
|---|---|
| Droit lecture | `OS_VFS_BACKEND_RIGHT_READ` (`1`) : lecture, métadonnées et listages backend |
| Droit mutation | `OS_VFS_BACKEND_RIGHT_MUTATE` (`2`) : écriture, renommage, suppression et création backend |
| Droit complet | combinaison lecture + mutation (`3`) ; compatibilité de `vfs-backend-grant` |
| Commande minimale | `vfs-backend-grant-read <pid>` |
| IPC corrélé | `OS_IPC_VFS_BACKEND_GRANT_SCOPED` / réponse, PID et masque sur 8 octets |
| Syscall | `SYS_SERVICE_BACKEND_GRANT_SCOPED` (47), avec le masque en EDX |
| ABI | 48 syscalls, indices 0–47 |

Le registre conserve un masque dans chaque entrée de capacité backend. Le contrôle noyau est effectué à chaque primitive : les lectures, `stat` et listages demandent le droit lecture ; les écritures, suppressions, renommages, `mkdir` et `rmdir` demandent le droit mutation. Le propriétaire actuel du nom `vfs` n’est jamais limité par ce masque.

## Compatibilité et révocation

`vfs-backend-grant <pid>` garde sa sémantique historique et délègue les deux droits. Un octroi scoped sur une entrée déjà existante met à jour son masque sans créer de doublon. La révocation explicite, les purges de PID, le retrait de nom et les transferts de service retirent toujours l’entrée entière, quel que soit son profil.

> Le périmètre est **par opération**, pas par chemin ni par fichier. Il ne fournit pas de droits temporels, de quotas, de journal durable, d’identité vérifiée ou de tokens non forgeables. Le PID demeure local et volatile.

## Vérification

Le test Unity du registre ajoute un cas lecture seule : lecture autorisée, mutation refusée, octroi complet compatible et masque invalide rejeté. `make test-all` valide **213/213** tests.

Le contrat QEMU VFS lance `vfsreadclaim`, attend son état d’attente, appelle `vfs-backend-grant-read`, puis exige le marqueur `read-only enforced`. Ce client lit `hello.txt` par le backend et vérifie que l’écriture backend de `scope.txt` retourne `OS_VFS_BACKEND_DENIED`; le contrat conserve également la vérification du propriétaire public `vfs`.
