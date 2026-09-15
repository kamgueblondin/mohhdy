# ASSIST-060 / 061 - vue navigateur d'instance et FS sandbox

**Date :** 15 septembre 2026
**Statut :** premiere tranche livree dans `agent/` (simulateur DOM, profil Playwright optionnel, pas US-031)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit comment, une fois l'**instance Mohhdy** déployée, un
opérateur ouvre une **vue navigateur locale** (navigateur-OS du SE,
bootstrap) et, si le runtime est oriente navigateur, consulte un
**systeme de fichiers virtuel** borne.
Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).
Image : [assist050_docker_runtime.md](assist050_docker_runtime.md).
Gestes : [assist020_gestes_mcp.md](assist020_gestes_mcp.md).
Plan : [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md).

Ce n'est **pas** US-031 livre. Le navigateur-OS est un **devoir du SE**.
La phase 3 / US-031 restent **non implementes**. Playwright / Chromium
est un **profil optionnel** : absent du chemin slim (`make agent-smoke`,
`Dockerfile`). Guide :
[assist_playwright_optional.md](assist_playwright_optional.md). Aucun
Internet public hors allowlist. Aucun secret dans l'image.
`docs/ETAT_REEL.md` n'est **pas** mis a jour : le guest n'a pas gagne
ce FS.

## ASSIST-060 : URLs de la vue instance

Apres `python3 agent/server.py` ou `docker run -p 8080:8080 mohhdy-agent` :

| URL | Role |
|---|---|
| `GET /browser` | Page operateur : miroir du simulateur `/demo-app` (iframe locale) et etat harness |
| `GET /api/browser` | JSON : runtime, harness, `browser_engine`, drapeaux phase 3 / US-031 a false, snapshot DOM, etat page optionnel |
| `POST /api/browser/navigate` | Operateur : ouvrir une URL locale ou allowlistee (501 si Playwright absent) |
| `GET /api/browser/screenshot` | Capture PNG si une page optionnelle est ouverte (501 sinon) |
| `GET /demo-app` | Appli hote mock deja livree (ASSIST-020) |
| `GET /admin` | Console sessions (autre surface, pas un navigateur) |

La page `/browser` sonde `/api/browser` et reaffiche ce que l'agent
voit dans le simulateur DOM (menu, formulaire, pointeur). Meme origine,
sans reseau public.

Honnêteté :

- Harness session = `dom_simulator`. Moteur optionnel : `browser_engine=optional_not_installed` tant que Playwright n'est pas installe.
- `phase3_complete=false`, `us031_complete=false` dans `/health` et `/api/browser`.
- Administrer hors du seul panneau : `/browser` plus `/demo-app` et `/admin`.

## ASSIST-061 : FS navigateur (sandbox d'instance)

Quand MOHHDY est lance comme runtime oriente navigateur **ou** en
agent Docker, l'instance expose un FS **virtuel** :

| Racine | Contenu |
|---|---|
| `demo/` | Assets statiques (`agent/static/` : pages HTML, JS, CSS, texte) |
| `data/` | Seulement si `MOHHDY_AGENT_DATA` pointe vers un repertoire (sessions, factures demo) |

API (lecture seule) :

```text
GET /api/browser/fs
GET /api/browser/fs?path=demo
GET /api/browser/fs?path=demo/demo-app.html
GET /api/browser/fs/list?path=demo
GET /api/browser/fs/read?path=demo/fs-sandbox.txt
```

UI : `GET /browser/fs` (explorateur, jeton comme `/admin`).

Auth :

- Si `ADMIN_TOKEN` est defini a l'execution, listage et lecture
  exigent `Authorization: Bearer`. Sans variable : mode stub ouvert
  (comme l'admin).
- POST / PUT / DELETE : `405 fs_read_only`. Aucune mutation exposee.
  (S'il y en avait, elles seraient aussi protegees par le jeton.)

Isolation :

- Traversal (`../`, chemin absolu, lien hors sandbox) : `403 path_denied`.
- Pas le code Python (`server.py`, `tools.py`).
- Pas de `.env`, `.pem`, `.key`.
- Pas le FS du guest i386 AOS.

## Mode docker vs browser

| Valeur | Comment | Sens |
|---|---|---|
| `docker` (defaut) | `MOHHDY_AGENT_RUNTIME=docker` ou JSON `runtime.kind` | Agent conteneur / install PC. Meme HTTP. FS sandbox d'instance quand meme disponible |
| `browser` | `MOHHDY_AGENT_RUNTIME=browser`, profil compose `browser`, `Dockerfile.playwright` (cible slim) | Lancement oriente navigateur (vision "le web est le FS"). Simulateur, pas Chromium |
| `playwright` (extra) | `MOHHDY_AGENT_BROWSER_ENGINE=playwright`, profil compose `playwright` | Controle operateur /browser. Gros. Pas US-031. Gestes de session = simulateur |

`GET /health` publie `runtime`. L'environnement prime sur le JSON.

```text
MOHHDY_AGENT_RUNTIME=browser python3 agent/server.py
```

Le profil compose optionnel :

```text
cd agent
docker compose --profile browser up --build
```

Chromium n'est **pas** dans l'image slim. Pour le profil optionnel :
[assist_playwright_optional.md](assist_playwright_optional.md).

## Verifier (hors QEMU, hors make ci)

```text
make agent-smoke
python3 agent/server.py
curl -fsS http://127.0.0.1:8080/browser | head
curl -fsS http://127.0.0.1:8080/api/browser
curl -fsS http://127.0.0.1:8080/api/browser/fs?path=demo
```

Avec jeton :

```text
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-agent
curl -fsS -H 'Authorization: Bearer change-me-at-runtime' \
  http://127.0.0.1:8080/api/browser/fs?path=demo
```

## Hors perimetre (cette tranche)

- Navigateur-OS US-031, onglets-processus, phase 3 complete
- Gestes de session routes via Playwright (reste le simulateur)
- Ecriture dans le sandbox, Internet public hors allowlist, OpenAI, secret dans l'image
- Chromium dans `Dockerfile` slim / `make agent-docker`
- Allonger `make integration-qemu`
