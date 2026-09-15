# ASSIST-050 - image Docker du runtime agent

**Date :** 15 septembre 2026
**Statut :** image HTTP livrée ; sessions et admin = [assist010_sessions_admin.md](assist010_sessions_admin.md)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit comment construire, lancer et vérifier le conteneur
HTTP du track Agent Support. Il n'héberge pas le noyau Multiboot i386.
Il n'allonge pas `make integration-qemu`. Il n'entre pas dans `make ci`.

Spec produit : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md)
(ASSIST-050). Sessions / embed / admin : ASSIST-010, 011, 040.
Plan : [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md).

## Ce que l'image sert

Arborescence parallèle `agent/` (Python 3, bibliothèque standard uniquement) :

| Route | Réponse |
|---|---|
| `GET /health` | 200 JSON `status=ok`, `service=mohhdy-agent`, `llm=stub_echo` |
| `GET /admin` | Console HTML : liste et détail des sessions |
| `GET /embed.js` | Widget public (bulle + session HTTP), **sans secret** |
| `GET /demo` | Page hôte qui charge `embed.js` |
| `GET /demo-app` | Appli hôte mock (menu, formulaire facture, simulateur DOM) |
| `GET /` | Index des routes |
| `POST /api/sessions` | Ouvre une session visiteur |
| `POST /api/sessions/{id}/tools` | Geste ou outil MCP allowliste |

Honnêteté produit :

- Les réponses chat sont un **stub local** (echo ou extraits de KB), pas un LLM de production.
- Gestes navigateur : **simulateur DOM** in-process, pas Chromium / Playwright. Guide : [assist020_gestes_mcp.md](assist020_gestes_mcp.md).
- Escalade et handoff : [assist012_droits_handoff.md](assist012_droits_handoff.md).
- Le noyau Multiboot **n'est pas** booté dans ce conteneur.
- `ADMIN_TOKEN` se passe au `docker run`, jamais dans l'image.

Détail sessions / CSP / snippet : [assist010_sessions_admin.md](assist010_sessions_admin.md).

## Prérequis

- Python 3.12+ pour la fumée locale (`make agent-smoke`) : déjà le Python hôte du prototype.
- Docker (BuildKit optionnel) **seulement** pour l'image. Absent de la CI AOS.

Aucun paquet pip. Aucun `.env` n'est lu au démarrage.

## Construire et lancer (`docker run`)

Depuis la racine du dépôt :

```text
docker build -t mohhdy-agent ./agent
docker run --rm -p 8080:8080 mohhdy-agent
```

Admin protégé (jeton **uniquement** à l'exécution) :

```text
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-agent
```

Cible Make equivalente (ne touche pas QEMU) :

```text
make agent-docker
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-agent
```

Compose local, depuis `agent/` :

```text
cd agent
ADMIN_TOKEN=change-me-at-runtime docker compose up --build
```

Le service écoute `0.0.0.0:8080`. Surcharge possible :

- `MOHHDY_AGENT_HOST` (défaut `0.0.0.0`)
- `MOHHDY_AGENT_PORT` (défaut `8080`)
- `ADMIN_TOKEN` (optionnel, runtime seulement)
- `MOHHDY_AGENT_DATA` (optionnel : répertoire JSON des sessions)
- `MOHHDY_AGENT_CONFIG` (optionnel : JSON KB + allowlist)
- `MOHHDY_AGENT_KB` (optionnel : fichier texte / Markdown / JSON d'extraits)
- `MOHHDY_AGENT_MODE` (optionnel : `self_host` ou `hosted`, scaffold ASSIST-053)
- `MOHHDY_AGENT_SITE_ID` (optionnel : identifiant d'instance)

Ne pas passer de jeton OpenAI, de `.env` ou de `env_file` dans le **build**.
L'image est construite utilisateur non-root `mohhdy` (uid 10001).
`.dockerignore` exclut `.env`, clés et tests.

## Vérifier

Fumée stdlib, **sans Docker**, hors gate AOS :

```text
make agent-smoke
```

Santé, isolation de deux sessions, liste admin, jeton, révocation,
escalade, handoff. N'est **pas** appelée par `make ci` ni par
`make integration-qemu`.

Fumée contre une origine déjà lancée :

```text
python3 agent/server.py
BASE_URL=http://127.0.0.1:8080 agent/scripts/smoke.sh
```

Controles manuels :

```text
curl -fsS http://127.0.0.1:8080/health
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/admin
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/embed.js
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/demo
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/demo-app
```

Ouvrir `http://127.0.0.1:8080/demo` : la bulle en bas à droite ouvre une
session et accepte un message. L'echo stub s'affiche dans le panneau.

## Ce que l'image ne contient pas

- Secret, `.env`, clé API, certificat privé, `ADMIN_TOKEN` cuit
- Binaire noyau `build/mohhdy.bin`, initrd, ISO GRUB
- Dépendance pip, Node, navigateur outillé (Chromium n'est pas dans l'image slim)
- Modèle GPT-2 / GGUF

## Relation au prototype AOS

Le hobby OS i386 reste la couche vérifiée ([ETAT_REEL.md](ETAT_REEL.md)).
Ce runtime ne remplace pas QEMU, ne s'ajoute pas aux sept contrats
d'intégration, et ne doit pas faire grandir le job `integration-qemu`.
Le job CI optionnel `agent-http-smoke` (Python stdlib, sans Docker) tourne
en parallèle ; il ne `needs` pas le build i386.

Install PC, hyperviseur et mode hosted : [assist051_052_053_deploy.md](assist051_052_053_deploy.md).
