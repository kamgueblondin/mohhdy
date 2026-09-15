# Incrément Foundation 35 — Portée par service des générations backend

## Objet

L’incrément 35 retire l’invalidation croisée introduite par la génération backend globale. Chaque entrée publiée du registre possède désormais sa propre génération backend, initialisée à `1` lors de son enregistrement. L’observation de `vfs` ne devient donc plus obsolète lorsqu’une délégation du service `demo` est créée, modifiée ou révoquée.

| Élément | Comportement |
|---|---|
| Stockage | `backend_generation` dans l’entrée de registre du service publié |
| Initialisation | `1` lors de `service_registry_register` |
| Mutation ciblée | Seule la génération du nom dont la délégation change progresse |
| Observation | `vfs-backend-observe` compare uniquement la génération de `vfs` |
| Purgation | La génération du nom stocké dans chaque délégation est avancée avant l’effacement |

Les transitions d’octroi, changement de profil, révocation, transfert et purge restent observables par le propriétaire du service concerné. Les noms non concernés ne reçoivent pas de faux état `stale`.

> Cette amélioration réduit les faux positifs d’obsolescence ; elle n’ajoute ni verrou, ni transaction, ni capability, ni garantie d’instantané atomique.

## Vérification

Le test de registre crée `vfs` et `demo`, modifie une délégation de `demo`, puis vérifie que l’observation de `vfs` à la génération `1` reste valide. Il démontre ensuite qu’un octroi sur `vfs` fait bien avancer uniquement sa génération et retourne `OS_SERVICE_STALE` pour l’ancienne valeur. `make test-all` reste vert avec **217/217** tests.
