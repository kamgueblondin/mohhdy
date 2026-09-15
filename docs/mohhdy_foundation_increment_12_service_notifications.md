# MOHHDY Foundation — Incrément 12 : notifications de service

> **Statut : livré et vérifié localement.** L’incrément 12 ajoute une notification best-effort, non bloquante et locale lorsque le propriétaire d’un nom de service change.

## Objectif

Le registre Foundation associait déjà un nom court à un PID et permettait le transfert par son propriétaire. Un bénéficiaire devait cependant interroger ou tenter régulièrement le registre pour constater un transfert. Cette tranche introduit `SYS_SERVICE_NOTIFY`, qui permet à une tâche Ring 3 de s’abonner à un nom et de recevoir un événement IPC synthétique lorsque le nom est publié, transféré, retiré ou purgé.

| Élément | Contrat livré |
|---|---|
| ABI | `SYS_SERVICE_NOTIFY = 30` ; `MAX_SYSCALLS = 31`. |
| Abonnements | Huit couples `(nom, PID)` au plus ; une seconde souscription identique est idempotente. |
| Transport | Boîte IPC noyau existante, message `OS_IPC_SERVICE_EVENT`, émetteur noyau `sender_pid = 0`. |
| Charge | Nom NUL-paddé de 16 octets, ancien PID, nouveau PID et code de raison ; 28 octets utiles. |
| Échec de livraison | La boîte pleine ne bloque ni le transfert ni la purge ; l’événement est alors perdu explicitement selon la sémantique best-effort. |
| Nettoyage | Les abonnements d’une tâche sont retirés à `SYS_EXIT` et `SYS_KILL`. |

## Raisons d’événement

| Code | Transition |
|---:|---|
| `1` | Publication : `0 → nouveau PID`. |
| `2` | Transfert : `ancien PID → nouveau PID`. |
| `3` | Retrait explicite : `ancien PID → 0`. |
| `4` | Purge de cycle de vie ou détection lazy : `ancien PID → 0`. |

L’événement est construit par le noyau, avec `request_id = 0`, puis transmis aux abonnés trouvés au moment de la transition. Le noyau attribue `sender_pid = 0` ; un message utilisateur ne peut donc pas être interprété comme un événement valide par le parseur partagé.

## Démonstration visible

Le shell fournit `service-watch <nom>`, qui enregistre l’abonnement. `ipc-recv` reconnaît ensuite les événements et les affiche sous la forme :

```text
service-event demo old 1 new 2 reason 2
```

`serviceclaim` n’effectue plus une boucle d’inscription active lorsqu’il trouve `demo` occupé. Il s’abonne, annonce `serviceclaim waiting demo`, reçoit l’événement de transfert, puis confirme `serviceclaim claimed demo` lorsque le registre le désigne effectivement comme propriétaire.

## Vérification

Les tests Unity couvrent l’idempotence des abonnements, la capacité maximale, leur nettoyage, l’instantané des noms d’un propriétaire avant purge et la sérialisation/désérialisation bornée de l’événement. Le contrat QEMU de transfert vérifie l’abonnement du shell, l’événement de transfert, la réaction de `serviceclaim`, puis l’événement de purge après `kill`.

```bash
make test-all
make integration-qemu
make ci
```

## Limites honnêtes

Cette API ne fournit pas de capability, d’authentification du propriétaire, d’accusé de réception, de reprise, de journal durable, de filtre avancé ni de garantie de livraison. La capacité totale est huit abonnements et la capacité de la boîte IPC d’un destinataire reste quatre messages ; une saturation entraîne une perte best-effort plutôt qu’un blocage du registre. L’abonnement n’est pas une attente bloquante ni un mécanisme de RPC. Le registre demeure volatile et le noyau reste monolithique.
