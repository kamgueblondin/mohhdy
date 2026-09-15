# ASSIST-050 - image Docker de l'instance Mohhdy

**Date :** 15 septembre 2026
**Statut :** image HTTP bootstrap `mohhdy-agent` livree ; entree produit = `mohhdy-os` ([osui_0_1_2.md](osui_0_1_2.md))
**Ponctuation :** ASCII usuel et accents français uniquement

L'entree produit Docker est le shell graphique `osui/` (`docker run mohhdy-os`,
port 8080, `GET /`). Ce document decrit encore le conteneur bootstrap
`mohhdy-agent` (APIs). Docker n'heberge pas le noyau Multiboot i386. Il
n'allonge pas `make integration-qemu`. Il n'entre pas dans `make ci`.

Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md)
(ASSIST-050). Sessions / embed / admin : ASSIST-010, 011, 040.
Plan (un produit) : [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md).

## Ce que l'image sert

Arborescence `agent/` (Python 3, bibliotheque standard uniquement) : scaffold userspace de l'instance, **pas** un produit a cote.

| Route | Réponse |
|---|---|
| `GET /health` | 200 JSON `status=ok`, `service=mohhdy-agent`, `llm=stub_echo` |
| `GET /admin` | Console HTML : liste et détail des sessions |
| `GET /embed.js` | Widget public (bulle + session HTTP), **sans secret** |
| `GET /demo` | Page hôte qui charge `embed.js` |
| `GET /demo-app` | Appli hôte mock (menu, formulaire facture, simulateur DOM) |
| `GET /browser` | Vue navigateur d'instance : miroir du simulateur (ASSIST-060, pas US-031) |
| `GET /browser/fs` | Explorateur du FS sandbox (ASSIST-061) |
| `GET /` | Index des routes |
| `POST /api/sessions` | Ouvre une session visiteur |
| `POST /api/sessions/{id}/tools` | Geste ou outil MCP allowliste |

Honnêteté produit :

- Les réponses chat sont un **stub local** (echo ou extraits de KB), pas un LLM de production.
- Gestes navigateur : **simulateur DOM** in-process pour les sessions. Playwright / Chromium : profil **optionnel** hors image slim. Guide : [assist_playwright_optional.md](assist_playwright_optional.md).
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
- `MOHHDY_AGENT_RUNTIME` (optionnel : `docker` ou `browser`, ASSIST-061)
- `MOHHDY_AGENT_BROWSER_ENGINE` (optionnel : `off`, `playwright` ou `chromium` ; voir [assist_playwright_optional.md](assist_playwright_optional.md))
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
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/browser
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/browser/fs
```

Ouvrir `http://127.0.0.1:8080/demo` : la bulle en bas à droite ouvre une
session et accepte un message. L'echo stub s'affiche dans le panneau.

## Ce que l'image ne contient pas

- Secret, `.env`, clé API, certificat privé, `ADMIN_TOKEN` cuit
- Binaire noyau `build/mohhdy.bin`, initrd, ISO GRUB
- Dépendance pip, Node, navigateur outillé (Chromium n'est pas dans l'image slim ; extra : `Dockerfile.playwright` cible `playwright`)
- Modèle GPT-2 / GGUF

## Relation au prototype guest

Le guest i386 reste la tranche **mesuree** ([ETAT_REEL.md](ETAT_REEL.md)).
Ce scaffold ne remplace pas QEMU, ne s'ajoute pas aux sept contrats
d'integration, et ne doit pas faire grandir le job `integration-qemu`.
Le job CI optionnel `agent-http-smoke` (Python stdlib, sans Docker) tourne
en parallele ; il ne `needs` pas le build i386. C'est le meme produit :
l'instance autonome, pas un sidecar.

Install PC, hyperviseur et mode hosted : [assist051_052_053_deploy.md](assist051_052_053_deploy.md).
Vue navigateur et FS sandbox : [assist060_061_browser.md](assist060_061_browser.md).
Profil Playwright optionnel : [assist_playwright_optional.md](assist_playwright_optional.md).
