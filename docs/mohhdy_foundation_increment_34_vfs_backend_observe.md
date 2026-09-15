# Incrément Foundation 34 — Observation générationnelle des délégations backend VFS

## Objet

L’incrément 34 permet au propriétaire public vivant de `vfs` de vérifier qu’un inventaire de délégations backend n’est pas devenu obsolète. La commande `vfs-backend-observe <generation>` transmet une génération attendue au médiateur. Elle retourne l’inventaire lorsque la génération est encore courante, ou l’état explicite `stale` avec la génération actuelle lorsqu’une mutation a eu lieu.

| Élément | Contrat |
|---|---|
| Commande | `vfs-backend-observe <generation>` |
| Syscall | `SYS_SERVICE_BACKEND_OBSERVE` (`50`) |
| ABI | `MAX_SYSCALLS = 51` ; plage `0–50` |
| Requête IPC | `OS_IPC_VFS_BACKEND_OBSERVE` (`0x56465324`), génération `uint32` |
| Réponse IPC | `OS_IPC_VFS_BACKEND_OBSERVE_REPLY` (`0x56465325`), 44 octets |
| Réponse | statut, génération courante, compteur, puis au plus quatre couples PID/masque |

La génération commence à `1` au démarrage du registre et reste non nulle. Elle évolue après un nouvel octroi backend, une modification effective de profil, une révocation, une purge du bénéficiaire, un retrait du propriétaire ou un transfert qui retire les délégations associées. Un octroi idempotent qui laisse le profil inchangé ne la fait pas évoluer.

> Une génération signale seulement qu’un instantané n’est plus actuel. Elle ne délivre pas de capability et ne transforme pas l’inventaire en autorisation.

Si la génération attendue vaut `0`, le médiateur renvoie l’inventaire courant. Si elle est différente de la génération active, le registre renvoie `OS_SERVICE_STALE`, transmet la génération courante et vide toutes les entrées. Le registre remet aussi la sortie à zéro avant un refus de propriétaire ou de nom invalide.

## Vérification

`make test-all` valide **217/217** tests. La nouvelle preuve du registre couvre les générations 1, 2 et 3, l’obsolescence après octroi et révocation, la liste vide lors de `stale` et l’absence de divulgation pour un autre propriétaire.

Le contrat QEMU VFS observe l’inventaire complet à la génération 2 après l’octroi complet, puis exige `stale generation 3` après révocation. La stabilisation du contrat sépare en outre les marqueurs série `type 0` et `data deferred`, car les traces de l’ordonnanceur peuvent légalement s’intercaler entre ces deux fragments de sortie sans perte du message IPC.

La génération est globale aux délégations backend du registre et ne constitue pas un verrou, un jeton de réservation, une ACL par chemin, une identité vérifiée, une expiration ou un audit durable. Chaque primitive backend garde donc son contrôle d’accès au moment effectif de l’opération.
